/**************************************************************
*  Filename:    TaskGroup.cpp
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: �豸����Ϣʵ����.
*
*  @author:     lijingjing
*  @version     05/28/2008  lijingjing  Initial Version
**************************************************************/
#include "ace/High_Res_Timer.h"
#include "TimerTask.h"
#include "common/CommHelper.h"
#include "MainTask.h"
#include "pklog/pklog.h"
#include "pkcomm/pkcomm.h"
#include <stdlib.h>
#include <cmath>
#include "HourTimer.h"
#include "RuleTimer.h"

extern	CPKLog			g_logger;

#ifndef MAX_PATH
#define MAX_PATH		260
#endif // MAX_PATH

/**
*  ���캯��.
*
*
*  @version     05/30/2008  lijingjing  Initial Version.
*/
CTimerTask::CTimerTask()
{
    ACE_Select_Reactor *pSelectReactor = new ACE_Select_Reactor();
    m_pReactor = new ACE_Reactor(pSelectReactor, true);
	ACE_Task<ACE_MT_SYNCH>::reactor(m_pReactor);
	m_hPkData = NULL;
}


/**
*  ��������.
*
*
*  @version     05/30/2008  lijingjing  Initial Version;
*/
CTimerTask::~CTimerTask()
{
     // ��������豸ɾ��֮�󣬷����豸�����ݿ��е�ACE_Event_Handler�����reator��Cancel_timer������reactor��ɾ�����쳣
    if (m_pReactor)
    {
        delete m_pReactor;
        m_pReactor = NULL;
    }
}

/**
*  �����߳�;
*
*
*  @version     07/09/2008  lijingjing  Initial Version;
*/
int CTimerTask::Start()
{
    m_bExit = false;
    int nRet = this->activate(THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED);
    if (0 == nRet)
        return PK_SUCCESS;

    return nRet;
}

void CTimerTask::Stop()
{
    this->m_bExit = true;
    ACE_Task<ACE_MT_SYNCH>::reactor()->end_reactor_event_loop(); // ֹͣ��Ϣѭ��;
    this->wait();
    g_logger.LogMessage(PK_LOGLEVEL_WARN, "TimerTask thread exited!");
}

int CTimerTask::svc()
{
	ACE_Task<ACE_MT_SYNCH>::reactor()->owner(ACE_OS::thr_self());
    OnStart();
    while (!m_bExit)
    {
		ACE_Task<ACE_MT_SYNCH>::reactor()->reset_reactor_event_loop();
		ACE_Task<ACE_MT_SYNCH>::reactor()->run_reactor_event_loop();
    }
    OnStop();
    return 0;
}

// Group�߳�����ʱ;
void CTimerTask::OnStart()
{
	// ��ʼ���ڴ����ݿ�;
	if (m_hPkData == NULL)
	{
		m_hPkData = pkInit("127.0.0.1", NULL, NULL);
		if (m_hPkData == NULL)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "CTimerTask::OnStart(), Init PKData failed");
			return;
		}
	}

	// ����һ��������ݿ����ӵĶ�ʱ��;
	ACE_Time_Value	tvPollRate;		// ɨ�����ڣ���λms;
	//tvPollRate.msec(10000);		// 30����1�����е����ݿ�����
	tvPollRate.msec(5000);			// 30����1�����е����ݿ�����
	ACE_Time_Value tvPollPhase;
	tvPollPhase.msec(0);
	tvPollPhase += ACE_Time_Value::zero;
	m_pReactor->schedule_timer((ACE_Event_Handler *)this, NULL, tvPollPhase, tvPollRate);

	CheckAndConnectDBs(); // �ȳ�������1�����е����ݿ�
	AutoCreateStatTables(); // ����ͳ����Ҫ�ı�����t_db_diff_xxxx(hour,day,month,year....)

	// ��������Сʱ�Ķ�ʱ��;
	for (vector<CHourTimer *>::iterator itTimer = m_vecHourTimer.begin(); itTimer != m_vecHourTimer.end(); itTimer++)
	{
		CHourTimer *pTimer = *itTimer;
		pTimer->StartTimer();
	}

	// ������;
	for (vector<CRuleTimer *>::iterator itTimer = m_vecRuleTimer.begin(); itTimer != m_vecRuleTimer.end(); itTimer++)
	{
		CRuleTimer *pTimer = *itTimer;
		pTimer->StartTimer();
	}
}

