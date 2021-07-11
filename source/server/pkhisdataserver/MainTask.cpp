/**************************************************************
 *  Filename:    ExecutionTak.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: ִ��Ԥ�����̣߳����յ���Ϣ��������Դ����������ִ����ϵ�.
 *
 *  @author:     shijunpu
 *  @version     05/22/2008  shijunpu   Initial Version
**************************************************************/
#include "TrendTimerTask.h"
#include "RecordTimerTask.h"
#include "ace/High_Res_Timer.h"
#include "common/eviewcomm_internal.h"
#include <fstream>  
#include <streambuf>  
#include <iostream>  

#include "pkdbapi/pkdbapi.h"
#include "pkinifile/pkinifile.h"
#include "pklog/pklog.h"
#include "json/json.h"
#include "common/eviewdefine.h"
#include "pkdata/pkdata.h"	
#include "global.h"
#include "MainTask.h"


using namespace std;

#ifndef __sun
using std::map;
using std::string;
#endif

#define NULLASSTRING(x) x==NULL?"":x
extern CPKLog g_logger;

void StringReplace(string &strBase, string strSrc, string strDes)
{
	string::size_type pos = 0;
	string::size_type srcLen = strSrc.size();
	string::size_type desLen = strDes.size();
	pos = strBase.find(strSrc, pos);
	while ((pos != string::npos))
	{
		strBase.replace(pos, srcLen, strDes);
		pos = strBase.find(strSrc, (pos + desLen));
	}
}

inline string& ltrim(string &str) {
	string::iterator p = find_if(str.begin(), str.end(), not1(ptr_fun<int, int>(isspace)));
	str.erase(str.begin(), p);
	return str;
}

inline string& rtrim(string &str) {
	string::reverse_iterator p = find_if(str.rbegin(), str.rend(), not1(ptr_fun<int, int>(isspace)));
	str.erase(p.base(), str.end());
	return str;
}

inline string& trim(string &str) {
	ltrim(rtrim(str));
	return str;
}

string SubString(string &src, int nLen)
{
	if (src.length() < nLen)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "when SubStr(%s,%d), srcLen:%d < %d", src.c_str(), nLen, src.c_str(), nLen);
		return "";
	}

	string strSubString = src.substr(0, nLen); // 2017-06-28 10:15:00.123 ---��2017-06-28 10:1
	return strSubString;
}

CMainTask::CMainTask():m_mongoConn(true) // �����Զ���������
{
	m_bStop = false;
	ACE_Select_Reactor *pSelectReactor = new ACE_Select_Reactor();
	m_pReactor = new ACE_Reactor(pSelectReactor, true);
	this->reactor(m_pReactor);
	m_strMongoConnString = "";
	m_nVersion = 0;
	m_tmLastMainData = 0;

}

CMainTask::~CMainTask()
{
	if(m_pReactor)	
	{
		delete m_pReactor;
		m_pReactor = NULL;
	}
}


/**
 *  Start the Task and threads in the task are activated.
 *
 *  @return 
 *	- ==0 success
 *	- !=0 failure
 *
 *  @version     05/23/2008  shijunpu  Initial Version.
 */
long CMainTask::StartTask()
{
	long lRet = (long)activate(THR_NEW_LWP | THR_JOINABLE |THR_INHERIT_SCHED, 1);
	if(lRet != PK_SUCCESS)
	{
		lRet = -2;
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, _("CExecutionTask: StartTask() Failed, RetCode: %d"), lRet);
	}

	return PK_SUCCESS;	
}

/**
 *  stop the request handler task and threads in it.
 *
 *  @return 
 *	- ==0 success
 *	- !=0 failure
 *
 *  @version     05/23/2008  shijunpu  Initial Version.
 */
long CMainTask::StopTask()
{
	m_bStop = true;
	this->reactor()->end_reactor_event_loop();
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "MainTask::StopTask()...");
	int nWaitResult = wait(); // ����ȴ��������أ�����������
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "MainTask::StopTask() return");

	return PK_SUCCESS;	
}

