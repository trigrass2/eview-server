#include <string>
#include "HourTimer.h"
#include "TimerTask.h"
#include "pklog/pklog.h"
#include "json/json.h"
#include "pkdbapi/pkdbapi.h"
#include <algorithm>

#include "MainTask.h"
extern CPKLog g_logger;
using namespace std;

string Json2String(Json::Value &jsonVal);

string SubString(string src, int nLen)
{
	if (src.length() < nLen)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "when SubStr(%s,%d), srcLen:%d < %d", src.c_str(), nLen, src.c_str(), nLen);
		return "";
	}

	string strSubString = src.substr(0, nLen); // 2017-06-28 10:15:00.123 ---》2017-06-28 10:1
	return strSubString;
}

/**
*  构造函数.
*
*
*  @version     05/30/2008  lijingjing  Initial Version.
*/
CHourTimer::CHourTimer(CTimerTask *pTimerTask) :m_pTimerTask(pTimerTask)
{
	m_pRule = NULL;
	m_bFirstInsert = true;
}

/**
*  析构函数.
*
*
*  @version     05/30/2008  lijingjing  Initial Version.
*/
CHourTimer::~CHourTimer()
{
}

int CHourTimer::InitPKData()
{
	m_nTagDataNum = m_pRule->m_vecStatTagName.size();
	if (m_nTagDataNum > 0)
	{
		m_pkTagDatas = new PKDATA[m_nTagDataNum]();
		for (int i = 0; i < m_pRule->m_vecStatTagName.size(); i++)
		{
			string strTagName = m_pRule->m_vecStatTagName[i];
			PKStringHelper::Safe_StrNCpy(m_pkTagDatas[i].szObjectPropName, strTagName.c_str(), sizeof(m_pkTagDatas[i].szObjectPropName));
			strcpy(m_pkTagDatas[i].szFieldName, "");
		}
	}
	return 0;
}

void CHourTimer::StartTimer()
{
	// 组织每次定时器要访问的数据
	if (m_pRule->m_vecStatTagName.size() <= 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "没有需要计算区间差值的规则配置!");
		return;
	}

	g_logger.LogMessage(PK_LOGLEVEL_INFO, "配置了 %d个需要计算区间的变量规则, 将开启该定时器转存, 基础表名为:%s, 周期为1小时!", m_pRule->m_vecStatTagName.size(), "t_db_diff_xxxx");
	InitPKData();
	 
	LoadBeginRawValueFromDB2Map(DB_TABLENAME_DIFF_HOUR, m_mapTagName2BeginRawHourValue); // 读取数据库中上次的值;
	LoadBeginRawValueFromDB2Map(DB_TABLENAME_DIFF_DAY, m_mapTagName2BeginRawDayValue); // 读取数据库中上次的值;
	LoadBeginRawValueFromDB2Map(DB_TABLENAME_DIFF_MONTH, m_mapTagName2BeginRawMonthValue); // 读取数据库中上次的值;
	LoadBeginRawValueFromDB2Map(DB_TABLENAME_DIFF_YEAR, m_mapTagName2BeginRawYearValue); // 读取数据库中上次的值;

	ACE_Time_Value	tvPollRate;		// 扫描周期，单位ms
	tvPollRate.msec(30 * 1000);
	ACE_Time_Value tvPollDelay(0); // 1970-1-1 00:00:00
	m_pTimerTask->m_pReactor->schedule_timer((ACE_Event_Handler *)(CHourTimer *)this, NULL, tvPollDelay, tvPollRate);
}

void CHourTimer::StopTimer()
{
	m_pTimerTask->m_pReactor->cancel_timer((ACE_Event_Handler *)(CHourTimer *)this);
}


