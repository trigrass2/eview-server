#include <string>
#include "RuleTimer.h"
#include "TimerTask.h"
#include "pklog/pklog.h"
#include "json/json.h"
#include "pkdbapi/pkdbapi.h"
#include <algorithm>

#include "MainTask.h"
extern CPKLog g_logger;
using namespace std;

string Json2String(Json::Value &jsonVal)
{
	if (jsonVal.isNull())
		return "";

	string strRet = "";
	if (jsonVal.isString())
	{
		strRet = jsonVal.asString();
		return strRet;
	}

	if (jsonVal.isInt())
	{
		int nVal = jsonVal.asInt();
		char szTmp[32];
		sprintf(szTmp, "%d", nVal);
		strRet = szTmp;
		return strRet;
	}

	if (jsonVal.isDouble())
	{
		double dbVal = jsonVal.asDouble();
		char szTmp[32];
		sprintf(szTmp, "%f", dbVal);
		strRet = szTmp;
		return strRet;
	}

	if (jsonVal.isBool())
	{
		bool bVal = jsonVal.asBool();
		char szTmp[32];
		sprintf(szTmp, "%d", bVal ? 1 : 0);
		strRet = szTmp;
		return strRet;
	}

	return "";
}

/**
*  构造函数.
*
*
*  @version     05/30/2008  lijingjing  Initial Version.
*/
CRuleTimer::CRuleTimer(CTimerTask *pTimerTask) :m_pTimerTask(pTimerTask)
{
	m_pRule = NULL;
	m_nPeriodSec = 60 * 60; 
	m_bFirstInsert = true;
}

/**
*  析构函数.
*
*
*  @version     05/30/2008  lijingjing  Initial Version.
*/
CRuleTimer::~CRuleTimer()
{
	//m_vecTagName.clear();
	//m_vecObjectName.clear();
	//m_vecPropName.clear();
}

void CRuleTimer::StartTimer()
{
	// 组织每次定时器要访问的数据
	int nTagDataNum = m_pRule->m_vecColPropConfig.size() + m_pRule->m_vecColTagConfig.size();
	if (nTagDataNum <= 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "定时器保存规则:%s, 表名:%s, 周期:%d 秒, 数据点个数:%d <= 0 ! 将不开启该定时器转存!", m_pRule->m_strName.c_str(), m_pRule->m_strTableName.c_str(), m_nPeriodSec, nTagDataNum);
		return;
	}

	g_logger.LogMessage(PK_LOGLEVEL_INFO, "定时器保存规则:%s, 表名:%s, 周期:%d 秒, 数据点个数:%d > 0 ! 将开启该定时器转存!", m_pRule->m_strName.c_str(), m_pRule->m_strTableName.c_str(), m_nPeriodSec, nTagDataNum);

	for (int i = 0; i < m_pRule->m_vecObjProps.size(); i++)
	{
		CDBTransferRuleObjProps *pObjProps = m_pRule->m_vecObjProps[i];
		pObjProps->InitPKData();
	}

	for (int i = 0; i < m_pRule->m_vecTags.size(); i++)
	{
		CDBTransferRuleTags *pTags = m_pRule->m_vecTags[i];
		pTags->InitPKData();
	}

	if (m_nPeriodSec > 0)
	{
		ACE_Time_Value	tvPollRate;		// 扫描周期，单位ms
		tvPollRate.msec(m_nPeriodSec * 1000);
		ACE_Time_Value tvPollDelay(0); // 1970-1-1 00:00:00
		m_pTimerTask->m_pReactor->schedule_timer((ACE_Event_Handler *)(CRuleTimer *)this, NULL, tvPollDelay, tvPollRate);
	}
}

void CRuleTimer::StopTimer()
{
	m_pTimerTask->m_pReactor->cancel_timer((ACE_Event_Handler *)(CRuleTimer *)this);
}