/*�Ӵ����¼����������¼���Ϣ
 *  main procedure of the task.
 *	get a msgblk and 
 *  @return 
 *	- ==0 success
 *	- !=0 failure
 *
 *  @version     05/23/2008  shijunpu  Initial Version.

 //���� EventTypeTaskManager ActionTypeTaskManagers
 */
int CMainTask::svc()
{
	g_logger.LogMessage(PK_LOGLEVEL_INFO, _("MainTask::svc() begin"));
	
	ACE_High_Res_Timer::global_scale_factor();
	this->reactor()->owner(ACE_OS::thr_self ());

	int nRet = PK_SUCCESS;
	nRet = OnStart();

	while(!m_bStop)
	{
		PKComm::Sleep(200);

		MaintainAllTables();

		//this->reactor()->reset_reactor_event_loop();
		//this->reactor()->run_reactor_event_loop();
	}

	g_logger.LogMessage(PK_LOGLEVEL_INFO,"MainTask stopped");
	OnStop();

	return PK_SUCCESS;	
}

int CMainTask::OnStart()
{
	int nRet = 0;
	// �����ʱ�����Ƕ�ʱ����Ƿ�Ҫɾ���ļ��������Ƿ�ɹ��Ķ�ʱ��
	//��ȡ���������Ϣ;
	nRet = LoadConfig();
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "LoadConfig failed,ret:%d", nRet);
		return nRet;
	}

	//����Mongodb���ݿ�;
#ifdef _WIN32
	WSAData wsaData;
	if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)
	{
		return -1;
	}
#endif 

	ReadHisDatabaseUrl(); // ��ȡmongo�ĵ�ַ

	InitMongdb();
	TRENDTIMER_TASK->StartTask();
	RECORDTIMER_TASK->StartTask();

	return 0;
}

int CMainTask::OnStop()
{
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "MainTask::OnStop begin.");
	m_pReactor->close();
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "MainTask::OnStop Reactor->close().");

	// ֹͣ�����߳�;
	TRENDTIMER_TASK->StopTask();
	RECORDTIMER_TASK->StopTask();

	g_logger.LogMessage(PK_LOGLEVEL_INFO, "MainTask::OnStop end.");
	return 0;
}

int CMainTask::LoadObjectPropIntervalConfig(CPKDbApi &PKDbApi, map<string, string> &mapTagName2Interval)
{
	int nRet = PK_SUCCESS;

	// ��ѯtag��
	long lQueryId = -1;
	char szSql[2048] = { 0 };
	vector<vector<string> > vecRow;

	int nPropChangeCount = 0;
	sprintf(szSql, "select OBJ.name as object_name,PROP.name as class_prop_name,A.object_prop_field_name as object_prop_field_name,A.object_prop_field_value as object_prop_field_value \
				   				   		from t_object_prop A LEFT JOIN t_object_list OBJ on A.object_id = OBJ.id LEFT JOIN t_class_prop PROP on A.class_prop_id = PROP.id\
										where (A.object_prop_field_name='hisdata_interval' or A.object_prop_field_name='trenddata_interval') and (OBJ.enable is NULL or OBJ.enable<>0 or OBJ.enable='') ");
	string strError;
	nRet = PKDbApi.SQLExecute(szSql, vecRow, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "fail to read class info from database:%s, return:%d, error:%s", szSql, nRet, strError.c_str());
		return -2;
	}

	for (int iTab = 0; iTab < vecRow.size(); iTab++)
	{
		vector<string> &row = vecRow[iTab];
		int iCol = 0;
		string strObjectName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strClassPropName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strObjectPropKey = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strObjectPropValue = NULLASSTRING(row[iCol].c_str());
		iCol++;

		if (strObjectName.empty() || strClassPropName.empty() || strObjectPropKey.empty())
			continue;

		string strTagPropName = strObjectName + "." + strClassPropName + strObjectPropKey;
		map<string, string>::iterator itMap = mapTagName2Interval.find(strTagPropName);
		if (itMap != mapTagName2Interval.end()) // �����ڸ��豸���˳�
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "LoadObjectPropChangeConfig, ��������Ϊ:%s����������:%s, �����ظ�! key��%s, value:%s ", strObjectName.c_str(), strClassPropName.c_str(), strObjectPropKey.c_str(), strObjectPropValue.c_str());
			continue;
		}
		mapTagName2Interval[strTagPropName] = strObjectPropValue;
	}

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "��ȡ���Ķ����Լ������Ը���:%d, ��Ч����:%d, ��Ч����:%d", vecRow.size(), mapTagName2Interval.size(), vecRow.size() - mapTagName2Interval.size());
	return PK_SUCCESS;
}

