/**************************************************************
 *  Filename:    CtrlProcessTask.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 控制命令处理类.
 *
 *  @author:    lijingjing
 *  @version    05/28/2008  lijingjing  Initial Version
 *  @version	9/17/2012  baoyuansong  数据块类型修改为字符串类型，并且修改增加数据块时的接口
 *  @version	3/1/2013   baoyuansong  增加由点生成块的逻辑，当设备下没有配置数据块时，直接由点生成数据块 .
**************************************************************/

#include "common/OS.h"
#include <ace/Default_Constants.h>
#include "ace/Thread_Mutex.h"
#include "ace/Guard_T.h"

#include "math.h"

#include "Task1.h"
#include "common/pkcomm.h"
#include "common/pkGlobalHelper.h"
#include "redisproxy/redisproxy.h"
#include "json/json.h"
#include "common/RMAPI.h"
#include "common/eviewdefine.h"
#include "common/pklog.h"

extern CRedisProxy		g_redisSub; // 订阅
extern CRedisProxy		g_redisRW; // 订阅

#define	DEFAULT_SLEEP_TIME		10

/**
 *  构造函数.
 *
 *  @param  -[in,out]  CDriver*  pDriver: [comment]
 *
 *  @version     05/30/2008  lijingjing  Initial Version.
 */
CTask1::CTask1()
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
CTask1::~CTask1()
{
	
}

/**
 *  虚函数，继承自ACE_Task.
 *
 *
 *  @version     05/30/2008  lijingjing  Initial Version.
 */
int CTask1::svc()
{	
	ACE_High_Res_Timer::global_scale_factor();

	int nRet = 0;
	nRet = OnStart();

	char szTime[ICV_HOSTNAMESTRING_MAXLEN] = {'\0'};
	CVTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));

	string strControlsChannel = "task1";
	//nRet = g_redisSub.SubChannel(strControlsChannel);
	// 更新驱动的状态
	//if(nRet != 0)
	{
	//	PKLog.LogMessage(PK_LOGLEVEL_ERROR, "process manager main task,Subscribe channle:%s failed,ret:%d", strControlsChannel.c_str(), nRet);
	}

	while(!m_bStop)
	{
		ACE_Time_Value tvSleep;
		tvSleep.msec(DEFAULT_SLEEP_TIME); // 100ms
		ACE_OS::sleep(tvSleep);		
		string strChannel, strMsg;
		if(0 == g_redisSub.RecvChannelMsg(strChannel, strMsg,100))
			printf("task1 recv pub:%s,%s \n", strChannel.c_str(), strMsg.c_str());

		string strVal;
		/*g_redisSub.get("task", strVal);
		printf("task getval(task=%s)\n", strVal.c_str());
		g_redisSub.set("task", "task1");
		g_redisSub.PublishMsg("task2", "from task1");
		*/
		g_redisRW.get("task", strVal);
		printf("task getval(task=%s)\n", strVal.c_str());
		g_redisRW.set("task", "task1");
		g_redisRW.PublishMsg("task2", "publishi from task1");
	}

	PKLog.LogMessage(PK_LOGLEVEL_NOTICE,"Process Manager Maintask exit normally！");
	OnStop();

	m_bStopped = true;
	return PK_SUCCESS;	
}

int CTask1::OnStart()
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
void CTask1::OnStop()
{
	//m_redisSub.Finalize();
	//m_redisRW.Finalize();
}

int CTask1::Start()
{
	return activate(THR_NEW_LWP | THR_JOINABLE |THR_INHERIT_SCHED);
}

void CTask1::Stop()
{
	m_bStop = true;
	int nWaitResult = wait();

	ACE_Time_Value tvSleep;
	tvSleep.msec(100);
	while(!m_bStopped)
		ACE_OS::sleep(tvSleep);
}