// 更新SQL是用于实时表，历史数据不更新
// 设备模式下,形成结果： update %s set %s='%s',%s='%s',%s=%s,%s='%s' where %s='%s' and %s='%s'
int CRuleTimer::GetUpdateTagsSQL(char *szSQL, int nSQLBuffLen, vector<CColValue> &vecColAndValue, vector<CColInfo *> &vecColInfo, const char *szTagTime, int nQuality, char *szCurDateTime)
{
	sprintf(szSQL, "update %s set ", m_pRule->m_strTableName.c_str());
	char szAssign[512] = { 0 }, szWhere[512] = { 0 };
	char szTmp[256] = { 0 };
	bool bFirstFiled = true;

	if (vecColAndValue.size() <= 0 || vecColInfo.size() != vecColInfo.size())
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "CRuleTimer::GetUpdateSQL, no-multitag mode, col count = 0");
		return -1;
	}

	// 如果配置了TAG名称
	if (m_pRule->m_colTagName.m_strColName.length() > 0 && vecColAndValue.size() > 0)
	{
		string strTagName1 = vecColAndValue[0].m_strTagOrPropName; // 仅仅取第一个tag点名称作为字段的值
		if (bFirstFiled)
		{
			sprintf(szTmp, " %s='%s'", m_pRule->m_colTagName.m_strColName.c_str(), szTagTime);
			bFirstFiled = false;
		}
		else
			sprintf(szTmp, ",%s='%s'", m_pRule->m_colTagName.m_strColName.c_str(), szTagTime);
		strcat(szSQL, szTmp);
	}

	for (int iCol = 0; iCol < vecColAndValue.size(); iCol++)
	{
		string strTagFieldName = vecColInfo[iCol]->m_strColName.c_str();
		int nTagDataType = vecColInfo[iCol]->m_nDataType;
		string strTagValue; // 单点记录模式下取第一个元素
		strTagValue = vecColAndValue[iCol].m_strTagValue; // 单点记录模式下取第一个元素

		CColValue *pColInfo = &vecColAndValue[iCol];
		if (bFirstFiled)
		{
			sprintf(szTmp, " %s='%s'", strTagFieldName.c_str(), strTagValue.c_str());
			bFirstFiled = false;
		}
		else
			sprintf(szTmp, ",%s='%s'", strTagFieldName.c_str(), strTagValue.c_str());
		strcat(szSQL, szTmp);
	}

	if (m_pRule->m_colDateTime.m_strColName.length() > 0)
	{
		if (bFirstFiled)
		{
			sprintf(szTmp, " %s='%s'", m_pRule->m_colDateTime.m_strColName.c_str(), szTagTime);
			bFirstFiled = false;
		}
		else
			sprintf(szTmp, ",%s='%s'", m_pRule->m_colDateTime.m_strColName.c_str(), szTagTime);
		strcat(szSQL, szTmp);
	}

	if (m_pRule->m_colQuality.m_strColName.length() > 0)
	{
		if (bFirstFiled)
		{
			if (m_pRule->m_colQuality.m_nDataType == COL_DATATYPE_ID_STRING)// 字符串类型字段
				sprintf(szTmp, " %s='%d'", m_pRule->m_colQuality.m_strColName.c_str(), nQuality);
			else
				sprintf(szTmp, " %s=%d", m_pRule->m_colQuality.m_strColName.c_str(), nQuality); // 数值类型字段
			bFirstFiled = false;
		}
		else
			if (m_pRule->m_colQuality.m_nDataType == COL_DATATYPE_ID_STRING)// 字符串类型字段
				sprintf(szTmp, ",%s='%d'", m_pRule->m_colQuality.m_strColName.c_str(), nQuality);
			else
				sprintf(szTmp, ",%s=%d", m_pRule->m_colQuality.m_strColName.c_str(), nQuality);

		strcat(szSQL, szTmp);
	}

	if (m_pRule->m_colUpdateTime.m_strColName.length() > 0)
	{
		if (bFirstFiled)
		{
			sprintf(szTmp, " %s='%s'", m_pRule->m_colUpdateTime.m_strColName.c_str(), szCurDateTime);
			bFirstFiled = false;
		}
		else
			sprintf(szTmp, ",%s='%s'", m_pRule->m_colUpdateTime.m_strColName.c_str(), szCurDateTime);
		strcat(szSQL, szTmp);
	}

	// 如果配置固定要写入的属性;
	for (int iCol = 0; iCol < m_pRule->m_vecFixCol.size(); iCol++)
	{
		CFixCol *pFixCol = m_pRule->m_vecFixCol[iCol];
		if (bFirstFiled)
		{
			sprintf(szTmp, " %s='%s'", pFixCol->m_strColName.c_str(), pFixCol->m_strColValue.c_str());
			bFirstFiled = false;
		}
		else
			sprintf(szTmp, ",%s='%s'", pFixCol->m_strColName.c_str(), pFixCol->m_strColValue.c_str());
		strcat(szSQL, szTmp);
	}

	//if (!m_pRule->m_bMultiTagMode)
	//{
	//	string strTagName = vecColAndValue[0]->m_strColName.c_str();
	//	string strColName1 = vecColAndValue[0]->m_strColName;
	//	sprintf(szTmp, " where %s='%s'", strColName1.c_str(), strTagName.c_str());
	//}

	strcat(szSQL, szTmp);
	return 0;
}

