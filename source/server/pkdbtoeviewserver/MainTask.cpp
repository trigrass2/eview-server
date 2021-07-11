/**************************************************************
*  Filename:    CtrlProcessTask.cpp
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: �����������;
*
*  @author:    xingxing;
*  @version    1.0.0
**************************************************************/
#include "common/OS.h"
#include <ace/Default_Constants.h>
#include "ace/Thread_Mutex.h"
#include "ace/Guard_T.h"

#include "pkcomm/pkcomm.h"
#include "pklog/pklog.h"
#include "pkdbapi/pkdbapi.h"
#include "MainTask.h"

extern CPKLog g_logger;

#define  CHECK_NULL_RETURN_ERR(X)  { if(!X) return EC_ICV_DA_CONFIG_FILE_STRUCTURE_ERROR; }
#define	DEFAULT_SLEEP_TIME		100	
#define NULLASSTRING(x) x==NULL?"":x
#define SQLBUFFERSIZE			1024


CMainTask::CMainTask()
{
	m_bStop = false;

	m_nPeriod = 10;	//��ѯ���ڣ�ȱʡΪ10��
	m_strEviewServerIp = "127.0.0.1";
	m_vecTag2StringPair.clear();
	m_hPkData = NULL;

	m_vecSQLToExecute.clear(); // �������±�������ʷ������

	m_tvLastQueryDB = ACE_Time_Value::zero;	// �ϴ���ѯ���ݿ��ʱ��
}

CMainTask::~CMainTask()
{
	m_vecTag2StringPair.clear();
	m_vecSQLToExecute.clear(); // �������±�������ʷ������

	m_bStop = true;
}

int CMainTask::LoadConfig()
{
	CPKIniFile iniFile;
	string strConfigFile = PKComm::GetProcessName();
	strConfigFile = strConfigFile + ".conf";
	string strConfigPath = PKComm::GetConfigPath();
	iniFile.SetFilePath(strConfigFile.c_str(), strConfigPath.c_str());
	m_nPeriod = iniFile.GetInt("database", "period", 10);
	if (m_nPeriod <= 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "��ȡ�����ļ�:%s,·��:%s,[database]/periodֵΪ:%d,�������0,����Ϊȱʡֵ:10��", strConfigFile.c_str(), strConfigPath.c_str(), m_nPeriod);
		m_nPeriod = 10;
	}

	m_strEviewServerIp = iniFile.GetString("eview", "serverip", "");
	if (m_strEviewServerIp.empty())
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "��ȡ�����ļ�:%s,·��:%s,[eview]/ipΪ��,����Ϊȱʡֵ:127.0.0.1", strConfigFile.c_str(), strConfigPath.c_str());
		m_strEviewServerIp = "127.0.0.1";
	}

	// ��ѯ���е�tag��
	vector<string> vecTagName;
	int nRet = iniFile.GetItemNames("tags", vecTagName);
	for (int i = 0; i < vecTagName.size(); i++)
	{
		string strTagName = vecTagName[i];
		string strTagSQL = iniFile.GetString("tags", strTagName.c_str(), "");
		if (strTagName.empty() || strTagSQL.empty())
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "��ȡ�����ļ�:%s,·��:%s,[tags], ��:%d��, ����:%s ��SQL:%s Ϊ��,����", strConfigFile.c_str(), strConfigPath.c_str(), strTagName.c_str(), strTagSQL.c_str());
			continue;
		}
		m_vecTag2StringPair.push_back(std::make_pair(strTagName, strTagSQL));
	}
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "��ȡ�����ļ�:%s,·��:%s,����:%d��,eview ip:%s, tag����:%d", strConfigFile.c_str(), strConfigPath.c_str(), m_nPeriod, m_strEviewServerIp.c_str(), m_vecTag2StringPair.size());
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "��ע������������:%s �ϵ�eview����", m_strEviewServerIp.c_str());

	return 0;
}

/**
*  ��ʼ�����������̺߳���ط���;
*
*  @return   0/1.
*
*  @version  10/14/2016	xingxing   Initial Version
*/