//每隔多少秒更新一次数据中的数据; 1127
int CHourTimer::handle_timeout(const ACE_Time_Value &current_time, const void *act /*= 0*/)
{
	char szCurDateTime[32] = { 0 };
	PKTimeHelper::GetCurHighResTimeString(szCurDateTime, sizeof(szCurDateTime));
	string strCurTime = szCurDateTime;
	string strYYYYMMDDHH = SubString(strCurTime, TIMELEN_YYYYMMDD_HH);

	char szSql[1024] = { 0 };
	// read tag data
	int nRet = pkMGet(this->m_pTimerTask->m_hPkData, m_pkTagDatas, m_nTagDataNum);
	if (nRet != 0)
	{
		char szTip[512];
		for (int iTmp = 0; iTmp < m_nTagDataNum; iTmp++)
		{
			PKDATA * pkData = m_pkTagDatas + iTmp;
			string strObjPropName = pkData->szObjectPropName;
			if (strlen(szTip) == 0)
				PKStringHelper::Safe_StrNCpy(szTip, pkData->szObjectPropName, sizeof(szTip));
			else
				PKStringHelper::Snprintf(szTip, sizeof(szTip), "%s,%s", szTip, pkData->szObjectPropName);
		}
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "rule:%s,  pkMGet获取:%d个对象属性数据返回失败, 点:%s, 返回值:%d", m_pRule->m_strName.c_str(), m_nTagDataNum, szTip, nRet);
		// return nRet;
	}

	// 每一行记录只需要1个质量和时间戳
	int nDataQuality = 0;
	string strDataTime = "";
	vector<CColValue> vecColValue;
	Json::Reader jsonReader;
	for (int iTag = 0; iTag < m_nTagDataNum; iTag++)
	{
		PKDATA * pkData = m_pkTagDatas + iTag;
		string strTagName = pkData->szObjectPropName;
		string strDataValue = pkData->szData;

		if (nRet == 0 && pkData->nStatus == 0)
		{
			// json对象! 需要解析;
			Json::Value jsonTagData;
			if (!jsonReader.parse(pkData->szData, jsonTagData, false))
			{
				strDataTime = szCurDateTime;
				strDataValue = "??";
				nDataQuality = -10000;
			}
			else
			{
				if (jsonTagData.isObject()) // 返回的是整个json数值对象;
				{
					strDataTime = Json2String(jsonTagData["t"]);
					strDataValue = Json2String(jsonTagData["v"]);
					nDataQuality = ::atoi(Json2String(jsonTagData["q"]).c_str());
				}
				else // 取得某个域的值，如v的值;
				{
					nDataQuality = pkData->nStatus;
					strDataValue = Json2String(jsonTagData);
				}
			}
		}
		else
		{
			strDataTime = szCurDateTime;
			strDataValue = "???";
			nDataQuality = pkData->nStatus;
			if (nDataQuality == 0)
				pkData->nStatus = nRet;
		}

		// 直接写入数据库（第一次），或更新到数据库中（每分钟）。数据库表的主键：tagname+datatime(YYYY-MM-DD HH)
		BuildInsertOrUpateSQLAndExecute(strTagName, strDataValue, nDataQuality, current_time, DB_TABLENAME_DIFF_HOUR);
		BuildInsertOrUpateSQLAndExecute(strTagName, strDataValue, nDataQuality, current_time, DB_TABLENAME_DIFF_DAY);
		BuildInsertOrUpateSQLAndExecute(strTagName, strDataValue, nDataQuality, current_time, DB_TABLENAME_DIFF_MONTH);
		BuildInsertOrUpateSQLAndExecute(strTagName, strDataValue, nDataQuality, current_time, DB_TABLENAME_DIFF_YEAR);
	} // for (int iTag = 0; iTag < m_nTagDataNum; iTag++)
	return 0;
} // int CHourTimer::handle_timeout

int CHourTimer::LoadBeginRawValueFromDB2Map(string strTableName, map<string, std::pair<string, double> > & mapTagName2RawValue)
{
	mapTagName2RawValue.clear();
	char szSql[4096];
	sprintf(szSql, "select max(calc_time),name, begin_rawvalue,calc_value from %s group by name", strTableName.c_str());
	CMainTask *pMainTask = MAIN_TASK;
	CPKDbApi & pkdb = m_pRule->m_pDbConn->m_pkdb;
	if (!pkdb.IsConnected())
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "规则:%s, 数据库未连接, 将在定时器中检查连接, 这里不做重连操作!", m_pRule->m_strName.c_str());
		return -1;
	}
	string strError;
	vector<vector<string> > vecRows;
	int nRet4 = pkdb.SQLExecute(szSql, vecRows, &strError);
	if (nRet4 != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "规则:%s, SQl Execute Failed:%s, error:%s!", m_pRule->m_strName.c_str(), szSql, strError.c_str());
		return -1;
	}

	for (int iRow = 0; iRow < vecRows.size(); iRow++)
	{
		vector<string> vecRow = vecRows[iRow];
		string strUpdateTime = vecRow[0];
		string strTagName = vecRow[1];
		string strAccuValue = vecRow[2];
		if (strAccuValue.empty())
			continue;
		double dbBeginRawValue = ::atof(strAccuValue.c_str());
		mapTagName2RawValue[strTagName] = std::make_pair(strUpdateTime, dbBeginRawValue);
		vecRow.clear();
	}
	vecRows.clear();
	return 0;
}