// 更新SQL是用于实时表，历史数据不更新
// 设备模式下,形成结果： update %s set %s='%s',%s='%s',%s=%s,%s='%s' where %s='%s' and %s='%s'
int CRuleTimer::GetUpdateObjectSQL(char *szSQL, int nSQLBuffLen, const char *szObjectName, vector<CColValue> &vecColAndValue, vector<CColInfo *> &vecColInfo, const char *szTagTime, int nQuality, char *szCurDateTime)
{
	if (strlen(szObjectName) <= 0 || vecColAndValue.size() <= 0 || vecColInfo.size() != vecColAndValue.size())
		return -1;

	sprintf(szSQL, "update %s set ", m_pRule->m_strTableName.c_str());
	char szTmp[256] = { 0 };
	bool bFirstFiled = true;
	string strPropName = "";
	for (int iCol = 0; iCol < vecColAndValue.size(); iCol++)
	{
		string strTagFieldName = vecColInfo[iCol]->m_strColName.c_str();
		int nTagDataType = vecColInfo[iCol]->m_nDataType;
		string strTagValue = "";
		strTagValue = vecColAndValue[iCol].m_strTagValue; // 单点记录模式下取第一个元素

		CColValue *pColValue = &vecColAndValue[iCol];
		CColInfo *pColInfo = vecColInfo[iCol];

		if (bFirstFiled)
		{
			sprintf(szTmp, " %s='%s'", strTagFieldName.c_str(), strTagValue.c_str());
			bFirstFiled = false;
		}
		else
			sprintf(szTmp, ",%s='%s'", strTagFieldName.c_str(), strTagValue.c_str());
		strcat(szSQL, szTmp);
	}

	if (m_pRule->m_colDateTime.m_strColName.length() > 0)
	{
		if (bFirstFiled)
		{
			sprintf(szTmp, " %s='%s'", m_pRule->m_colDateTime.m_strColName.c_str(), szTagTime);
			bFirstFiled = false;
		}
		else
			sprintf(szTmp, ",%s='%s'", m_pRule->m_colDateTime.m_strColName.c_str(), szTagTime);
		strcat(szSQL, szTmp);
	}

	if (m_pRule->m_colQuality.m_strColName.length() > 0)
	{
		if (bFirstFiled)
		{
			if (m_pRule->m_colQuality.m_nDataType == COL_DATATYPE_ID_STRING)// 字符串类型字段
				sprintf(szTmp, " %s='%d'", m_pRule->m_colQuality.m_strColName.c_str(), nQuality);
			else
				sprintf(szTmp, " %s=%d", m_pRule->m_colQuality.m_strColName.c_str(), nQuality);
			bFirstFiled = false;
		}
		else
		{
			if (m_pRule->m_colQuality.m_nDataType == COL_DATATYPE_ID_STRING)// 字符串类型字段
				sprintf(szTmp, ",%s='%d'", m_pRule->m_colQuality.m_strColName.c_str(), nQuality);
			else
				sprintf(szTmp, ",%s=%d", m_pRule->m_colQuality.m_strColName.c_str(), nQuality);

		}
		strcat(szSQL, szTmp);
	}

	if (m_pRule->m_colUpdateTime.m_strColName.length() > 0)
	{
		if (bFirstFiled)
		{
			sprintf(szTmp, " %s='%s'", m_pRule->m_colUpdateTime.m_strColName.c_str(), szCurDateTime);
			bFirstFiled = false;
		}
		else
			sprintf(szTmp, ",%s='%s'", m_pRule->m_colUpdateTime.m_strColName.c_str(), szCurDateTime);
		strcat(szSQL, szTmp);
	}

	// 如果配置固定要写入的属性
	for (int iCol = 0; iCol < m_pRule->m_vecFixCol.size(); iCol++)
	{
		CFixCol *pFixCol = m_pRule->m_vecFixCol[iCol];
		if (bFirstFiled)
		{
			sprintf(szTmp, " %s='%s'", pFixCol->m_strColName.c_str(), pFixCol->m_strColValue.c_str());
			bFirstFiled = false;
		}
		else
			sprintf(szTmp, ",%s='%s'", pFixCol->m_strColName.c_str(), pFixCol->m_strColValue.c_str());
		strcat(szSQL, szTmp);
	}

	string strColName1 = vecColInfo[0]->m_strColName.c_str();
	string strPropName1 = vecColInfo[0]->m_strColName;
	sprintf(szTmp, " where %s='%s' and %s='%s'", m_pRule->m_colObjName.m_strColName.c_str(), szObjectName, strPropName1.c_str(), strColName1.c_str());

	strcat(szSQL, szTmp);
	return 0;
}

