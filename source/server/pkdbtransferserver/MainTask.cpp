/**************************************************************
*  Filename:    MainTask.cpp
*  Copyright:   Shanghai Peak Software Co., Ltd.
*
*  Description: 网关客户端发布数据;
*
*  @author:    ycd;
*  @version    1.0.0
**************************************************************/
#include "ace/OS.h"
#include <ace/Default_Constants.h>
#include "map"

#include "math.h"
#ifdef _WIN32
#include "windows.h"
#else
#include "unistd.h"
#include "stdlib.h"
#endif

#include "pkcomm/pkcomm.h"
#include "pklog/pklog.h"
#include "pkdbapi/pkdbapi.h" 
#include "MainTask.h"
#include "TimerTask.h"
#include "RuleTimer.h"
#include "HourTimer.h"
#include "common/eviewcomm_internal.h"
using namespace std;

extern CPKLog g_logger;

#define  CHECK_NULL_RETURN_ERR(X)  { if(!X) return EC_ICV_DA_CONFIG_FILE_STRUCTURE_ERROR; }
#define	DEFAULT_SLEEP_TIME		100	
#define NULLASSTRING(x) x==NULL?"":x
#define SLEEP_MSEC				200

#define	DEFAULT_SLEEP_TIME		100	
#define SQLBUFFERSIZE			1024

int DatatypeToId(const char *szDataType)
{
	string strDataType = szDataType;
	std::transform(strDataType.begin(), strDataType.end(), strDataType.begin(), ::tolower);
	if (strDataType.find("int") != string::npos)
		return COL_DATATYPE_ID_INT;
	if (strDataType.find("real") != string::npos || strDataType.find("float") != string::npos || strDataType.find("double") != string::npos)
		return COL_DATATYPE_ID_REAL;
	if (strDataType.find("string") != string::npos || strDataType.find("text") != string::npos || strDataType.find("char") != string::npos)
		return COL_DATATYPE_ID_STRING;
	if (strDataType.find("time") != string::npos || strDataType.find("date") != string::npos)
		return COL_DATATYPE_ID_DATETIME;
	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "未知的列字段类型：%s, 认为是 字符串 类型", szDataType);
	return COL_DATATYPE_ID_STRING;
}

CMainTask::CMainTask()
{
	m_bStop = false;
	m_dbEview.InitFromConfigFile("db.conf", "database"); // .InitLocalSQLite();
	m_strValueOnBadQuality_Int = "-100000";
	m_strValueOnBadQuality_Real = "-100000";						    
	m_strValueOnBadQuality_String = "***";
	m_strValueOnBadQuality_DateTime = "1970-01-01 00:00:00";
}

CMainTask::~CMainTask()
{
	m_bStop = true;
}

int CMainTask::svc()
{
    //read all tags name from redis;
	//获取初始化状态;
	int nRet = 0;
	nRet = OnStart();

	ACE_Time_Value tv;
	tv.set_msec(SLEEP_MSEC);
	while (!m_bStop)
	{
		//1查询有变化的值发送

		ACE_OS::sleep(tv);
	}
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "MainTask exit normally!");
	OnStop();
	return PK_SUCCESS;
}

// 每一个规则，赋予到一个定时器中;
int CMainTask::AssignRuleToTimerObject()
{
	int nRet = PK_SUCCESS;
	CTimerTask *pTimerTask = TRANSFER_TIMER_TASK;
	map<string, CDBTransferRule *>::iterator itRule = m_mapId2TransferRule.begin();
	for (; itRule != m_mapId2TransferRule.end(); itRule++)
	{
		CDBTransferRule *pDBRule = itRule->second;
		pDBRule->BuildStatTags();
		int nPeriodSec = ::atoi(pDBRule->m_strSavePeriod.c_str());
		if (nPeriodSec <= 0)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "规则:%s, 配置的周期为:%s 秒, <= 0, 非法!跳过该规则!", pDBRule->m_strName.c_str(), nPeriodSec);
		}
		else
		{
			CRuleTimer *pTimer = new CRuleTimer(pTimerTask);
			pTimer->m_pRule = pDBRule;
			pTimer->m_nPeriodSec = ::atoi(pDBRule->m_strSavePeriod.c_str());
			pTimerTask->m_vecRuleTimer.push_back(pTimer);
		}

		// 检查是否有需要进行统计的定时器的规则;
		if (pDBRule->m_vecStatTagName.size() > 0)
		{
			CHourTimer *pTimer = new CHourTimer(pTimerTask);
			pTimer->m_pRule = pDBRule;
			pTimerTask->m_vecHourTimer.push_back(pTimer);
		}
	}
	return PK_SUCCESS;
}
//
int CMainTask::OnStart()
{
	int nStatus = PK_SUCCESS;
	int nRet = 0;

	nRet = LoadConfig();
	if (nRet < 0)
	{
		//g_logger.LogMessage(PK_LOGLEVEL_ERROR, "从数据库读取配置失败，退出...");
		//return -1;
	}

	// 将不同的Rule，赋予不同的Task
	AssignRuleToTimerObject();

	TRANSFER_TIMER_TASK->Start();
	return nRet;
}