int CMainTask::LoadConfig()
{
	CPKDbApi PKDbApi;
	int nRet = PKDbApi.InitFromConfigFile("db.conf", "database");
	if (nRet != PK_SUCCESS)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Init DataBase failed, ret:%d", nRet);
		PKDbApi.UnInitialize();
		return nRet;
	}

	m_nVersion = PKEviewComm::GetVersion(PKDbApi, "pkhisdataserver");
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "��ȡ���İ汾��Ϊ:%", m_nVersion);

	string strTrendIntervalSec, strTrendSaveDay, strRecordIntervalMinute, strRecordSaveDay;
	PKEviewComm::LoadSysParam(PKDbApi, "trenddata_interval_second", "0", strTrendIntervalSec);
	PKEviewComm::LoadSysParam(PKDbApi, "hisdata_interval_minute", "0", strRecordIntervalMinute);
	PKEviewComm::LoadSysParam(PKDbApi, "hisdata_save_day", "90", strRecordSaveDay); // ֻ��������δ���������ǲ��ܱ������ݵ�
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "��ȡȫ�ֲ���, ʵʱ���Ƽ��:%s�롣��ʷ���ݱ�������%s���ӣ�����ʱ����%s��",
		strTrendIntervalSec.c_str(), strRecordIntervalMinute.c_str(), strRecordSaveDay.c_str());
	m_nTrendIntervalSec = ::atoi(strTrendIntervalSec.c_str());
	m_nRecordIntervalSec = ::atoi(strRecordIntervalMinute.c_str()) * 60; // ��Ϊ��λ
	m_nRecordSaveDay = ::atoi(strRecordSaveDay.c_str());

	if (m_nTrendIntervalSec <= 0) // ���Ƽ���������1����
		m_nTrendIntervalSec = 0;

	if (m_nTrendIntervalSec > 60) // ���Ʊ���С��1����
		m_nTrendIntervalSec = 60;

	if (m_nRecordIntervalSec <= 0) // ȱʡ�ǲ������
		m_nRecordIntervalSec = 0;

	if (m_nRecordSaveDay <= 0)
		m_nRecordSaveDay = 90; // ΪnullΪ0��ʾȱʡ����90��

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "��֤����, ʵʱ���Ƽ��:%d�롣��ʷ��¼��������%d�룬����ʱ����%d��",
		m_nTrendIntervalSec, m_nRecordIntervalSec, m_nRecordSaveDay);

	//����sqlite���ݿⲢ�ҽ����ݽ��б���;
	m_vecTags.clear();

	vector<vector<string> > vecRow;
	vector<int > lis;
	long lQueryId = 0;
	char szSql[2048] = { 0 };

	int nTagNumWithTrend = 0;
	int nTagNumWithHisData = 0;

	// �豸ģʽ�µı���
	string strError;
	sprintf(szSql, "select name,hisdata_interval,trenddata_interval from t_device_tag \
	where (hisdata_interval is not null and hisdata_interval  <> '' and hisdata_interval > 0) \
		or(trenddata_interval is not null and trenddata_interval  <> '' and trenddata_interval > 0) ");
	nRet = PKDbApi.SQLExecute(szSql, vecRow, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "execute query tags from t_device_tag to save history data failed, sql:%s, error:%s", szSql, strError.c_str());
	}
	else
	{
		//������������tag�㱣������;
		for (int iRow = 0; iRow < vecRow.size(); iRow++)
		{
			vector<string> &row = vecRow[iRow];

			int iCol = 0;
			string strTagName = NULLASSTRING(row[iCol].c_str());
			iCol++;
			string strHisDataInterVal = NULLASSTRING(row[iCol].c_str());
			iCol++;
			string strTrendDataInterVal = NULLASSTRING(row[iCol].c_str());
			iCol++;

			if (strTagName.empty())
			{
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "tagname:%s is null", strTagName.c_str());
				continue;
			}

			CTagInfo *pTag = new CTagInfo();
			pTag->strTagName = strTagName;
			if (strHisDataInterVal.empty())
				pTag->nRecordIntervalSec = m_nRecordIntervalSec;
			else
				pTag->nRecordIntervalSec = ::atof(strHisDataInterVal.c_str()) * 60;

			if (pTag->nRecordIntervalSec > 0)
				nTagNumWithHisData++;

			if (strTrendDataInterVal.empty())
				pTag->nTrendIntervalSec = m_nTrendIntervalSec;
			else
				pTag->nTrendIntervalSec = ::atoi(strTrendDataInterVal.c_str());

			if (pTag->nTrendIntervalSec > 0)
				nTagNumWithTrend++;

			m_vecTags.push_back(pTag);
		}
	}

	//�˴����ִ��ʧ�ܿ�����Ҫ�ٴ��������ݿ�
	// ����ģʽ
	map<string, string> mapObjPropName2Interval;
	nRet = LoadObjectPropIntervalConfig(PKDbApi, mapObjPropName2Interval);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "LoadObjectPropIntervalConfig failed! return:%d", nRet);
	}

	// �����tag��
	char szSql2[2048] = { 0 };
	strcpy(szSql2, "select OBJ.name as objname,PROP.name as propname,PROP.hisdata_interval, PROP.trenddata_interval\
		from t_object_list OBJ, t_class_prop PROP, t_class_list CLS\
		where OBJ.class_id = CLS.id and PROP.class_id = CLS.id and (OBJ.enable is NULL or OBJ.enable<>0 or OBJ.enable='') \
		and ( (PROP.hisdata_interval is not null and PROP.hisdata_interval  <> '' and PROP.hisdata_interval > 0) \
		or (PROP.trenddata_interval is not null and PROP.trenddata_interval  <> '' and PROP.trenddata_interval > 0) )");
	if(m_nVersion >= VERSION_PKHISDB_OBJECTINTERVAL)
		strcpy(szSql2, "select OBJ.name as objname,PROP.name as propname,PROP.hisdata_interval, PROP.trenddata_interval,OBJ.hisdata_interval, OBJ.trenddata_interval\
		from t_object_list OBJ, t_class_prop PROP, t_class_list CLS\
		where OBJ.class_id = CLS.id and PROP.class_id = CLS.id and (OBJ.enable is NULL or OBJ.enable<>0 or OBJ.enable='') \
		and ( (PROP.hisdata_interval is not null and PROP.hisdata_interval  <> '' and PROP.hisdata_interval > 0) \
		or (PROP.trenddata_interval is not null and PROP.trenddata_interval  <> '' and PROP.trenddata_interval > 0) )");

	nRet = PKDbApi.SQLExecute(szSql2, vecRow, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "execute query tags fromt_device_tag to save history data failed, sql:%s, error:%s", szSql2, strError.c_str());
	}
	else
	{
		// ����
		for (int iTab = 0; iTab < vecRow.size(); iTab++)
		{
			vector<string> &row = vecRow[iTab];
			int iCol = 0;
			string strObjectName = NULLASSTRING(row[iCol].c_str());
			iCol++;
			string strPropName = NULLASSTRING(row[iCol].c_str());
			iCol++;
			string strPropHisDataInterVal = NULLASSTRING(row[iCol].c_str());
			iCol++;
			string strPropTrendDataInterVal = NULLASSTRING(row[iCol].c_str());
			iCol++;
			string strObjHisDataInterVal = "";
			string strObjTrendDataInterVal = "";
			if (m_nVersion >= VERSION_PKHISDB_OBJECTINTERVAL)
			{
				strObjHisDataInterVal = NULLASSTRING(row[iCol].c_str());
				iCol++;
				strObjTrendDataInterVal = NULLASSTRING(row[iCol].c_str());
				iCol++;
			}

			if (strObjectName.empty() || strPropName.empty())
			{
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "MainTask::LoadConfig, object.name(%s) or propname(%s) is null, No.%d/%d, SQL:%s", strObjectName.c_str(), strPropName.c_str(), iTab, vecRow.size(), szSql);
				continue;
			}

			string strTagName = strObjectName + "." + strPropName;
 
			CTagInfo *pTag = new CTagInfo();
			pTag->strTagName = strTagName;
			string strHisDataTagProp = strTagName + ".hisdata_interval"; // objectname.propname.hisdata_interval
			string strTrendDataTagProp = strTagName + ".trenddata_interval"; // objectname.propname.trenddata_interval

			// ���������Ա�����û���Լ����õĶ�������ֵ
			map<string, string>::iterator itHisDataTag = mapObjPropName2Interval.find(strHisDataTagProp);
			string strHisDataInterVal;
			if (strPropHisDataInterVal.compare("0") != 0) // ������Բ�����Ϊ0, Ϊnull���߷�0ֵ��ȡ���������ֵ
				strHisDataInterVal = strObjHisDataInterVal;
			if (itHisDataTag != mapObjPropName2Interval.end()) // ��������������Լ������� 
				strHisDataInterVal = itHisDataTag->second; 
			if(strHisDataInterVal.compare("") == 0) // ���Ϊnull��˵����������Ծ�Ϊ���ã���ôȡȫ��
				pTag->nRecordIntervalSec = m_nRecordIntervalSec;
			else
				pTag->nRecordIntervalSec = ::atof(strHisDataInterVal.c_str()) * 60; // ���ӻ���Ϊ��

			if (pTag->nRecordIntervalSec > 0)
				nTagNumWithHisData++;

			// ���������Ա�����û���Լ����õĶ�������ֵ
			map<string, string>::iterator itTrendDataTag = mapObjPropName2Interval.find(strTrendDataTagProp);
			string strTrendDataInterVal;
			if (strPropTrendDataInterVal.compare("0") != 0) // ������Բ�����Ϊ0, Ϊnull���߷�0ֵ��ȡ���������ֵ
				strTrendDataInterVal = strObjTrendDataInterVal;
			if (itTrendDataTag != mapObjPropName2Interval.end()) // ��������������Լ������� 
				strTrendDataInterVal = itTrendDataTag->second;
			if (strTrendDataInterVal.compare("") == 0) // ���Ϊnull��˵����������Ծ�Ϊ���ã���ôȡȫ��
				pTag->nTrendIntervalSec = m_nTrendIntervalSec;
			else
				pTag->nTrendIntervalSec = ::atoi(strTrendDataInterVal.c_str()); // ��λ����������
			if (pTag->nTrendIntervalSec > 0)
				nTagNumWithTrend++;

			m_vecTags.push_back(pTag);
			mapObjPropName2Interval.clear();
		}
	}

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "Load Config over. tag count:%d������+�豸ģʽ��, tag to save trend:%d, tag to save history data:%d ", 
		m_vecTags.size(), nTagNumWithHisData, nTagNumWithTrend);
	if (nTagNumWithHisData == 0 || nTagNumWithTrend == 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "Load Config over. SQL: %s, SQL2: %s", szSql, szSql2);
	}
	PKDbApi.UnInitialize();
	return 0;
}