// Group�߳�ֹͣʱ;
void CTimerTask::OnStop()
{
	m_pReactor->cancel_timer((ACE_Event_Handler *)this);
	
	for (vector<CRuleTimer *>::iterator itTimer = m_vecRuleTimer.begin(); itTimer != m_vecRuleTimer.end(); itTimer++)
    {
		CRuleTimer *pTimer = *itTimer;
		pTimer->StopTimer();
    }

	for (vector<CHourTimer *>::iterator itTimer = m_vecHourTimer.begin(); itTimer != m_vecHourTimer.end(); itTimer++)
	{
		CHourTimer *pTimer = *itTimer;
		pTimer->StopTimer();
	}

	ACE_Task<ACE_MT_SYNCH>::reactor()->end_reactor_event_loop(); // ֹͣ��Ϣѭ��
    m_pReactor->close();

	pkExit(m_hPkData);
}

/**
*  $(Desp) .
*  $(Detail) .
*
*  @param		-[in]  ACE_HANDLE fd : [comment]
*  @return		int.
*
*  @version	11/20/2012  shijunpu  Initial Version.
*  @version	8/20/2012  baoyuansong  �޸��ж϶������Ƿ��������ж��������󣬵����ж����Ϣʱ�������������һ����Ϣ.
*/
int CTimerTask::handle_output(ACE_HANDLE fd /*= ACE_INVALID_HANDLE*/)
{
    return 0;
}

// �����ʱ�����������ڸ��£�����״̬�㡢�������ӵ�;
int CTimerTask::handle_timeout(const ACE_Time_Value &current_time, const void *act /*= 0*/)
{
	CheckAndConnectDBs(); // ������ݿ����ӣ����δ���������������;
	return 0;
}

// ���ÿ�����ݿ����������õ����е�ת������ѯselect count(1) from {ת������}, �����һ���ɹ��ˣ�����Ϊ����������������жϿ����ӡ���������
int CTimerTask::CheckAndConnectDBs()
{
	map<string, CDBConnection *> &mapId2DbConn = MAIN_TASK->m_mapId2DbConn;
	map<string, CDBConnection *>::iterator itDBConn = mapId2DbConn.begin();		// ���е����ݿ�����;
	for (; itDBConn != mapId2DbConn.end(); itDBConn++)
	{
		CDBConnection *pDBConn = itDBConn->second;
		CPKDbApi &pkdb = pDBConn->m_pkdb;

		// ��������
		//long lRet1 = pkdb.SQLConnect(pDBConn->m_strConnString.c_str(), pDBConn->m_strUserName.c_str(), pDBConn->m_strPassword.c_str(), pDBConn->m_nDBType, 2, pDBConn->m_strCodeset.c_str());

		//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
		if (!pkdb.IsConnected()) // ���ݿ�϶���û��������
		//if (false)
		{
			// ��������
			long lRet = pkdb.SQLConnect(pDBConn->m_strConnString.c_str(), pDBConn->m_strUserName.c_str(), pDBConn->m_strPassword.c_str(), pDBConn->m_nDBType, 2, pDBConn->m_strCodeset.c_str());
			//long lRet = pkdb.SQLConnect("127.0.0.1@MyDatabase", "sa", "123456", PK_DATABASE_TYPEID_SQLSERVER, 1000, "utf8");

			if (lRet == 0)
			{
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "����id:%s, name:%s �����ݿ����ӳɹ�!", pDBConn->m_strId.c_str(), pDBConn->m_strName.c_str());
				continue;
			}

			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "����id:%s, name:%s �����ݿ�����ʧ��, ����ֵ:%d, ���������û������롢���Ӵ������ò���!", pDBConn->m_strId.c_str(), pDBConn->m_strName.c_str(), lRet);
			continue;
		}

		// ���ݿ��ڲ�����Ϊ�������ˣ���Ҳ������;�Ͽ��ˣ���Ҫ���;
		bool bConnected = false;
		vector<CDBTransferRule *>::iterator itDBRule = pDBConn->m_vecTransferRule.begin();
		for (; itDBRule != pDBConn->m_vecTransferRule.end(); itDBRule ++)
		{
			CDBTransferRule *pDBRule = *itDBRule;
			if (pkdb.TestConnected(pDBRule->m_strTableName.c_str()) == 0)
			{
				g_logger.LogMessage(PK_LOGLEVEL_INFO, "check connection id:%s, name:%s database, connected!", pDBConn->m_strId.c_str(), pDBConn->m_strName.c_str());
				bConnected = true;
				break;
			}
			else
			{
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "check connection id:%s, name:%s database, disconnected!", pDBConn->m_strId.c_str(), pDBConn->m_strName.c_str());

			}
		}

		if (bConnected)
		{
			continue;
		}

		// ��������
		long lRet = pkdb.SQLConnect(pDBConn->m_strConnString.c_str(), pDBConn->m_strUserName.c_str(), pDBConn->m_strPassword.c_str(), pDBConn->m_nDBType, 2, pDBConn->m_strCodeset.c_str());
		if (lRet == 0)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "����id:%s, name:%s �����ݿ����ӳɹ�!", pDBConn->m_strId.c_str(), pDBConn->m_strName.c_str());
			continue;
		}

		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "����id:%s, name:%s �����ݿ�����ʧ��, ����:%d, ���������û������롢���Ӵ������ò���!", pDBConn->m_strId.c_str(), pDBConn->m_strName.c_str(), lRet);
	}

	return 0;
}

