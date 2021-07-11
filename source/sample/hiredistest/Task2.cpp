#include "common/OS.h"
#include <ace/Default_Constants.h>
#include "ace/Thread_Mutex.h"
#include "ace/Guard_T.h"

#include "math.h"

#include "Task2.h"
#include "common/pkcomm.h"
#include "common/pkGlobalHelper.h"
#include "redisproxy/redisproxy.h"
#include "json/json.h"
#include "common/RMAPI.h"
#include "common/eviewdefine.h"
#include "common/pklog.h"

extern CRedisProxy		g_redisSub; // 订阅
extern CRedisProxy		g_redisRW; // 订阅

#define	DEFAULT_SLEEP_TIME		100	

/**
 *  构造函数.
 *
 *  @param  -[in,out]  CDriver*  pDriver: [comment]
 *
 *  @version     05/30/2008  lijingjing  Initial Version.
 */
CTask2::CTask2()
{
	m_bStop = false;
	m_bStopped = false;
}

/**
 *  析构函数.
 *
 *
 *  @version     05/30/2008  lijingjing  Initial Version.
 */
CTask2::~CTask2()
{
	
}

/**
 *  虚函数，继承自ACE_Task.
 *
 *
 *  @version     05/30/2008  lijingjing  Initial Version.
 */
int CTask2::svc()
{	
	ACE_High_Res_Timer::global_scale_factor();

	int nRet = 0;
	nRet = OnStart();

	char szTime[ICV_HOSTNAMESTRING_MAXLEN] = {'\0'};
	CVTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));

	string strControlsChannel = "task2";
	//nRet = g_redisRW.SubChannel(strControlsChannel);
	// 更新驱动的状态
	if(nRet != 0)
	{
	//	PKLog.LogMessage(PK_LOGLEVEL_ERROR, "process manager main task,Subscribe channle:%s failed,ret:%d", strControlsChannel.c_str(), nRet);
	}

	while(!m_bStop)
	{
		ACE_Time_Value tvSleep;
		tvSleep.msec(DEFAULT_SLEEP_TIME); // 100ms
		ACE_OS::sleep(tvSleep);		
		string strChannel, strMsg;
		//if(0 == g_redisRW.RecvChannelMsg(strChannel, strMsg, 100))
		//	printf("task1 recv pub:%s,%s \n", strChannel.c_str(), strMsg.c_str());

		string strVal;
		/*
		g_redisSub.get("task", strVal);
		printf("task2 getval(task=%s)\n", strVal.c_str());
		g_redisSub.set("task", "task2");
		g_redisSub.PublishMsg("task1", "from task2");
		*/
		g_redisRW.get("task", strVal);
		printf("task2 getval(task=%s)\n", strVal.c_str());
		g_redisRW.set("task", "task2");
		g_redisRW.PublishMsg("task1", "from task2");
	}

	PKLog.LogMessage(PK_LOGLEVEL_NOTICE,"Process Manager Maintask exit normally！");
	OnStop();

	m_bStopped = true;
	return PK_SUCCESS;	
}

int CTask2::OnStart()
{
	//int nRet = PK_SUCCESS;
	//nRet = m_redisSub.Initialize(PK_LOCALHOST_IP, PDB_LISTEN_PORT, PDB_ACCESS_PASSWORD);
	//if(nRet != PK_SUCCESS)
	//{
	//	PKLog.LogMessage(PK_LOGLEVEL_ERROR, "procmgr Main Task, Initialize Sub failed, ret:%d", nRet);
	//}
	//nRet = m_redisRW.Initialize(PK_LOCALHOST_IP, PDB_LISTEN_PORT, PDB_ACCESS_PASSWORD);
	//if(nRet != PK_SUCCESS)
	//{
	//	PKLog.LogMessage(PK_LOGLEVEL_ERROR, "procmgr Main Task, Initialize RW failed, ret:%d", nRet);
	//}

	return 0;
}

// 本任务停止时
void CTask2::OnStop()
{
	//m_redisSub.Finalize();
	//m_redisRW.Finalize();
}

int CTask2::Start()
{
	return activate(THR_NEW_LWP | THR_JOINABLE |THR_INHERIT_SCHED);
}

void CTask2::Stop()
{
	m_bStop = true;
	int nWaitResult = wait();

	ACE_Time_Value tvSleep;
	tvSleep.msec(100);
	while(!m_bStopped)
		ACE_OS::sleep(tvSleep);
}