// MYSQL, date, time, datetime类型用字符串都可以插入： insert into datetimetest(date1,time1,datetime1) values('2019-05-06 12:22:22.234','2019-05-06 12:22:22.234','2019-05-06 12:22:22.234')
int CRuleTimer::BuildValueWithFormat(char *szDbFieldValueExpr, int nFieldValueBuffLen, int nDataType, string strValue, int nQuality, bool bInsertComma)
{
	szDbFieldValueExpr[0] = 0;
	if (nDataType == COL_DATATYPE_ID_STRING)
	{
		if (nQuality != 0) // 质量bad时的值
			strValue = MAIN_TASK->m_strValueOnBadQuality_String;

		if(bInsertComma)
			PKStringHelper::Snprintf(szDbFieldValueExpr, nFieldValueBuffLen, ",'%s'", strValue.c_str());
		else
			PKStringHelper::Snprintf(szDbFieldValueExpr, nFieldValueBuffLen, "'%s'", strValue.c_str());
	}
	else if (nDataType == COL_DATATYPE_ID_INT)
	{
		if (nQuality != 0) // 质量bad时的值
			strValue = MAIN_TASK->m_strValueOnBadQuality_Int;
		int nInt = ::atoi(strValue.c_str()); // 避免出现 ??? 无法插入的问题，所以要做一次转换
		if (bInsertComma)
			PKStringHelper::Snprintf(szDbFieldValueExpr, nFieldValueBuffLen, ",%d", nInt);
		else
			PKStringHelper::Snprintf(szDbFieldValueExpr, nFieldValueBuffLen, "%d", nInt);
	}
	else if (nDataType == COL_DATATYPE_ID_REAL)
	{
		if (nQuality != 0) // 质量bad时的值
			strValue = MAIN_TASK->m_strValueOnBadQuality_Real;
		double dbData = ::atof(strValue.c_str()); // 避免出现 ??? 无法插入的问题，所以要做一次转换
		if (bInsertComma)
			PKStringHelper::Snprintf(szDbFieldValueExpr, nFieldValueBuffLen, ",%f", dbData);
		else
			PKStringHelper::Snprintf(szDbFieldValueExpr, nFieldValueBuffLen, "%f", dbData);
	}
	else if (nDataType == COL_DATATYPE_ID_DATETIME)
	{
		if (nQuality != 0) // 质量bad时的值
			strValue = MAIN_TASK->m_strValueOnBadQuality_DateTime;
		if(strValue.length() > 19) // 去掉毫秒
			strValue = strValue.substr(0, 19); 

		CPKDbApi & pkdb = m_pRule->m_pDbConn->m_pkdb;
		int nDataBaseTypeId = pkdb.GetDatabaseTypeId();
		char szDbSpecific[128] = { 0 };
		if (nDataBaseTypeId == PK_DATABASE_TYPEID_ORACLE)
			PKStringHelper::Snprintf(szDbSpecific, sizeof(szDbSpecific), "to_date('%s','YYYY-MM-DD HH24:MI:SS')", strValue.c_str());
		else if (nDataBaseTypeId == PK_DATABASE_TYPEID_MYSQL) // DATE, TIME, TIMESTAMP, DATETIME类型可以直接插入字符串，会自动识别为数据库
			PKStringHelper::Snprintf(szDbSpecific, sizeof(szDbSpecific), "'%s'", strValue.c_str());
		else if (nDataBaseTypeId == PK_DATABASE_TYPEID_SQLITE) // 就是字符串类型
			PKStringHelper::Snprintf(szDbSpecific, sizeof(szDbSpecific), "'%s'", strValue.c_str());
		else if (nDataBaseTypeId == PK_DATABASE_TYPEID_SQLSERVER)
			PKStringHelper::Snprintf(szDbSpecific, sizeof(szDbSpecific), "to_date('%s','YYYY-MM-DD HH24:MI:SS')", strValue.c_str());
		else
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "BuildValueWithFormat, unknown database typeid:%d, date time assume string", nDataBaseTypeId);
			PKStringHelper::Snprintf(szDbSpecific, sizeof(szDbSpecific), "'%s'", strValue.c_str());  // 字符串
		}

		if (bInsertComma)
			strcpy(szDbFieldValueExpr, ",");
		
		strcat(szDbFieldValueExpr, szDbSpecific);
	}
	else
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "BuildValueWithFormat, unknown colid:%d, assume is string", nDataType);
		if (bInsertComma)
			strcpy(szDbFieldValueExpr, ",'");

		strcat(szDbFieldValueExpr, strValue.c_str());
		strcat(szDbFieldValueExpr, "'");
	}
	return 0;
}