/*
hisdataserver=127.0.0.1:27017
*/
int CMainTask::ReadHisDatabaseUrl()
{
	// �õ������ļ���·��
	const char *szConfigPath = PKComm::GetConfigPath();
	CPKIniFile iniFile("db.conf", szConfigPath);
	string strHisDbIp = iniFile.GetString("hisdb", "ip", "");
	string strHisDbPort = iniFile.GetString("hisdb", "port", "");
	if (strHisDbIp.empty())
		strHisDbIp = PK_LOCALHOST_IP;
	if (strHisDbPort.empty())
	{
		char szPort[32];
		sprintf(szPort, "%d", PKHISDB_LISTEN_PORT);
		strHisDbPort = szPort;
	}

	char szMonConn[256];
	sprintf(szMonConn, "%s:%s", strHisDbIp.c_str(), strHisDbPort.c_str());
	m_strMongoConnString = szMonConn; // ȱʡ��IP�Ͷ˿�
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "MainTask, hisdb ip=%s, port=%s", strHisDbIp.c_str(), strHisDbPort.c_str());
	return 0;
}

int CMainTask::EnsureIndexOfExistTables(mongo::DBClientConnection & mongoConn)
{
	try
	{
		// ���������������
		//g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "EnsureIndexOfExistTables,create index of record_*,trend_*, index col={t,n}");
		int nTableCount = 0;
		int nTableExistIndex = 0;
		int nTableCreateFail = 0;
		int nTableNewIndex = 0;
		list<string> lCollections = mongoConn.getCollectionNames(MONGO_DB_NAME);
		for (list<string>::iterator itCol = lCollections.begin(); itCol != lCollections.end(); itCol++)
		{
			string strTableName = *itCol;
			if (strTableName.find("record_") != string::npos || strTableName.find("trend_") != string::npos)  // trend_YYYY-mm-dd  // trend_newest,trend_YYYY-mm-dd
			{
				nTableCount++;
				string strDBTable = MONGO_DB_NAME;
				strDBTable = strDBTable + "." + strTableName;
				std::list<BSONObj> listIndexBson = mongoConn.getIndexSpecs(strDBTable);
				bool bHaveExistIndex = false;
				for (std::list<BSONObj>::iterator itIndex = listIndexBson.begin(); itIndex != listIndexBson.end(); itIndex++)
				{
					BSONObj &indexBson = *itIndex;
					string strIndexName = indexBson.toString(); // { v: 2, key: { n: 1, t: -1 }, name: "n_1_t_-1", ns: "peak.record_2019_05_11" }
					if (strIndexName.find("n_1_t_-1") != string::npos)
					{
						bHaveExistIndex = true;
						break;
					}
				}

				if (bHaveExistIndex)
					nTableExistIndex++;
				else
				{
					int nRet = CreateTableIndex(mongoConn, strDBTable);
					if (nRet == 0)
					{
						nTableNewIndex++;
						nTableExistIndex++;
					}
					else
						nTableCreateFail++;
				}
			}
		}

		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "MainTask::EnsureIndexOfExistTables, Check Index of tables, table count:%d, withIndex count:%d, NewIndex:%d, Fail:%d", nTableCount, nTableExistIndex, nTableNewIndex, nTableCreateFail);
	}
	catch (const mongo::DBException &e)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "MainTask::EnsureIndexOfExistTables, except:%s", e.what());
		return -2;
	}
	return 0;
}

