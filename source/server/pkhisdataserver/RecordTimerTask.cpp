/**************************************************************
 *  Filename:    ExecutionTak.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 执行预案的线程，接收的消息包括触发源触发，动作执行完毕等.
 *
 *  @author:     shijunpu
 *  @version     05/22/2008  shijunpu   Initial Version
**************************************************************/
#include "RecordTimerTask.h" // 必须放在最前面，否则mongo 编译错误

#include "ace/High_Res_Timer.h"
#include <fstream>  
#include <streambuf>  
#include <iostream>  

#include "pkeviewdbapi/pkeviewdbapi.h"
#include "pklog/pklog.h"
#include "json/json.h"
#include "common/eviewdefine.h"
#include "pkdata/pkdata.h"	
#include "RecordTimer.h"

#include "MainTask.h"
#include "global.h"

using namespace std;
using namespace mongo;

#ifndef __sun
using std::map;
using std::string;
#endif

#define NULLASSTRING(x) x==NULL?"":x
extern CPKLog g_logger;
int GetPKMemDBPort(int &nListenPort, string &strPassword);

CRecordTimerTask::CRecordTimerTask() :m_mongoConn(true)
{
	m_bStop = false;
	ACE_Select_Reactor *pSelectReactor = new ACE_Select_Reactor();
	m_pReactor = new ACE_Reactor(pSelectReactor, true);
	this->reactor(m_pReactor);
}

CRecordTimerTask::~CRecordTimerTask()
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
long CRecordTimerTask::StartTask()
{
	long lRet = (long)activate(THR_NEW_LWP | THR_JOINABLE |THR_INHERIT_SCHED, 1);
	if(lRet != PK_SUCCESS)
	{
		lRet = -2;
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, _("CRecordTimerTask: StartTask() Failed, RetCode: %d"), lRet);
	}
	else
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, _("CRecordTimerTask: StartTask() success"));
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
long CRecordTimerTask::StopTask()
{
	m_bStop = true;
	StopTimers(); // 必须先停止定时器
	this->reactor()->end_reactor_event_loop();
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "MainTask::StopTask()...");
	int nWaitResult = wait(); // 这个等待会立马返回？？？？？？
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "MainTask::StopTask() return");

	return PK_SUCCESS;	
}

/*从触发事件发过来的事件信息
 *  main procedure of the task.
 *	get a msgblk and 
 *  @return 
 *	- ==0 success
 *	- !=0 failure
 *
 *  @version     05/23/2008  shijunpu  Initial Version.

 //启动 EventTypeTaskManager ActionTypeTaskManagers
 */
int CRecordTimerTask::svc()
{
	g_logger.LogMessage(PK_LOGLEVEL_INFO, _("MainTask::svc() begin"));
	
	ACE_High_Res_Timer::global_scale_factor();
	this->reactor()->owner(ACE_OS::thr_self ());

	int nRet = PK_SUCCESS;
	nRet = OnStart();

	while(!m_bStop)
	{
		this->reactor()->reset_reactor_event_loop();
		this->reactor()->run_reactor_event_loop();
	}

	g_logger.LogMessage(PK_LOGLEVEL_INFO,"CRecordTimerTask stopped");
	OnStop();

	return PK_SUCCESS;	
}

int CRecordTimerTask::OnStart()
{
	// 这个定时器，是定时检查是否要删除文件和连接是否成功的定时器
	m_pReactor->cancel_timer((ACE_Event_Handler *)this);
	ACE_Time_Value	tvCheckConn;		// 扫描周期，单位ms
	tvCheckConn.msec(10 * 1000); 
	ACE_Time_Value tvStart = ACE_Time_Value::zero + ACE_Time_Value(5); // 从第0:0:5秒开始检查，以便让0:0:0秒的数据处理完 

	ACE_Timer_Queue *timer_queue = m_pReactor->timer_queue();
	if (0 != timer_queue)
		timer_queue->schedule((ACE_Event_Handler *)this, NULL, tvStart, tvCheckConn);

	// m_pReactor->schedule_timer((ACE_Event_Handler *)this, NULL, tvStart, tvCheckConn);

	// 初始化内存数据库.必须放在初始化mongodb之前
	int nRet = InitRedis();
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "RecordTimerTask::OnStart,connect pkmemdb failed! please check pkmemdb started and ip&port is correct");
		//return nRet; 如果退出后面就无法执行了！
	}
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "RecordTimerTask::OnStart,init pkmemdb successfully!");

	// 生成定时器
	nRet = GenerateTimersByTagInfo();
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "RecordTimerTask::OnStart, generate %d record timers failed,retcode：%d", m_mapInterval2TagGroupTimer.size(), nRet);
		// return nRet; 如果退出后面就无法执行了！
	}
	else
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "RecordTimerTask::OnStart, generate %d record timers successfully", m_mapInterval2TagGroupTimer.size());

	//连接Mongodb数据库;
	nRet = InitMongdb();
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "RecordTimerTask::OnStart, failed to connect to PKHisDB，check pkhisdb started first");
		//return nRet; 如果退出后面就无法执行了！
	}
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "RecordTimerTask::OnStart, init PKHisDB successful(OnStart)");

	StartTimers();

	return 0;
}