int CTimerTask::AutoCreateStatTables()
{
	map<string, CDBConnection *> &mapId2DbConn = MAIN_TASK->m_mapId2DbConn;
	map<string, CDBConnection *>::iterator itDBConn = mapId2DbConn.begin();		// ���е����ݿ�����;
	for (; itDBConn != mapId2DbConn.end(); itDBConn++)
	{
		CDBConnection *pDBCon = itDBConn->second;
		string strTableName = "t_db_diff_hour";
		string strTableCreateSQL = "create table t_db_diff_hour(name varchar(128), calc_time varchar(20), calc_value varchar(20), begin_rawvalue varchar(20), end_rawvalue varchar(20), create_time varchar(20), update_time varchar(20), primary key(name,calc_time));";
		AutoCreateTable(pDBCon, strTableName, strTableCreateSQL);

		strTableName = "t_db_diff_day";
		strTableCreateSQL = "create table t_db_diff_day(name varchar(128), calc_time varchar(20), calc_value varchar(20), begin_rawvalue varchar(20), end_rawvalue varchar(20), create_time varchar(20), update_time varchar(20), primary key(name,calc_time));";
		AutoCreateTable(pDBCon, strTableName, strTableCreateSQL);

		strTableName = "t_db_diff_week";
		strTableCreateSQL = "create table t_db_diff_week(name varchar(128), calc_time varchar(20), calc_value varchar(20), begin_rawvalue varchar(20), end_rawvalue varchar(20), create_time varchar(20), update_time varchar(20), primary key(name,calc_time));";
		AutoCreateTable(pDBCon, strTableName, strTableCreateSQL);

		strTableName = "t_db_diff_month";
		strTableCreateSQL = "create table t_db_diff_month(name varchar(128), calc_time varchar(20), calc_value varchar(20), begin_rawvalue varchar(20), end_rawvalue varchar(20), create_time varchar(20), update_time varchar(20), primary key(name,calc_time));";
		AutoCreateTable(pDBCon, strTableName, strTableCreateSQL);

		strTableName = "t_db_diff_year";
		strTableCreateSQL = "create table t_db_diff_year(name varchar(128), calc_time varchar(20), calc_value varchar(20), begin_rawvalue varchar(20), end_rawvalue varchar(20), create_time varchar(20), update_time varchar(20), primary key(name,calc_time));";
		AutoCreateTable(pDBCon, strTableName, strTableCreateSQL);
	}
	return 0;
}

int CTimerTask::AutoCreateTable(CDBConnection *pDBCon, string strTableName, string strCreateSQL)
{
	if (pDBCon->m_mapTableCreated2Time.find(strTableName) != pDBCon->m_mapTableCreated2Time.end()) // �Ѿ����������
		return 0;

	// ����ʧ�ܣ���ѯ�ñ��Ƿ����
	char szSql[4096];
	sprintf(szSql, "select count(1) from %s", strTableName.c_str());
	string strError;
	int nRet = pDBCon->m_pkdb.SQLExecute(szSql, &strError);
	if (nRet == 0) /// ��ѯ�ɹ���˵�����ڴ˱�
	{
		pDBCon->m_mapTableCreated2Time[strTableName] = "";
		return 0;
	}

	// �޴˱���Ҫ����
	nRet = pDBCon->m_pkdb.SQLExecute(strCreateSQL.c_str(), &strError);
	if (nRet == 0) /// �����ɹ�
	{
		pDBCon->m_mapTableCreated2Time[strTableName] = "";
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "�Զ�������: %s �ɹ�!", strTableName.c_str());
		return 0;
	}

	// �޴˱�����Ҳʧ��
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "��: %s �����ڣ�����ʧ��!", strTableName.c_str());

	return -1;
}