int CMainTask::CreateTableIndex(mongo::DBClientConnection & mongoConn, string &strTableName)
{
	try {
		mongo::IndexSpec oIds;
		mongo::IndexSpec::KeyVector keyVec;
		std::pair<std::string, mongo::IndexSpec::IndexType> index1_1("n", mongo::IndexSpec::kIndexTypeAscending);
		std::pair<std::string, mongo::IndexSpec::IndexType> index1_2("t", mongo::IndexSpec::kIndexTypeDescending);
		keyVec.push_back(index1_1);
		keyVec.push_back(index1_2);
		oIds.addKeys(keyVec);
		oIds.unique(false);
		//oIds.addKey("v");
		ACE_Time_Value tvBegin = ACE_OS::gettimeofday();
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "Begin to Create index({'n':1,'t':-1}) of table %s!", strTableName.c_str());
		mongoConn.createIndex(strTableName, oIds);
		ACE_Time_Value tvEnd = ACE_OS::gettimeofday();
		ACE_Time_Value tvSpan = tvEnd - tvBegin;
		ACE_UINT64 nMSec = tvSpan.get_msec();
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "Create index({'n':1,'t':-1}) of table %s success, consume:%d MS!", strTableName.c_str(), nMSec);
		return 0;
	}
	catch (DBException& e) {
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Create Index Exception(%s): %s", strTableName.c_str(), e.what());
		return -1;
	}
	catch (std::exception& e) {
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Create Index Exception(%s): %s", strTableName.c_str(), e.what());
		return -2;
	}
	catch (...) {
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Create Index Exception(%s): NULL", strTableName.c_str());
		return -3;
	}
}