// 设备模式下,形成结果; sprintf(szSql, "insert into %s(%s,%s,%s,%s,%s,%s,%s) values('%s','%s','%s','%s','%s',%s,'%s')", m_pRule->m_strTableName.c_str(),
// m_strFieldName_ObjName.c_str(), m_strFieldName_PropName.c_str(), m_strFieldName_DataTime.c_str(), m_strFieldName_Value.c_str(), m_strFieldName_Quality.c_str(), m_strFieldName_UpdateTime.c_str(),
// strObjName.c_str(), strPropName.c_str(), strDataTime.c_str(), strDataValue.c_str(), nDataQuality, szCurDateTime);
int CRuleTimer::GetInsertObjectSQL(char *szSQL, int nSQLBuffLen, const char *szObjectName, vector<CColValue> &vecCols, vector<CColInfo *> &vecColInfo, const char *szTagTime, int nQuality, char *szCurDateTime)
{
	char szColNames[512] = { 0 }, szColValues[512] = { 0 };
	char szColName[128] = { 0 }, szColValue[128] = { 0 };
	bool bFirstFiled = true;
	//if (vecCols.size() <= 0 || vecColInfo.size() != vecCols.size())
	//	return -1;

	if (vecCols.size() <= 0)
		return -1;

	// 如果配置了对象名称;
	if (m_pRule->m_colObjName.m_strColName.length() > 0)
	{
		if (bFirstFiled)
		{
			sprintf(szColName, "%s", m_pRule->m_colObjName.m_strColName.c_str());
			sprintf(szColValue, "'%s'", szObjectName);
			bFirstFiled = false;
		}
		else
		{
			sprintf(szColName, ",%s", m_pRule->m_colObjName.m_strColName.c_str());
			sprintf(szColValue, ",'%s'", szObjectName);
		}
		strcat(szColNames, szColName);
		strcat(szColValues, szColValue);
	}

	// 如果配置了对象的属性，一个或多个;
	for(int iProp = 0; iProp < vecCols.size(); iProp ++)
	{
		CColValue *pColValue = &vecCols[iProp];
		CColInfo *pColInfo = vecColInfo[iProp];
		if (bFirstFiled)
		{
			sprintf(szColName, "%s", pColInfo->m_strColName.c_str());
			BuildValueWithFormat(szColValue, sizeof(szColValue), pColInfo->m_nDataType, pColValue->m_strTagValue, pColValue->m_nQuality, false);
			// sprintf(szColValue, "'%s'", pColInfo->m_strTagValue.c_str());
			bFirstFiled = false;
		}
		else
		{
			sprintf(szColName, ",%s", pColInfo->m_strColName.c_str());
			// sprintf(szColValue, "'%s'", pColInfo->m_strTagValue.c_str());
			BuildValueWithFormat(szColValue, sizeof(szColValue), pColInfo->m_nDataType, pColValue->m_strTagValue, pColValue->m_nQuality, true);
		}
		strcat(szColNames, szColName);
		strcat(szColValues, szColValue);
	}

	if (m_pRule->m_colQuality.m_strColName.length() > 0) // 质量一定是数值型！！;
	{
		if (bFirstFiled)
		{
			sprintf(szColName, "%s", m_pRule->m_colQuality.m_strColName.c_str());
			if (m_pRule->m_colQuality.m_nDataType == COL_DATATYPE_ID_STRING)// 字符串类型字段;
				sprintf(szColValue, "'%d'", nQuality);
			else
				sprintf(szColValue, "%d", nQuality);

			bFirstFiled = false;
		}
		else
		{
			sprintf(szColName, ",%s", m_pRule->m_colQuality.m_strColName.c_str());
			if (m_pRule->m_colQuality.m_nDataType == COL_DATATYPE_ID_STRING)// 字符串类型字段;
				sprintf(szColValue, ",'%d'", nQuality);
			else
				sprintf(szColValue, ",%d", nQuality);

		}
		strcat(szColNames, szColName);
		strcat(szColValues, szColValue);
	}

	if (m_pRule->m_colDateTime.m_strColName.length() > 0) // 具有日期字段;
	{
		string strTagTime = szTagTime;
		if (bFirstFiled)
		{
			sprintf(szColName, "'%s'", szTagTime);
			BuildValueWithFormat(szColValue, sizeof(szColValue), m_pRule->m_colDateTime.m_nDataType, strTagTime.c_str(), 0, false);
			//sprintf(szColValue, "%s", m_pRule->m_strFieldName_DataTime.c_str());
			bFirstFiled = false;
		}
		else
		{
			sprintf(szColName, ",%s", m_pRule->m_colDateTime.m_strColName.c_str());
			BuildValueWithFormat(szColValue, sizeof(szColValue), m_pRule->m_colDateTime.m_nDataType, strTagTime.c_str(), 0, true);
			//sprintf(szColValue, ",'%s'", szTagTime);
		}
		strcat(szColNames, szColName);
		strcat(szColValues, szColValue);
	}

	if (m_pRule->m_colUpdateTime.m_strColName.length() > 0) // 具有更新时间字段，当前时间字段一定有有效值，质量为0;
	{
		string strCurDateTime = szCurDateTime;
		if (bFirstFiled)
		{
			sprintf(szColName, "%s", m_pRule->m_colUpdateTime.m_strColName.c_str());
			BuildValueWithFormat(szColValue, sizeof(szColValue), m_pRule->m_colUpdateTime.m_nDataType, strCurDateTime, 0, false);
			//sprintf(szColValue, "'%s'", szCurDateTime);
			bFirstFiled = false;
		}
		else
		{
			sprintf(szColName, ",%s", m_pRule->m_colUpdateTime.m_strColName.c_str());
			BuildValueWithFormat(szColValue, sizeof(szColValue), m_pRule->m_colUpdateTime.m_nDataType, strCurDateTime, 0, true);
			//sprintf(szColValue, ",'%s'", szCurDateTime);
		}
		strcat(szColNames, szColName);
		strcat(szColValues, szColValue);
	}

	// 如果配置固定要写入的属性;
	for (int iCol = 0; iCol < m_pRule->m_vecFixCol.size(); iCol++)
	{
		CFixCol *pFixCol = m_pRule->m_vecFixCol[iCol];

		if (bFirstFiled)
		{
			sprintf(szColName, "%s", pFixCol->m_strColName.c_str());
			BuildValueWithFormat(szColValue, sizeof(szColValue), pFixCol->m_nDataType, pFixCol->m_strColValue, 0, false);
			//sprintf(szColValue, "'%s'", pFixCol->m_strColValue.c_str());
			bFirstFiled = false;
		}
		else
		{
			sprintf(szColName, ",%s", pFixCol->m_strColName.c_str());
			BuildValueWithFormat(szColValue, sizeof(szColValue), pFixCol->m_nDataType, pFixCol->m_strColValue, 0, true);
			//sprintf(szColValue, ",'%s'", pFixCol->m_strColValue.c_str());
		}
		strcat(szColNames, szColName);
		strcat(szColValues, szColValue);
	}

	sprintf(szSQL, "insert into %s(%s) values(%s)", m_pRule->m_strTableName.c_str(), szColNames, szColValues);
	return 0;
}