int CRecordTimerTask::OnStop()
{
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "RecordTimerTask::OnStop begin.");
	m_pReactor->close();
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "RecordTimerTask::OnStop Reactor->close().");
	// 停止其他线程;
	m_redisRW.finalize();
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "RecordTimerTask::OnStop end.");
	return 0;
}

int CRecordTimerTask::StopTimers()
{
	// 先取消定时器，免得累计了很多定时器消息导致无法退出
	std::map<int, CRecordTimer *>::iterator itRecordTimer = m_mapInterval2TagGroupTimer.begin();
	for (; itRecordTimer != m_mapInterval2TagGroupTimer.end(); itRecordTimer++)
	{
		CRecordTimer *pTimer = itRecordTimer->second;
		pTimer->Stop();
	}
	return 0;
}

int CRecordTimerTask::GenerateTimersByTagInfo()
{
	//读取TAG点，并分组到CRecordTimer;
	vector<CTagInfo *>::iterator itTag;
	for (itTag = MAIN_TASK->m_vecTags.begin(); itTag != MAIN_TASK->m_vecTags.end(); itTag++)
	{
		CTagInfo *pTag = *itTag;
		if (pTag->nRecordIntervalSec <= 0)
			continue;

		// 判断并加入历史记录定时器组
		CRecordTimer *pRecordTimer = NULL;
		map<int, CRecordTimer *>::iterator itRecordTimer = m_mapInterval2TagGroupTimer.find(pTag->nRecordIntervalSec);
		if (itRecordTimer == m_mapInterval2TagGroupTimer.end())
		{
			pRecordTimer = new CRecordTimer();
			pRecordTimer->SetTimerTask(this);
			pRecordTimer->m_nIntervalMS = pTag->nRecordIntervalSec * 1000;// 单位为毫秒
			m_mapInterval2TagGroupTimer[pTag->nRecordIntervalSec] = pRecordTimer;
		}
		else
			pRecordTimer = itRecordTimer->second;

		pRecordTimer->m_vecTags.push_back(pTag);
		pRecordTimer->m_vecTagName.push_back(pTag->strTagName);
	}

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "GenerateTimersByTagInfo, record %d timers", m_mapInterval2TagGroupTimer.size());

	return 0;
}


int CRecordTimerTask::StartTimers()
{
	// 先取消历史记录定时器，免得累计了很多定时器消息导致无法退出;
	std::map<int, CRecordTimer *>::iterator itRecordTimer = m_mapInterval2TagGroupTimer.begin();
	for (; itRecordTimer != m_mapInterval2TagGroupTimer.end(); itRecordTimer++)
	{
		CRecordTimer *pTimer = itRecordTimer->second;
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "generate record timer, interval :%d second, tag count:%d", pTimer->m_nIntervalMS/1000, pTimer->m_vecTags.size());
		int nRet = pTimer->Start();
	}
	return 0;
}

/**
*  功能：初始化MongoDb数据库;
*
*  @version   04/10/2017  xingxing Initial Version.
*/
int CRecordTimerTask::InitMongdb()
{
	try
	{ 
		MAIN_TASK->m_mutexMongo.acquire();
		m_mongoConn.connect(MAIN_TASK->m_strMongoConnString.c_str());
		MAIN_TASK->m_mutexMongo.release();
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "RecordTimerTask::InitMongdb, connect to pkhisdb successfully");
	}
	catch (const mongo::DBException &e)
	{
		MAIN_TASK->m_mutexMongo.release();
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "RecordTimerTask::InitMongdb, connect to pkhisdb except:%s", e.what());
		return -2;
	} 
	return PK_SUCCESS;
}
/**
*  功能：初始化Redis.
*
*  @version     04/10/2017  xingxing  Initial Version.
*/
int CRecordTimerTask::InitRedis()
{
	int nRet = 0;

	int nPKMemDBListenPort = PKMEMDB_LISTEN_PORT;
	string strPKMemDBPassword = PKMEMDB_ACCESS_PASSWORD;
	GetPKMemDBPort(nPKMemDBListenPort, strPKMemDBPassword);

	nRet = m_redisRW.initialize(PK_LOCALHOST_IP, nPKMemDBListenPort, strPKMemDBPassword.c_str(), 0);
	if (nRet != PK_SUCCESS)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "MainTask m_redisRW.Initialize failed,ret:%d", nRet);
		return nRet;
	}
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "record, Init PKMemDB success");
	return nRet;
}