// �Ƿ��mongo�ı��Ѿ�����������
//void CMainTask::EnsureTableIndex(mongo::DBClientConnection & mongoConn, string strTableName)
//{
//	map<string, bool>::iterator itTable = m_mapTableName2Index.find(strTableName);
//	if (itTable == m_mapTableName2Index.end()) {
//		int nRet = CreateTableIndex(mongoConn, strTableName);
//		if (nRet == 0) // ���������ɹ�
//			m_mapTableName2Index[strTableName] = true;
//	}
//}


int CMainTask::MaintainAllTables()
{
	time_t tmNow;
	time(&tmNow);

	if (labs(tmNow - m_tmLastMainData) < 60) // second
	{
		return 0;
	}

	m_tmLastMainData = tmNow;

	// ɾ��10����֮ǰ��������������
	time_t tmTenMinutesAo = tmNow - 600; // ʮ����ǰ��ʱ��
	char szDateTime[32] = { 0 };
	PKTimeHelper::Time2String(szDateTime, sizeof(szDateTime), tmTenMinutesAo);
	ACE_Time_Value tvBegin = ACE_OS::gettimeofday();
	BSONObjBuilder condBuilder;
	condBuilder.append("$lt", szDateTime);
	BSONObjBuilder objBuilder;
	objBuilder.append(REC_ATTR_TIME, condBuilder.obj()); // {"time":{"$gt":"2017-06-28 10:00:01","$lt":"2017-06-30 12:00:01"}}

	mongo::Query query(objBuilder.obj());
	this->m_mutexMongo.acquire();

	try
	{
		m_mongoConn.remove(MONGO_DB_TABLE_TREND_NEWEST, query); // ɾ��10����֮ǰ������. ��������ڻᴥ���쳣��
	}
	catch (const mongo::DBException &e)
	{
		m_mutexMongo.release();
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "MainTask::MaintainAllTables m_mongoConn.remove(MONGO_DB_TABLE_TREND_NEWEST) except:%s, continue...", e.what());
	}
	EnsureIndexOfExistTables(m_mongoConn); // ȷ��ÿ���������������
	this->m_mutexMongo.release();

	ACE_Time_Value tvEnd = ACE_OS::gettimeofday();
	ACE_Time_Value tvSpan = tvEnd - tvBegin;
	float fSec = tvSpan.get_msec() / 1000.0f;

	g_logger.LogMessage(PK_LOGLEVEL_INFO, "MainTask::MaintainAllTables, consume:%f ms", fSec);
	return 0;
}

/**
*  ���ܣ���ʼ��MongoDb���ݿ�;
*
*  @version   04/10/2017  xingxing Initial Version.
*/
int CMainTask::InitMongdb()
{
	try
	{
		mongo::client::initialize();
		m_mutexMongo.acquire();
		m_mongoConn.connect(MAIN_TASK->m_strMongoConnString.c_str());
 		m_mutexMongo.release();
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "MainTask::InitMongdb connect to pkhisdb successfully");

		MaintainAllTables();
	}
	catch (const mongo::DBException &e)
	{
		m_mutexMongo.release();
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "MainTask::InitMongdb connect to pkhisdb except:%s", e.what());
		return -2;
	}
	return PK_SUCCESS;
}