// 根据传入的时间，以及数据库表名，计算数据库表t_db_diff_xxxx的calc_time的时间格式
int CHourTimer::GetStatTimeString(string &strStatTimeString, ACE_Time_Value tvCurrentTime, string strTableName)
{
	//string strTime = strCurYYYYMMDDHH + ":00:00";
	//time_t tmTime = PKTimeHelper::String2Time((char *)strTime.c_str());
	//ACE_Time_Value tvTime(tmTime);
	ACE_Time_Value tvSpan(10);//-10秒, 可以认为这个时间才是当前时间的最后1刻
	ACE_Time_Value tvBaseHour = tvCurrentTime - tvSpan;
	time_t tmBaseTime = tvBaseHour.get_msec() / 1000;
	char szBaseTime[32];
	PKTimeHelper::Time2String(szBaseTime, sizeof(szBaseTime), tmBaseTime); // yyyy-mm-dd hh:mm:ss
	strStatTimeString = szBaseTime; 
	GetTimeFormatString(strStatTimeString, szBaseTime, strTableName); // yyyy-mm-dd hh:mm:ss --> yyyy-mm-dd hh
	return 0;
}

map<string, std::pair<string, double> > * CHourTimer::GetMapOfTimeSpan(string strTableName)
{
	map<string, std::pair<string, double> > * pMapTagName2BeginRawValue = NULL;
	if (strTableName.compare(DB_TABLENAME_DIFF_HOUR) == 0)
		pMapTagName2BeginRawValue = &m_mapTagName2BeginRawHourValue;
	else if (strTableName.compare(DB_TABLENAME_DIFF_DAY) == 0)
		pMapTagName2BeginRawValue = &m_mapTagName2BeginRawDayValue;
	else if (strTableName.compare(DB_TABLENAME_DIFF_MONTH) == 0)
		pMapTagName2BeginRawValue = &m_mapTagName2BeginRawMonthValue;
	else if (strTableName.compare(DB_TABLENAME_DIFF_YEAR) == 0)
		pMapTagName2BeginRawValue = &m_mapTagName2BeginRawYearValue;
	return pMapTagName2BeginRawValue;
}