// 本任务停止时;
int CMainTask::OnStop()
{
	return 0;
}

int CMainTask::Start()
{
	return activate(THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED);
}

void CMainTask::Stop()
{
	m_bStop = true;
	int nWaitResult = wait();
}

int CMainTask::ReadAllTags()
{
	m_vecTagsName.clear();
	// ---- 读取所有的TAG点----.还要根据对象模式还是设备模式进行读取;
	int nRet = 0;
	char szSql[1024] = { 0 };

	// 设备模式;
	{
		sprintf(szSql, "select tt.name from t_device_tag tt ,t_device_list tl, t_device_driver td where (tt.device_id = tl.id and tl.driver_id = td.id and tl.disable <> 1)");
		vector<vector<string> > vecRows;
		nRet = m_dbEview.SQLExecute(szSql, vecRows);
		if (nRet == 0)
		{
			for (int i = 0; i < vecRows.size(); i++)
			{
				string strTagName = vecRows[i][0];
				m_vecTagsName.push_back(strTagName);
			}
		}
		else
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "查询TAG列表失败, %s, 返回:%d", szSql, nRet);
		}
	}
	
	// 对象模式;
	{
		sprintf(szSql, "select obj.name as objname,prop.name as propname from t_class_list cls, t_class_prop prop,t_object_list obj \
			where prop.class_id = cls.id and obj.class_id = cls.id and(obj.device_id = null or obj.device_id in(select id from t_device_list where t_device_list.disable < >1))");
		vector<vector<string> > vecRows;
		nRet = m_dbEview.SQLExecute(szSql, vecRows);
		if (nRet == 0)
		{
			for (int i = 0; i < vecRows.size(); i++)
			{
				string strObjName = vecRows[i][0];
				string strPropName = vecRows[i][1];
				string strTagName = strObjName + "." + strPropName;
				m_vecTagsName.push_back(strTagName);
			}
		}
		else
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "查询对象和属性列表失败, %s, 返回:%d", szSql, nRet);
		}
	}

	int nTagNum = m_vecTagsName.size();
	return 0;
}

//************************************
// Method:    GetGwConfig
// FullName:  CMainTask::GetGwConfig
// Access:    public 
// Returns:   int
// Qualifier: 获取网关配置信息，在表 t_gateway_conf中
//************************************
int CMainTask::LoadConfig()
{
	int nRet = PK_SUCCESS;
	string strIniFilePath = PKComm::GetConfigPath();
	strIniFilePath = strIniFilePath + PK_OS_DIR_SEPARATOR + PKComm::GetProcessName() + PK_OS_DIR_SEPARATOR + ".ini";


	PKEviewComm::LoadSysParam(m_dbEview, "valueonbad_int", "-100000", m_strValueOnBadQuality_Int);
	PKEviewComm::LoadSysParam(m_dbEview, "valueonbad_real", "-100000", m_strValueOnBadQuality_Real);
	PKEviewComm::LoadSysParam(m_dbEview, "valueonbad_string", "*", m_strValueOnBadQuality_String);
	PKEviewComm::LoadSysParam(m_dbEview, "valueonbad_datetime", "1970-01-01 00:00:00", m_strValueOnBadQuality_DateTime);

	g_logger.LogMessage(PK_LOGLEVEL_INFO, "ReadFromTable t_sys_param, valueonbad_int=%s, valueonbad_real=%s, valueonbad_string=%s, valueonbad_datetime=%s",
		m_strValueOnBadQuality_Int.c_str(), m_strValueOnBadQuality_Real.c_str(), m_strValueOnBadQuality_String.c_str(), m_strValueOnBadQuality_DateTime.c_str());

	nRet = ReadDBConnList();
	if (nRet != 0)
	{
		return nRet;
	}

	nRet = ReadTransferRuleList();
	if (nRet != 0)
	{
		return nRet;
	}

	nRet = ReadTagList();
	if (nRet != 0)
	{
		return nRet;
	}
	return nRet;
}