// 设备模式下,形成结果： update %s set %s='%s',%s='%s',%s=%s,%s='%s' where %s='%s' and %s='%s'
int CRuleTimer::GetInsertTagsSQL(char *szSQL, int nSQLBuffLen, vector<CColValue> &vecColValue, vector<CColInfo *> &vecColInfo, const char *szTagTime, int nQuality, char *szCurDateTime)
{
	char szColNames[512] = { 0 }, szColValues[512] = { 0 };
	char szColName[512] = { 0 }, szColValue[512] = { 0 };
	bool bFirstFiled = true;
	//if (vecColValue.size() <= 0 || vecColInfo.size() != vecColValue.size())
	//	return -1;

	// 如果配置了TAG名称
	if (m_pRule->m_colTagName.m_strColName.length() > 0 && vecColValue.size() > 0)
	{
		string strTagName1 = vecColValue[0].m_strTagOrPropName; // 仅仅取第一个tag点名称作为字段的值
		if (bFirstFiled)
		{
			sprintf(szColName, "%s", m_pRule->m_colTagName.m_strColName.c_str());
			sprintf(szColValue, "'%s'", strTagName1.c_str());
			bFirstFiled = false;
		}
		else
		{
			sprintf(szColName, ",%s", m_pRule->m_colObjName.m_strColName.c_str());
			sprintf(szColValue, ",'%s'", strTagName1.c_str());
		}
		strcat(szColNames, szColName);
		strcat(szColValues, szColValue);
	}

	// 如果配置了多个TAG点，一个或多个
	for (int iProp = 0; iProp < vecColValue.size(); iProp++)
	{
		CColValue *pColValue = &vecColValue[iProp];
		CColInfo *pColInfo = vecColInfo[iProp];

		if (bFirstFiled)
		{
			sprintf(szColName, "%s", pColInfo->m_strColName.c_str());
			BuildValueWithFormat(szColValue, sizeof(szColValue), pColInfo->m_nDataType, pColValue->m_strTagValue, pColValue->m_nQuality, false);
			//sprintf(szColValue, "'%s'", pColInfo->m_strTagValue.c_str());

			bFirstFiled = false;
		}
		else
		{
			sprintf(szColName, ",%s", pColInfo->m_strColName.c_str());
			BuildValueWithFormat(szColValue, sizeof(szColValue), pColInfo->m_nDataType, pColValue->m_strTagValue, pColValue->m_nQuality, true);
			// sprintf(szColValue, ",'%s'", pColInfo->m_strTagValue.c_str());
		}
		strcat(szColNames, szColName);
		strcat(szColValues, szColValue);
	}

	// 有tagvalue字段，那么写入值
	if (vecColValue.size() > 0)
	{
		if (bFirstFiled)
		{
			sprintf(szColName, "%s", vecColInfo[0]->m_strColName.c_str());
			BuildValueWithFormat(szColValue, sizeof(szColValue), vecColInfo[0]->m_nDataType, vecColValue[0].m_strTagValue, vecColValue[0].m_nQuality, false);
			//sprintf(szColValue, "'%s'", vecCols[0]->m_strTagValue.c_str());
			bFirstFiled = false;
		}
		else
		{
			sprintf(szColName, ",%s", vecColInfo[1]->m_strColName.c_str());
			BuildValueWithFormat(szColValue, sizeof(szColValue), vecColInfo[0]->m_nDataType, vecColValue[0].m_strTagValue, vecColValue[0].m_nQuality, true);
			//sprintf(szColValue, ",'%s'", vecCols[0]->m_strTagValue.c_str());
		}
		strcat(szColNames, szColName);
		strcat(szColValues, szColValue);
	}

	if (m_pRule->m_colDateTime.m_strColName.length() > 0) // 日期
	{
		string strDateTime = szTagTime;
		if (bFirstFiled)
		{
			sprintf(szColName, "%s", m_pRule->m_colDateTime.m_strColName.c_str());
			BuildValueWithFormat(szColValue, sizeof(szColValue), m_pRule->m_colDateTime.m_nDataType, strDateTime, 0, false);
			//sprintf(szColValue, "'%s'", szTagTime);
			bFirstFiled = false;
		}
		else
		{
			sprintf(szColName, ",%s", m_pRule->m_colDateTime.m_strColName.c_str());
			BuildValueWithFormat(szColValue, sizeof(szColValue), m_pRule->m_colDateTime.m_nDataType, strDateTime, 0, true);
			// sprintf(szColValue, ",'%s'", szTagTime);
		}
		strcat(szColNames, szColName);
		strcat(szColValues, szColValue);
	}

	if (m_pRule->m_colQuality.m_strColName.length() > 0)
	{
		if (bFirstFiled)
		{
			sprintf(szColName, "%s", m_pRule->m_colQuality.m_strColName.c_str());
			if (m_pRule->m_colQuality.m_nDataType == COL_DATATYPE_ID_STRING)// 字符串类型字段
				sprintf(szColValue, "'%d'", nQuality);
			else
				sprintf(szColValue, "%d", nQuality);
			bFirstFiled = false;
		}
		else
		{
			sprintf(szColName, ",%s", m_pRule->m_colQuality.m_strColName.c_str());
			if (m_pRule->m_colQuality.m_nDataType == COL_DATATYPE_ID_STRING)// 字符串类型字段
				sprintf(szColValue, ",'%d'", nQuality);
			else
			sprintf(szColValue, ",%d", nQuality);
		}
		strcat(szColNames, szColName);
		strcat(szColValues, szColValue);
	}

	if (m_pRule->m_colUpdateTime.m_strColName.length() > 0)
	{
		string strCurDateTime = szCurDateTime;
		if (bFirstFiled)
		{
			sprintf(szColName, "%s", m_pRule->m_colUpdateTime.m_strColName.c_str());
			BuildValueWithFormat(szColValue, sizeof(szColValue), m_pRule->m_colUpdateTime.m_nDataType, strCurDateTime, 0, false);
			//sprintf(szColValue, "'%s'", szCurDateTime);
			bFirstFiled = false;
		}
		else
		{
			sprintf(szColName, ",%s", m_pRule->m_colUpdateTime.m_strColName.c_str());
			BuildValueWithFormat(szColValue, sizeof(szColValue), m_pRule->m_colUpdateTime.m_nDataType, strCurDateTime, 0, true);
			//sprintf(szColValue, ",'%s'", szCurDateTime);
		}
		strcat(szColNames, szColName);
		strcat(szColValues, szColValue);
	}

	// 如果配置固定要写入的属性;
	for (int iCol = 0; iCol < m_pRule->m_vecFixCol.size(); iCol++)
	{
		CFixCol *pFixCol = m_pRule->m_vecFixCol[iCol];

		if (bFirstFiled)
		{
			sprintf(szColName, "%s", pFixCol->m_strColName.c_str());
			BuildValueWithFormat(szColValue, sizeof(szColValue), pFixCol->m_nDataType, pFixCol->m_strColValue, 0, false);
			//sprintf(szColValue, "'%s'", pFixCol->m_strColValue.c_str());
			bFirstFiled = false;
		}
		else
		{
			sprintf(szColName, ",%s", pFixCol->m_strColName.c_str());
			BuildValueWithFormat(szColValue, sizeof(szColValue), pFixCol->m_nDataType, pFixCol->m_strColValue, 0, true);
			//sprintf(szColValue, ",'%s'", pFixCol->m_strColValue.c_str());
		}
		strcat(szColNames, szColName);
		strcat(szColValues, szColValue);
	}

	sprintf(szSQL, "insert into %s(%s) values(%s)", m_pRule->m_strTableName.c_str(), szColNames, szColValues);
	return 0;
}


