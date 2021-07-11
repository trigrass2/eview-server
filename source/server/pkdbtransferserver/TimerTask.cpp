/**************************************************************
*  Filename:    TaskGroup.cpp
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: 设备组信息实体类.
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
*  构造函数.
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
*  析构函数.
*
*
*  @version     05/30/2008  lijingjing  Initial Version;
*/
CTimerTask::~CTimerTask()
{
     // 必须放在设备删除之后，否则设备和数据块中的ACE_Event_Handler会调用reator的Cancel_timer，导致reactor已删除的异常
    if (m_pReactor)
    {
        delete m_pReactor;
        m_pReactor = NULL;
    }
}

/**
*  启动线程;
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
    ACE_Task<ACE_MT_SYNCH>::reactor()->end_reactor_event_loop(); // 停止消息循环;
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

// Group线程启动时;
void CTimerTask::OnStart()
{
	// 初始化内存数据库;
	if (m_hPkData == NULL)
	{
		m_hPkData = pkInit("127.0.0.1", NULL, NULL);
		if (m_hPkData == NULL)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "CTimerTask::OnStart(), Init PKData failed");
			return;
		}
	}

	// 开启一个检查数据库连接的定时器;
	ACE_Time_Value	tvPollRate;		// 扫描周期，单位ms;
	//tvPollRate.msec(10000);		// 30秒检查1次所有的数据库连接
	tvPollRate.msec(5000);			// 30秒检查1次所有的数据库连接
	ACE_Time_Value tvPollPhase;
	tvPollPhase.msec(0);
	tvPollPhase += ACE_Time_Value::zero;
	m_pReactor->schedule_timer((ACE_Event_Handler *)this, NULL, tvPollPhase, tvPollRate);

	CheckAndConnectDBs(); // 先尝试连接1次所有的数据库
	AutoCreateStatTables(); // 建立统计需要的表，包括t_db_diff_xxxx(hour,day,month,year....)

	// 先启动按小时的定时器;
	for (vector<CHourTimer *>::iterator itTimer = m_vecHourTimer.begin(); itTimer != m_vecHourTimer.end(); itTimer++)
	{
		CHourTimer *pTimer = *itTimer;
		pTimer->StartTimer();
	}

	// 再启动;
	for (vector<CRuleTimer *>::iterator itTimer = m_vecRuleTimer.begin(); itTimer != m_vecRuleTimer.end(); itTimer++)
	{
		CRuleTimer *pTimer = *itTimer;
		pTimer->StartTimer();
	}
}

// Group线程停止时;
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

	ACE_Task<ACE_MT_SYNCH>::reactor()->end_reactor_event_loop(); // 停止消息循环
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
*  @version	8/20/2012  baoyuansong  修复判断队列中是否有数据判断条件错误，导致有多个消息时仅仅处理了最后一条消息.
*/
int CTimerTask::handle_output(ACE_HANDLE fd /*= ACE_INVALID_HANDLE*/)
{
    return 0;
}

// 这个定时器，仅仅用于更新：连接状态点、允许连接点;
int CTimerTask::handle_timeout(const ACE_Time_Value &current_time, const void *act /*= 0*/)
{
	CheckAndConnectDBs(); // 检查数据库连接，如果未连接上则进行连接;
	return 0;
}

// 检查每个数据库连接下配置的所有的转储表，查询select count(1) from {转储表名}, 如果有一个成功了，则认为连接正常。否则进行断开连接、重连操作
int CTimerTask::CheckAndConnectDBs()
{
	map<string, CDBConnection *> &mapId2DbConn = MAIN_TASK->m_mapId2DbConn;
	map<string, CDBConnection *>::iterator itDBConn = mapId2DbConn.begin();		// 所有的数据库连接;
	for (; itDBConn != mapId2DbConn.end(); itDBConn++)
	{
		CDBConnection *pDBConn = itDBConn->second;
		CPKDbApi &pkdb = pDBConn->m_pkdb;

		// 尝试连接
		//long lRet1 = pkdb.SQLConnect(pDBConn->m_strConnString.c_str(), pDBConn->m_strUserName.c_str(), pDBConn->m_strPassword.c_str(), pDBConn->m_nDBType, 2, pDBConn->m_strCodeset.c_str());

		//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
		if (!pkdb.IsConnected()) // 数据库肯定还没有连接上
		//if (false)
		{
			// 尝试连接
			long lRet = pkdb.SQLConnect(pDBConn->m_strConnString.c_str(), pDBConn->m_strUserName.c_str(), pDBConn->m_strPassword.c_str(), pDBConn->m_nDBType, 2, pDBConn->m_strCodeset.c_str());
			//long lRet = pkdb.SQLConnect("127.0.0.1@MyDatabase", "sa", "123456", PK_DATABASE_TYPEID_SQLSERVER, 1000, "utf8");

			if (lRet == 0)
			{
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "连接id:%s, name:%s 的数据库连接成功!", pDBConn->m_strId.c_str(), pDBConn->m_strName.c_str());
				continue;
			}

			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "连接id:%s, name:%s 的数据库连接失败, 返回值:%d, 请检查数据用户、密码、连接串等配置参数!", pDBConn->m_strId.c_str(), pDBConn->m_strName.c_str(), lRet);
			continue;
		}

		// 数据库内部变量为连接上了，但也可能中途断开了，需要检查;
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

		// 尝试连接
		long lRet = pkdb.SQLConnect(pDBConn->m_strConnString.c_str(), pDBConn->m_strUserName.c_str(), pDBConn->m_strPassword.c_str(), pDBConn->m_nDBType, 2, pDBConn->m_strCodeset.c_str());
		if (lRet == 0)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "连接id:%s, name:%s 的数据库连接成功!", pDBConn->m_strId.c_str(), pDBConn->m_strName.c_str());
			continue;
		}

		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "连接id:%s, name:%s 的数据库连接失败, 错误:%d, 请检查数据用户、密码、连接串等配置参数!", pDBConn->m_strId.c_str(), pDBConn->m_strName.c_str(), lRet);
	}

	return 0;
}

int CTimerTask::AutoCreateStatTables()
{
	map<string, CDBConnection *> &mapId2DbConn = MAIN_TASK->m_mapId2DbConn;
	map<string, CDBConnection *>::iterator itDBConn = mapId2DbConn.begin();		// 所有的数据库连接;
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
	if (pDBCon->m_mapTableCreated2Time.find(strTableName) != pDBCon->m_mapTableCreated2Time.end()) // 已经有这个表了
		return 0;

	// 创建失败，查询该表是否存在
	char szSql[4096];
	sprintf(szSql, "select count(1) from %s", strTableName.c_str());
	string strError;
	int nRet = pDBCon->m_pkdb.SQLExecute(szSql, &strError);
	if (nRet == 0) /// 查询成功，说明存在此表
	{
		pDBCon->m_mapTableCreated2Time[strTableName] = "";
		return 0;
	}

	// 无此表，需要创建
	nRet = pDBCon->m_pkdb.SQLExecute(strCreateSQL.c_str(), &strError);
	if (nRet == 0) /// 创建成功
	{
		pDBCon->m_mapTableCreated2Time[strTableName] = "";
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "自动创建表: %s 成功!", strTableName.c_str());
		return 0;
	}

	// 无此表，创建也失败
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "表: %s 不存在，创建失败!", strTableName.c_str());

	return -1;
}