int CMainTask::ReadDBConnList()
{
	m_vecTagsName.clear();
	vector<vector<string> > vecRows;
	int nRet = 0;
	char szSql[1024] = { 0 };
	sprintf(szSql, "select id,name,dbtype,connstring,username,password,description,codingset from t_db_connection");
	nRet = m_dbEview.SQLExecute(szSql, vecRows);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "查询数据库转储连接的列表失败, %s, 返回:%d", szSql, nRet);
		return -1;
	}

	for (int i = 0; i < vecRows.size(); i++)
	{
		vector<string> vecRow = vecRows[i];

		// 检查连接编号是否重复;
		string strId = vecRow[0];
		if (m_mapId2DbConn.find(strId) != m_mapId2DbConn.end())
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "查询数据库转储连接的列表发现id为:%s 的连接配置已经存在, 重复了, %s, 返回:%d", strId.c_str(), szSql, nRet);
			continue;
		}

		// 检查数据库类型是否合法;
		string strDbType = vecRow[2];
		int nDBType = CPKDbApi::ConvertDBTypeName2Id(strDbType.c_str());
		if (nDBType == PK_DATABASE_TYPEID_INVALID)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "查询数据库转储连接的列表发现id为:%s 的连接配置，不支持的数据库类型:%s, 我们支持:oracle/db2/mysql/postgresql/sqlite/odbc/sqlserver/interbase/informix/sybase", strId.c_str(), strDbType.c_str());
			continue;
		}

		CDBConnection *pDBConn = new CDBConnection();
		pDBConn->m_strId = strId;
		pDBConn->m_strName = vecRow[1];
		pDBConn->m_strDbType = strDbType;
		pDBConn->m_nDBType = nDBType;
		pDBConn->m_strConnString = vecRow[3];
		pDBConn->m_strUserName = vecRow[4];
		pDBConn->m_strPassword = vecRow[5]; 
		pDBConn->m_strDescription = vecRow[6];
		pDBConn->m_strCodeset = vecRow[7];
		vecRow.clear();

		if (m_mapId2DbConn.find(pDBConn->m_strId) != m_mapId2DbConn.end())
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "查询数据库转储连接的列表发现id为:%s 的连接配置已经存在, 重复了, %s, 返回:%d", pDBConn->m_strId.c_str(), szSql, nRet);
			delete pDBConn;
			continue;
		}
		m_mapId2DbConn[pDBConn->m_strId] = pDBConn;
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "数据库转储连接, id为:%s, name:%s, dbtype:%s, connstring:%s, username:%s,password:$s,description:%s, codeset:%s",
			pDBConn->m_strId.c_str(), pDBConn->m_strName.c_str(), pDBConn->m_strDbType.c_str(), pDBConn->m_strConnString.c_str(), pDBConn->m_strUserName.c_str(), pDBConn->m_strPassword.c_str(),
			pDBConn->m_strDescription.c_str(), pDBConn->m_strCodeset.c_str());
	}
	vecRows.clear();
	return 0;
}