// 得到数据表t_db_diff_xxx的时间对应的开始原始值 begin_rawvalue 
// 1. 先从对应的m_mapTagName2BeginRawXXXXValue 查找，找到则赋值、返回;
// 2. 如果未找到，则从同名数据库表(如t_db_diff_day)中精确查找，找到则赋值、返回;
// 3. 如果未找到，且不是hour表，则从t_db_diff_hour中按照模糊匹配（like 时间段）查找最小的值，找到则赋值、返回;
// 4. 什么都找不到则返回-1
int CHourTimer::GetBeginRawTagValue(double &dbBeginRawValue, bool &bExistInDB, string strTagName, string strBaseYYYYMMMMDDHH, string strTableName)
{
	bExistInDB = false;
	map<string, std::pair<string, double> > * pMapTagName2BeginRawValue = GetMapOfTimeSpan(strTableName);
	map<string, std::pair<string, double> >::iterator itMap = pMapTagName2BeginRawValue->find(strTagName);
	if (itMap != pMapTagName2BeginRawValue->end())
	{
		if (itMap->second.first.compare(strBaseYYYYMMMMDDHH) == 0) // 同一个时间段内，如果已经有了就使用已有的数据
		{
			dbBeginRawValue = itMap->second.second;
			bExistInDB = true;
			return 0;
		}
	}

	// 如果找不到，则从数据库再查询一把看有没有合适的小时值。如果查询成功，则更新m_mapTagName2BeginRawHourValue并返回
	char szSql[4096];
	sprintf(szSql, "select begin_rawvalue,calc_value from %s where name='%s' and calc_time='%s'", strTableName.c_str(), strTagName.c_str(), strBaseYYYYMMMMDDHH.c_str());
	CMainTask *pMainTask = MAIN_TASK;
	CPKDbApi & pkdb = m_pRule->m_pDbConn->m_pkdb;
	if (!pkdb.IsConnected())
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "规则:%s, 数据库未连接, 将在定时器中检查连接, 这里不做重连操作!", m_pRule->m_strName.c_str());
		return -1;
	}
	string strError;
	vector<vector<string> > vecRows;
	int nRet4 = pkdb.SQLExecute(szSql, vecRows, &strError);
	if (nRet4 != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "规则:%s, SQl Execute Failed:%s, error:%s!", m_pRule->m_strName.c_str(), szSql, strError.c_str());
		return -1;
	}

	string strRawValue = "";
	for (int iRow = 0; iRow < vecRows.size(); iRow++)
	{
		vector<string> vecRow = vecRows[iRow];
		strRawValue = vecRow[0];
		break;
	}
	vecRows.clear();	

	if (!strRawValue.empty()) // 找到了;
	{
		dbBeginRawValue = ::atof(strRawValue.c_str());
		(*pMapTagName2BeginRawValue)[strTagName] = std::make_pair(strBaseYYYYMMMMDDHH, dbBeginRawValue);
		bExistInDB = true;
		return 0;
	}

	// 小时表不需要在找了，找不到就结束了;
	if (strTableName.compare(DB_TABLENAME_DIFF_HOUR) == 0)
	{
		//dbBeginRawValue = 0;	//如果都找不到，则设置初始值为0; 1127
		return -10;
	}


	// 其他日、月、年还是从小时表中取得最新的时间的数据.如2019-11-26，则查询条件为 like 2019-11, 并取最小值
	sprintf(szSql, "select begin_rawvalue,calc_value,min(calc_time) from %s where name='%s' and calc_time like '%s%%'", DB_TABLENAME_DIFF_HOUR, strTagName.c_str(), strBaseYYYYMMMMDDHH.c_str());
	int nRet5 = pkdb.SQLExecute(szSql, vecRows, &strError);
	if (nRet5 != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "规则:%s, SQl Execute Failed:%s, error:%s!", m_pRule->m_strName.c_str(), szSql, strError.c_str());
		return -1;
	}

	strRawValue = "";
	for (int iRow = 0; iRow < vecRows.size(); iRow++)
	{
		vector<string> vecRow = vecRows[iRow];
		strRawValue = vecRow[0];
		break;
	}
	vecRows.clear();

	if (!strRawValue.empty()) // 找到了！
	{
		dbBeginRawValue = ::atof(strRawValue.c_str());
		// (*pMapTagName2BeginRawValue)[strTagName] = std::make_pair(strBaseYYYYMMMMDDHH, dbBeginRawValue);
		bExistInDB = false; // 同名数据库表中不存在该条记录
		return 0;
	}

	return -1;
}

int CHourTimer::GetTimeFormatString(string &strFormatTimeString, string strRawTimeString, string strTableName)
{
	if (strTableName.compare(DB_TABLENAME_DIFF_HOUR) == 0)
		strFormatTimeString = SubString(strRawTimeString, TIMELEN_YYYYMMDD_HH);
	else if (strTableName.compare(DB_TABLENAME_DIFF_DAY) == 0)
		strFormatTimeString = SubString(strRawTimeString, TIMELEN_YYYYMMDD);
	else if (strTableName.compare(DB_TABLENAME_DIFF_MONTH) == 0)
		strFormatTimeString = SubString(strRawTimeString, TIMELEN_YYYYMM);
	else if (strTableName.compare(DB_TABLENAME_DIFF_YEAR) == 0)
		strFormatTimeString = SubString(strRawTimeString, TIMELEN_YYYY);
	return 0;
}