int CMainTask::svc()
{
	int nRet = 0;
	nRet = OnStart();
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "��ʼ��OnStartʧ��:%d, �˳�", nRet);
		ACE_Time_Value tmWait;
		tmWait.msec(500);
		OnStop();
		return PK_SUCCESS;
	}

	while (!m_bStop)
	{
		ACE_Time_Value tvNow = ACE_OS::gettimeofday();
		ACE_Time_Value tvSpan = tvNow - m_tvLastQueryDB;
		int nSpanSec = labs(tvSpan.sec());
		if (nSpanSec < m_nPeriod)
		{
			ACE_Time_Value tmWait;
			tmWait.msec(200);
			ACE_OS::sleep(tmWait);
			continue;
		}

		int nRet = QueryDBToTags();
		m_tvLastQueryDB = tvNow;
	}

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "MainTask exit normally!");
	OnStop();
	return PK_SUCCESS;
}

int CMainTask::OnStart()
{
	int nStatus = PK_SUCCESS;
	int nRet = 0;

	string strConfigFile = PKComm::GetProcessName();
	strConfigFile = strConfigFile + ".conf";
	nRet = LoadConfig();

	CPKDbApi pkdb;
	// ������ݿ�ĵ�¼
	nRet = pkdb.InitFromConfigFile(strConfigFile.c_str(), "database");
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "PKDBToEviewServer fail to init database. please configfile: %s �� databaseС������", strConfigFile.c_str());
		pkdb.UnInitialize();
		return -1;
	}

	g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "PKDBToEviewServer init db successful");
	pkdb.UnInitialize();

	m_hPkData = pkInit((char *)m_strEviewServerIp.c_str(), NULL, NULL);

	return nRet;
}

// ������ֹͣʱ
int CMainTask::OnStop()
{
	if (m_hPkData)
		pkExit(m_hPkData);
	m_hPkData = NULL;

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

int CMainTask::SetTagValueToOK(string strTagName, string strTagValue)
{
	pkUpdate(m_hPkData, (char *)strTagName.c_str(), (char *)strTagValue.c_str(), 0);
	return 0;
}

int CMainTask::SetTagValueToBad(string strTagName, string strBadValue)
{
	pkUpdate(m_hPkData, (char *)strTagName.c_str(), (char *)strBadValue.c_str(), -100);
	return 0;
}

int CMainTask::QueryDBToTags()
{
	if (m_vecTag2StringPair.size() <= 0)
		return 0;
	int nRet = 0;

	CPKDbApi pkdb;
	string strConfigFile = PKComm::GetProcessName();
	strConfigFile = strConfigFile + ".conf";
	// ������ݿ�ĵ�¼
	nRet = pkdb.InitFromConfigFile(strConfigFile.c_str(), "database");
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "PKDBToEviewServer fail to init database. please check configfile: %s �� databaseС������", strConfigFile.c_str());
		pkdb.UnInitialize();
		return -1;
	}

	for (int i = 0; i < m_vecTag2StringPair.size(); i++)
	{
		std::pair<string,string> p = m_vecTag2StringPair[i];
		string strTagName = p.first;
		string strTagSQL = p.second;
		vector<vector<string> > vecRows;
		string strSqlErr;
		nRet = pkdb.SQLExecute(strTagSQL.c_str(), vecRows, &strSqlErr);
		if (nRet != 0)
		{
			SetTagValueToBad(strTagName, "");
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "tag:%s, execute query SQL exception:%s, SQL:%s", strTagName.c_str(), strSqlErr.c_str(), strTagSQL.c_str());
			continue;
		}
		if (vecRows.size() <= 0)
		{
			SetTagValueToBad(strTagName, "");
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "tag:%s, query record by SQL is 0:%s", strTagName.c_str(), strTagSQL.c_str());
			continue;
		}
		vector<string> vecRow = vecRows[0];
		if (vecRow.size() <= 0)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "tag:%s, execute sql to query:%d rows, but first row has 0 cols. SQL:%s", strTagName.c_str(), vecRows.size(), strTagSQL.c_str());
			SetTagValueToBad(strTagName, "");
			vecRows.clear();
			continue;
		}

		string strTagValue = vecRow[0];
		SetTagValueToOK(strTagName, strTagValue);
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "tag:%s, execute sql to query:%d rows, row[0][0]:%s. SQL:%s", strTagName.c_str(), vecRows.size(), strTagValue.c_str(), strTagSQL.c_str());
		vecRow.clear();
		vecRows.clear();
	}

	pkdb.UnInitialize();
	return 0;
}