//读取转换规则;
int CMainTask::ReadTransferRuleList()
{
	m_mapId2TransferRule.clear();
	vector<vector<string> > vecRows;
	int nRet = 0;
	char szSql[1024] = { 0 };
	
	sprintf(szSql, "select id,dbconn_id,name,tablename,saveperiod,writeonchange,description,writemethod,\
				    fieldname_objname,fieldname_propname,fieldname_tagname,fieldname_value,fieldname_quality,fieldname_datatime,fieldname_updatetime,fieldname_fixcols from t_db_transfer_rule");
	nRet = m_dbEview.SQLExecute(szSql, vecRows);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "查询数据库转储规则的列表失败: %s, 返回: %d", szSql, nRet);
		return -1;
	}

	for (int i = 0; i < vecRows.size(); i++)
	{
		vector<string> vecRow = vecRows[i];
		int iCol = 0;
		CDBTransferRule *pDBRule = new CDBTransferRule();
		pDBRule->m_strId = vecRow[iCol];
		iCol++;
		pDBRule->m_strDBConnId = vecRow[iCol];
		iCol++;
		pDBRule->m_strName = vecRow[iCol];
		iCol++;
		pDBRule->m_strTableName = vecRow[iCol];
		iCol++;
		pDBRule->m_strSavePeriod = vecRow[iCol];
		iCol++;
		pDBRule->m_bWriteOnChange = ::atoi(vecRow[iCol].c_str());
		iCol++;
		pDBRule->m_strDescription = vecRow[iCol];
		iCol++;
		pDBRule->m_strWriteMethod = vecRow[iCol];
		iCol++;
		if (PKStringHelper::StriCmp(pDBRule->m_strWriteMethod.c_str(), "real") == 0)
			pDBRule->m_bIsRTTable = true;
		else
			pDBRule->m_bIsRTTable = false;

		pDBRule->m_colObjName.ParseFrom(vecRow[iCol]);
		iCol++;
		string strFieldName_PropName = vecRow[iCol];
		iCol++;

		string strFieldName_TagName = vecRow[iCol];
		pDBRule->m_colTagName.ParseFrom(strFieldName_TagName); // 变量名称的列只允许1个变量名称，如果是多个则取第1个

		iCol++;
		string strFieldName_TagValue = vecRow[iCol];
		iCol++;
		pDBRule->m_colQuality.ParseFrom(vecRow[iCol]);
		iCol++;
		pDBRule->m_colDateTime.ParseFrom(vecRow[iCol]);
		iCol++;
		pDBRule->m_colUpdateTime.ParseFrom(vecRow[iCol]);
		iCol++;
		string strFieldName_FixCols = vecRow[iCol];
		iCol++;

		// fieldname_fixcols的多个字段的解析;
		vector<string> vecFixCol = PKStringHelper::StriSplit(strFieldName_FixCols, ";"); // stationid,int,X1234;stationname,string,瓜步汛
		for (int iFixCol = 0; iFixCol < vecFixCol.size(); iFixCol++)
		{
			string strFixCol = vecFixCol[iFixCol];
			if (strFixCol.empty())
				continue;
			CFixCol *pFixCol = new CFixCol();
			int nRet = pFixCol->ParseFrom(strFixCol); // stationid,int,X1234
			if (nRet == 0)
				pDBRule->m_vecFixCol.push_back(pFixCol);
			else
			{
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "DBRule, parse FixCol failed:%s, ruleName:%s, continue...", strFieldName_FixCols.c_str(), pDBRule->m_strName.c_str());
				delete pFixCol;
			}
		}

		// fieldname_tagvalue的多个字段的解析
		vector<string> vecColTagValue = PKStringHelper::StriSplit(strFieldName_TagValue, ";"); // 温度,int;湿度,string
		for (int iCol = 0; iCol < vecColTagValue.size(); iCol++)
		{
			string strColInfo = vecColTagValue[iCol];
			if (strColInfo.empty())
				continue;		
			
			CColInfo *pColInfo = new CColInfo();
			int nRet = pColInfo->ParseFrom(strColInfo); // stationid,int,X1234
			if (nRet == 0)
				pDBRule->m_vecColTagConfig.push_back(pColInfo);
			else
			{
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "DBRule, parse fieldname_TagName failed:%s, ruleName:%s, col:%s, continue...", strFieldName_TagName.c_str(), pDBRule->m_strName.c_str(), strColInfo.c_str());
				delete pColInfo;
			}
		}

		// fieldname_propname的多个字段的解析
		//---------------------------------------------bake--------------------------------------------------------------------
		//vector<string> vecColPropName = PKStringHelper::StriSplit(strFieldName_PropName, ";"); // 温度,int;湿度,string
		//for (int iCol = 0; iCol < vecColPropName.size(); iCol++)
		//{
		//	string strColInfo = vecColPropName[iCol];
		//	if (strColInfo.empty())
		//		continue;
		//	CColInfo *pColInfo = new CColInfo();
		//	int nRet = pColInfo->ParseFrom(strColInfo); // stationid,int,X1234
		//	if (nRet == 0)
		//		pDBRule->m_vecColPropConfig.push_back(pColInfo);
		//	else
		//	{
		//		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "DBRule, parse fieldname_TagName failed:%s, ruleName:%s, col:%s, continue...", strFieldName_TagName.c_str(), pDBRule->m_strName.c_str(), strColInfo.c_str());
		//		delete pColInfo;
		//	}
		//}
		//g_logger.LogMessage(PK_LOGLEVEL_INFO, "DBRule, rule id:%s, name:%s, table:%s, prop count:%d, tag count:%d, fixcount:%d", 
		//	pDBRule->m_strId.c_str(), pDBRule->m_strName.c_str(), pDBRule->m_strTableName.c_str(), pDBRule->m_vecColPropConfig.size(), pDBRule->m_vecColTagConfig.size(), pDBRule->m_vecFixCol.size());
		//vecRow.clear();
		//if (m_mapId2TransferRule.find(pDBRule->m_strId) != m_mapId2TransferRule.end())
		//{
		//	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "查询数据库转储规则的列表发现id为:%s 的规则配置已经存在, SQL: %s, 返回:%d", pDBRule->m_strId.c_str(), szSql, nRet);
		//	delete pDBRule;
		//	continue;
		//}
		//---------------------------------------------bake end------------------------------------------------------------------

		//===============================================================================================================================================================
		vector<string> vecColPropName = PKStringHelper::StriSplit(strFieldName_PropName, ";"); // 温度,int;湿度,string
		for (int iCol = 0; iCol < vecColPropName.size(); iCol++)
		{
			string strColInfo = vecColPropName[iCol];
			if (strColInfo.empty())
				continue;

			CColInfo *pColInfo = new CColInfo();
			int nRet = pColInfo->ParseFrom(strColInfo); // stationid,int,X1234
			if (nRet == 0)
				pDBRule->m_vecColPropConfig.push_back(pColInfo);
			else
			{
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "DBRule, parse fieldname_TagName failed:%s, ruleName:%s, col:%s, continue...", strFieldName_TagName.c_str(), pDBRule->m_strName.c_str(), strColInfo.c_str());
				delete pColInfo;
			}
		}

		g_logger.LogMessage(PK_LOGLEVEL_INFO, "DBRule, rule id:%s, name:%s, table:%s, prop count:%d, tag count:%d, fixcount:%d",
			pDBRule->m_strId.c_str(), pDBRule->m_strName.c_str(), pDBRule->m_strTableName.c_str(), pDBRule->m_vecColPropConfig.size(), pDBRule->m_vecColTagConfig.size(), pDBRule->m_vecFixCol.size());

		vecRow.clear();

		if (m_mapId2TransferRule.find(pDBRule->m_strId) != m_mapId2TransferRule.end())
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "查询数据库转储规则的列表发现id为:%s 的规则配置已经存在, SQL: %s, 返回:%d", pDBRule->m_strId.c_str(), szSql, nRet);
			delete pDBRule;
			continue;
		}
		//===================================================================================================================================================================


		// 检查是否有对应的数据库连接;
		map<string, CDBConnection *>::iterator itDBConn = m_mapId2DbConn.find(pDBRule->m_strDBConnId);
		if (itDBConn == m_mapId2DbConn.end())
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "查询数据库转储规则的列表发现id为:%s 的规则配置, 其数据库连接id:%s, 不存在该id的数据库连接配置, 跳过, SQL: %s, 返回:%d", 
				pDBRule->m_strId.c_str(), pDBRule->m_strDBConnId.c_str(), szSql, nRet);
			delete pDBRule;
			continue;
		}
		CDBConnection *pDBConn = itDBConn->second;
		pDBRule->m_pDbConn = pDBConn;
		pDBConn->m_vecTransferRule.push_back(pDBRule);

		m_mapId2TransferRule[pDBRule->m_strId] = pDBRule;
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "转储规则, id:%s,name:%s, tablename:%s, saveperiod:%s 秒, writeOnChange:%d, description:%s, 转储类型:%s, 是否是实时表:%s",
			pDBRule->m_strId.c_str(), pDBRule->m_strName.c_str(), pDBRule->m_strTableName.c_str(), pDBRule->m_strSavePeriod.c_str(), pDBRule->m_bWriteOnChange, pDBRule->m_strDescription.c_str(),
			pDBRule->m_strWriteMethod.c_str(), pDBRule->m_bIsRTTable ? "是" : "否");
	}
	vecRows.clear();
	return 0;
}