int CHourTimer::BuildInsertOrUpateSQLAndExecute(string strTagName, string strCurTagRawValue, int nDataQuality, ACE_Time_Value tvCurrentTime, string strTableName)
{
	if (nDataQuality != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "BuildInsertOrUpateSQLAndExecute failed, tag:%s, value:%s, QUALITY:%d <> 0", strTagName.c_str(), strCurTagRawValue.c_str(), nDataQuality);
		return -1;
	}

	double dbCurAccuValue = ::atof(strCurTagRawValue.c_str());
	string strError;
	double dbBeginRawValue;
	string strHourValue;
	char szCurYYYYMMDDHHMMSS[32];
	PKTimeHelper::Time2String(szCurYYYYMMDDHHMMSS, sizeof(szCurYYYYMMDDHHMMSS), tvCurrentTime.get_msec() / 1000);
	string strCurYYYYMMDDHH = "";
	GetTimeFormatString(strCurYYYYMMDDHH, szCurYYYYMMDDHHMMSS, strTableName);

	string strBeginRawValue = "";
	string strBaseHourYYYYMMMMDDHH;
	GetStatTimeString(strBaseHourYYYYMMMMDDHH, tvCurrentTime, strTableName);
	bool bExistInDB = true; // 是否在数据库中存在该条记录;
	int nRet = GetBeginRawTagValue(dbBeginRawValue, bExistInDB, strTagName, strBaseHourYYYYMMMMDDHH, strTableName);
	if (nRet == 0) // 得到上次的值则计算差值，否则不计算差值;
	{
		char szTmp[32];
		sprintf(szTmp, "%.2f", dbCurAccuValue);
		//strBeginRawValue = szTmp;
		strBeginRawValue = szTmp;	//初始时设置为实际值;

		double dbHourSpanValue = dbCurAccuValue - dbBeginRawValue;
		sprintf(szTmp, "%.2f", dbHourSpanValue);
		strHourValue = szTmp;
	}
	
	if (nRet == -10)
	{
		dbBeginRawValue = dbCurAccuValue;

		char szTmp[32];
		sprintf(szTmp, "%.2f", dbCurAccuValue);
		strBeginRawValue = szTmp;	//初始时设置为实际值;
		double dbHourSpanValue = dbCurAccuValue - dbBeginRawValue;
		sprintf(szTmp, "%.2f", dbHourSpanValue);
		strHourValue = szTmp;
	}

	char szSql[4096];
	if (!bExistInDB)
	{
		sprintf(szSql, "insert into %s(name, calc_time, calc_value, begin_rawvalue, end_rawvalue, create_time, update_time) values('%s','%s','%s', '%s','%s','%s','%s')",
			strTableName.c_str(), strTagName.c_str(), strBaseHourYYYYMMMMDDHH.c_str(), strHourValue.c_str(), strBeginRawValue.c_str(), strCurTagRawValue.c_str(), szCurYYYYMMDDHHMMSS, szCurYYYYMMDDHHMMSS);
	}
	else
	{
		sprintf(szSql, "update %s set calc_value=%s, end_rawvalue='%s',update_time='%s' where name='%s' and calc_time='%s'",
			strTableName.c_str(), strHourValue.c_str(), strCurTagRawValue.c_str(), szCurYYYYMMDDHHMMSS, strTagName.c_str(), strBaseHourYYYYMMMMDDHH.c_str());
	}

	CMainTask *pMainTask = MAIN_TASK;
	CPKDbApi & pkdb = m_pRule->m_pDbConn->m_pkdb;
	if (!pkdb.IsConnected())
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "规则:%s, 数据库未连接, 将在定时器中检查连接, 这里不做重连操作!", m_pRule->m_strName.c_str());
		return -1;
	}

	int nRet4 = pkdb.SQLExecute(szSql, &strError);
	if (nRet4 != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "规则:%s, SQl Execute Failed:%s, error:%s!", m_pRule->m_strName.c_str(), szSql, strError.c_str());
	}
	else
	{
		if (!bExistInDB) // 只有插入成功则需要更新值;
		{
			map<string, std::pair<string, double> > * pMapTagName2BeginRawValue = GetMapOfTimeSpan(strTableName);
			(*pMapTagName2BeginRawValue)[strTagName] = std::make_pair(strBaseHourYYYYMMMMDDHH, dbCurAccuValue);
		}
	}
	return 0;
}