int CRuleTimer::handle_timeout(const ACE_Time_Value &current_time, const void *act /*= 0*/)
{
	char szCurDateTime[32] = { 0 };
	PKTimeHelper::GetCurHighResTimeString(szCurDateTime, sizeof(szCurDateTime));

	CMainTask *pMainTask = MAIN_TASK;
	CPKDbApi & pkdb = m_pRule->m_pDbConn->m_pkdb;
	if (!pkdb.IsConnected())
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "规则:%s, 数据库未连接, 将在定时器中检查连接, 这里不做重连操作!", m_pRule->m_strName.c_str());
		return -1;
	}

	int nTransactRet = pkdb.SQLTransact();
	if (nTransactRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "规则:%s, 开始事务失败, 不启用事务! 返回:%d", m_pRule->m_strName.c_str(), nTransactRet);
		return -1;
	}
	char szSql[1024] = { 0 };
	// 对象模式;
	for (int iTags = 0; iTags < m_pRule->m_vecObjProps.size(); iTags++)
	{
		CDBTransferRuleObjProps *pTags = m_pRule->m_vecObjProps[iTags];

		// read tag data
		int nRet = pkMGet(this->m_pTimerTask->m_hPkData, pTags->m_pkTagDatas, pTags->m_nTagDataNum);
		if (nRet != 0)
		{
			char szTip[512] = { 0 };
			for (int iTmp = 0; iTmp < pTags->m_nTagDataNum; iTmp++)
			{
				PKDATA * pkData = pTags->m_pkTagDatas + iTmp;
				string strObjPropName = pkData->szObjectPropName;
				if (strlen(szTip) == 0)
					PKStringHelper::Safe_StrNCpy(szTip, pkData->szObjectPropName, sizeof(szTip));
				else
					PKStringHelper::Snprintf(szTip, sizeof(szTip), "%s,%s", szTip, pkData->szObjectPropName);
			}
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "rule:%s,  pkMGet获取:%d个对象属性数据返回失败, 点:%s, 返回值:%d", m_pRule->m_strName.c_str(), pTags->m_nTagDataNum, szTip, nRet);
			// return nRet;
		}

		// 每一行记录只需要1个质量和时间戳;
		int nDataQuality = 0;
		string strDataTime = "";
		vector<CColValue> vecColValue;
		Json::Reader jsonReader;
		for (int iTag = 0; iTag < pTags->m_nTagDataNum; iTag++)
		{
			string strDataValue = "";
			string strPropName = "";
			CColValue valColInfo;

			strPropName = pTags->m_vecProp[iTag]->m_strPropName;
			PKDATA * pkData = pTags->m_pkTagDatas + iTag;
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

			valColInfo.m_strTagOrPropName = strPropName;
			valColInfo.m_strTagValue = strDataValue;
			valColInfo.m_nQuality = nDataQuality;
			vecColValue.push_back(valColInfo);
		}

		// 实时数据表和历史数据表的SQL是相同的
		if (m_pRule->m_bIsRTTable) // 需要更新操作;
		{
			GetUpdateObjectSQL(szSql, sizeof(szSql), pTags->m_strObjName.c_str(), vecColValue, m_pRule->m_vecColPropConfig, strDataTime.c_str(), nDataQuality, szCurDateTime);
		}
		else // 历史表，需要插入操作;
		{
			GetInsertObjectSQL(szSql, sizeof(szSql), pTags->m_strObjName.c_str(), vecColValue, m_pRule->m_vecColPropConfig, strDataTime.c_str(), nDataQuality, szCurDateTime);
		}

		vecColValue.clear();

		string strError;
		int nRet4 = pkdb.SQLExecute(szSql, &strError);
		if (nRet4 != 0)
		{
			//g_logger.LogMessage(PK_LOGLEVEL_ERROR, "规则:%s, SQl Execute Failed:%s, error:%s!", m_pRule->m_strName.c_str(), szSql, strError.c_str());
			continue;
		}
	} // for (int iTags = 0; iTags < m_pRule->m_vecObjProps.size(); iTags++)
	 
	// 设备模式;
	// 某一个规则表示多行记录;
	for (int iTags = 0; iTags < m_pRule->m_vecTags.size(); iTags++)
	{
		// 对于某一行记录;
		CDBTransferRuleTags *pTags = m_pRule->m_vecTags[iTags];

		// read tag data
		int nRet = pkMGet(this->m_pTimerTask->m_hPkData, pTags->m_pkTagDatas, pTags->m_nTagDataNum);
		if (nRet != 0)
		{
			char szTip[512];
			for (int iTmp = 0; iTmp < pTags->m_nTagDataNum; iTmp++)
			{
				PKDATA * pkData = pTags->m_pkTagDatas + iTmp;
				string strObjPropName = pkData->szObjectPropName;
				if (strlen(szTip) == 0)
					PKStringHelper::Safe_StrNCpy(szTip, pkData->szObjectPropName, sizeof(szTip));
				else
					PKStringHelper::Snprintf(szTip, sizeof(szTip), "%s,%s", szTip, pkData->szObjectPropName);
			}
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "rule:%s,  pkMGet获取:%d个tag数据返回失败, 点:%s, 返回值:%d", m_pRule->m_strName.c_str(), pTags->m_nTagDataNum, szTip, nRet);
			// return nRet;
		}

		// 每一行记录只需要1个质量和时间戳
		int nDataQuality = 0;
		string strDataTime = "";
		vector<CColValue> vecColTagValue;
		Json::Reader jsonReader;
		for (int iTag = 0; iTag < pTags->m_nTagDataNum; iTag++)
		{
			string strDataValue = "";
			string strTagName = "";
			CColValue valColValue;

			strTagName = pTags->m_vecTag[iTag]->m_strTagName;
			PKDATA * pkData = pTags->m_pkTagDatas + iTag;
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
					if (jsonTagData.isObject()) // 返回的是整个json数值对象
					{
						strDataTime = Json2String(jsonTagData["t"]);
						strDataValue = Json2String(jsonTagData["v"]);
						nDataQuality = ::atoi(Json2String(jsonTagData["q"]).c_str());
					}
					else // 取得某个域的值，如v的值
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

			//// 字段数目配置少了，值直接改为？？？
			//{
			//	string strTagName1 = m_pRule->m_vecColTagConfig.size() > 0 ? m_pRule->m_vecColTagConfig[0]->m_strColName : "";
			//	string strPropName1 = m_pRule->m_vecColPropConfig.size() > 0 ? m_pRule->m_vecColPropConfig[0]->m_strColName : "";
			//	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "规则:%s, table:%s, t_db_transfer_rule的field_tagname（%s)/field_propname(%s)配置了：%d 列，但t_db_transfer_rule_objprop的prop_id或t_db_transfer_rule_tag的tag_id列仅配置了:%d列。两者必须相等！",
			//		m_pRule->m_strName.c_str(), m_pRule->m_strTableName.c_str(), strTagName1.c_str(), strPropName1.c_str(), m_pRule->m_vecColTagConfig.size(),
			//		pTags->m_nTagDataNum);
			//	nDataQuality = -100;
			//	strDataValue = "";
			//}

			valColValue.m_strTagOrPropName = strTagName;
			valColValue.m_strTagValue = strDataValue;
			valColValue.m_nQuality = nDataQuality;
			vecColTagValue.push_back(valColValue);
		}

		// 实时数据表和历史数据表的SQL是相同的
		if (m_pRule->m_bIsRTTable) // 需要更新操作;
		{
			if (m_bFirstInsert)
			{
				GetInsertTagsSQL(szSql, sizeof(szSql), vecColTagValue, m_pRule->m_vecColTagConfig, strDataTime.c_str(), nDataQuality, szCurDateTime);
			}
			else
			{
				GetUpdateTagsSQL(szSql, sizeof(szSql), vecColTagValue, m_pRule->m_vecColTagConfig, strDataTime.c_str(), nDataQuality, szCurDateTime);
			}
		}
		else // 历史表，需要插入操作;
		{
			GetInsertTagsSQL(szSql, sizeof(szSql), vecColTagValue, m_pRule->m_vecColTagConfig, strDataTime.c_str(), nDataQuality, szCurDateTime);
		}
		vecColTagValue.clear();

		string strError;
		int nRet4 = pkdb.SQLExecute(szSql, &strError);
		if (nRet4 != 0)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "规则:%s, SQl Execute Failed:%s, error:%s!", m_pRule->m_strName.c_str(), szSql, strError.c_str());
			continue;
		}
		else
			g_logger.LogMessage(PK_LOGLEVEL_INFO, "规则:%s, SQl Execute Success:%s!", m_pRule->m_strName.c_str(), szSql);

	}

	if (nTransactRet == 0)
	{
		nTransactRet = pkdb.SQLCommit();
	}
	m_bFirstInsert = false;
	return 0;
}