int CMainTask::ReadTagList()
{
	vector<vector<string> > vecRowsTag;
	int nRet = 0;
	char szSql[1024] = { 0 };
	int nTagCount = 0;
	
	// 设备模式 
	// sprintf(szSql, "select RULE_TAG.tag_id,TAG.name,rule_id from t_db_transfer_rule_tag RULE_TAG left JOIN t_device_tag TAG on TAG.id=tag_id");
	sprintf(szSql, "select rule_id,tag_id from t_db_transfer_rule_tag");
	nRet = m_dbEview.SQLExecute(szSql, vecRowsTag);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "查询要转储的变量列表失败,SQL: %s, 返回:%d", szSql, nRet);
		return -1;
	}

	for (int i = 0; i < vecRowsTag.size(); i++)
	{
		vector<string> vecRow = vecRowsTag[i];
		CDBTransferRuleTags *pDBRuleTag = new CDBTransferRuleTags();
		pDBRuleTag->m_strRuleId = vecRow[0];
		string strTagIds = vecRow[1];
		vector<string> vecTagId = PKStringHelper::StriSplit(strTagIds, ","); //tagid1,tagid2,tagid3,tagid4
		for (int iTag = 0; iTag < vecTagId.size(); iTag++)
		{
			string strTagId = vecTagId[iTag];
			sprintf(szSql, "select name,id from t_device_tag where id=%s", strTagId.c_str());
			string strError;
			string strTagName = ""; // 查找失败或者未找到，直接为空
			vector <vector<string > > vecRowsTag;
			int nRetTag = m_dbEview.SQLExecute(szSql, vecRowsTag, &strError);
			if (nRetTag != 0)
			{
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "根据转储t_db_transfer_rule_tag的tagid:%s查找tagname失败！该点的值将被置位NULL, SQL: %s, 返回:%d, ERROR:%s", strTagId.c_str(), szSql, nRetTag, strError.c_str());
			}
			else
			{
				if (vecRowsTag.size() == 0)
				{
					g_logger.LogMessage(PK_LOGLEVEL_ERROR, "根据转储t_db_transfer_rule_tag的tagid:%s, 未找到该TAGID对应的TAG！该点的值将被置位NULL, SQL: %s, 返回:%d", strTagId.c_str(), szSql, nRetTag);
				}
				else
				{
					vector<string> vecRow = vecRowsTag[0];
					strTagName = vecRow[0];
				}
			}
			CTagInfo *pTagInfo = new CTagInfo();
			pTagInfo->m_strTagId = strTagId;
			pTagInfo->m_strTagName = strTagName;
			pDBRuleTag->m_vecTag.push_back(pTagInfo);
			nTagCount++;
		}

		vecRow.clear();

		map<string, CDBTransferRule *>::iterator itRule = m_mapId2TransferRule.find(pDBRuleTag->m_strRuleId);
		if (itRule == m_mapId2TransferRule.end())
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "转储的tag点id:%s, 对应的规则id:%s, 规则未配置! SQL:%s", strTagIds.c_str(), pDBRuleTag->m_strRuleId.c_str(), szSql);
			delete pDBRuleTag;
			continue;
		}
		CDBTransferRule *pDbTransferRule = itRule->second;
		pDbTransferRule->m_vecTags.push_back(pDBRuleTag);
	}
	vecRowsTag.clear();
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "查询要转储的设备变量列表的有效个数为:%d", nTagCount);
	

	// 对象模式
	vector<vector<string> > vecRowsObjProp;
	// sprintf(szSql, "select RULE_OBJPROP.object_id,OBJ.name,RULE_OBJPROP.prop_id,PROP.name,rule_id from t_db_transfer_rule_objprop RULE_OBJPROP LEFT JOIN t_object_list OBJ on OBJ.id=RULE_OBJPROP.object_id LEFT JOIN t_class_prop PROP on  RULE_OBJPROP.prop_id=PROP.id ");
	sprintf(szSql, "select RULE_OBJPROP.object_id,OBJ.name,RULE_OBJPROP.prop_id,rule_id from t_db_transfer_rule_objprop RULE_OBJPROP LEFT JOIN t_object_list OBJ on OBJ.id=RULE_OBJPROP.object_id ");
	nRet = m_dbEview.SQLExecute(szSql, vecRowsObjProp);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "查询要转储的对象的属性列表失败, %s, 返回:%d", szSql, nRet);
		return -1;
	}

	for (int i = 0; i < vecRowsObjProp.size(); i++)
	{
		vector<string> vecRow = vecRowsObjProp[i];
		CDBTransferRuleObjProps *pDBRuleObjProp = new CDBTransferRuleObjProps();
		pDBRuleObjProp->m_strObjId = vecRow[0];
		pDBRuleObjProp->m_strObjName = vecRow[1];
		string strPropIds = vecRow[2];
		pDBRuleObjProp->m_strRuleId = vecRow[3];
		vecRow.clear();

		vector<string> vecTagObjProp = PKStringHelper::StriSplit(strPropIds, ","); //tagid1,tagid2,tagid3,tagid4
		for (int iObjProp = 0; iObjProp < vecTagObjProp.size(); iObjProp++)
		{
			string strObjPropId = vecTagObjProp[iObjProp];
			sprintf(szSql, "select name,id from t_class_prop PROP where id=%s", strObjPropId.c_str());
			string strError;
			string strObjPropName = ""; // 查找失败或者未找到，直接为空
			vector <vector<string> > vecRowProp;
			int nRetProp = m_dbEview.SQLExecute(szSql, vecRowProp, &strError);
			if (nRetProp != 0)
			{
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "根据转储t_db_transfer_rule_obj_prop的objpropid:%s 查找tagname失败！该点的值将被置位NULL, SQL: %s, 返回:%d, ERROR:%s", strObjPropId.c_str(), szSql, nRetProp, strError.c_str());
			}
			else
			{
				if (vecRowProp.size() == 0)
				{
					g_logger.LogMessage(PK_LOGLEVEL_ERROR, "根据转储t_db_transfer_rule_tag的tagid:%s, 未找到该TAGID对应的TAG！该点的值将被置位NULL, SQL: %s, 返回:%d", strObjPropId.c_str(), szSql, nRetProp);
				}
				else
				{
					vector<string> vecRow = vecRowProp[0];
					strObjPropName = vecRow[0];
				}
			}
			CPropInfo *pPropInfo = new CPropInfo();
			pPropInfo->m_strPropId = strObjPropId;
			pPropInfo->m_strPropName = strObjPropName;
			pDBRuleObjProp->m_vecProp.push_back(pPropInfo);
			nTagCount++;
		}
		vecTagObjProp.clear();

		map<string, CDBTransferRule *>::iterator itRule = m_mapId2TransferRule.find(pDBRuleObjProp->m_strRuleId);
		if (itRule == m_mapId2TransferRule.end())
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "转储的对象id:%s, 属性id:%s, 对应的规则id:%s, 规则未配置! SQL:%s", pDBRuleObjProp->m_strObjId.c_str(), strPropIds.c_str(), pDBRuleObjProp->m_strRuleId.c_str(), szSql);
			delete pDBRuleObjProp;
			continue;
		}

		CDBTransferRule *pDbTransferRule = itRule->second;
		pDbTransferRule->m_vecObjProps.push_back(pDBRuleObjProp);

	}
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "查询要转储的对象的属性+变量的个数为:%d", nTagCount);
	vecRowsObjProp.clear();
	
	return 0;	
}
