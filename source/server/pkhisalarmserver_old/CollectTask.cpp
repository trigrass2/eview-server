/**************************************************************
 *  Filename:    CtrlProcessTask.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 控制命令处理类.
 *
 *  @author:    shijunpu
 *  @version    05/28/2008  确认和删除报警也放在控制里面进行执行。这里也是二道贩子，将其转发给数据处理就完事
**************************************************************/

#include "common/OS.h"
#include <ace/Default_Constants.h>
#include "ace/Thread_Mutex.h"
#include "ace/Guard_T.h"

#include "math.h"

#include "CollectTask.h"
#include "common/pkcomm.h"
#include "common/pkGlobalHelper.h"
#include "redisproxy/redisproxy.h"
#include "json/json.h"
#include "common/RMAPI.h"
#include "common/pktagdef.h"
#include "pklog/pklog.h"
#include "HisAlarmServer.h"
#include "CollectTask.h"

extern CHisAlarmServer* g_pServer;
extern map<string, CTag *>	g_mapName2Tag;

#define	DEFAULT_SLEEP_TIME		100	

/**
 *  构造函数.
 *
 *  @param  -[in,out]  CDriver*  pDriver: [comment]
 *
 *  @version     05/30/2008  lijingjing  Initial Version.
 */
CCollectTask::CCollectTask()
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
CCollectTask::~CCollectTask()
{
	
}

/**
 *  虚函数，继承自ACE_Task.
 *
 *
 *  @version     05/30/2008  lijingjing  Initial Version.
 */
int CCollectTask::svc()
{	
	ACE_High_Res_Timer::global_scale_factor();

	int nRet = 0;
	nRet = OnStart();

	char szTime[ICV_HOSTNAMESTRING_MAXLEN] = {'\0'};
	PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));

	while(!m_bStop)
	{
		ACE_Time_Value tvSleep;
		tvSleep.msec(DEFAULT_SLEEP_TIME); // 100ms
		ACE_OS::sleep(tvSleep);		

		// channelname:simensdrv,value:[{"name":tag1,"value":"100"},{"name":tag2,"value":"100"}]
		string strControlsChannel = CHANNELNAME_CONTROL;
		nRet = m_redisSub.SubChannel(strControlsChannel);
		// 更新驱动的状态
		if(nRet != 0)
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "Control process task,Subscribe channle:%s failed,ret:%d", strControlsChannel.c_str(), nRet);
			g_pServer->UpdateServerStatus(-2,"subscribe control channel failed");
		}

		while(!m_bStop)
		{
			string strChannel = "";
			string strMessage;
			if(m_redisSub.RecvChannelMsg(strChannel, strMessage,100) != 0) // 未收到消息
			{
				ACE_OS::sleep(tvSleep);
				continue;
			}

			PKLog.LogMessage(PK_LOGLEVEL_INFO, "CollectTask recv an controls msg!");
			Json::Reader reader;
			Json::Value root;
			if (!reader.parse(strMessage, root, false))
			{
				PKLog.LogMessage(PK_LOGLEVEL_ERROR, "CollectTask recv an msg(%s), invalid!",strMessage.c_str());
				g_pServer->UpdateServerStatus(-2,"control msg format invalid");
				continue;
			}

			if (!root.isObject() && !root.isArray())
			{
				PKLog.LogMessage(PK_LOGLEVEL_ERROR, "CollectTask recv an msg(%s), not an object or array!",strMessage.c_str());
				g_pServer->UpdateServerStatus(-2,"control msg format invalid");
				continue;
			}

		}
	}

	PKLog.LogMessage(PK_LOGLEVEL_NOTICE,"CollectTask 正常退出！");
	OnStop();

	m_bStopped = true;
	return PK_SUCCESS;	
}

int CCollectTask::OnStart()
{
	int nStatus = PK_SUCCESS;
	nStatus = m_redisSub.Initialize(PK_LOCALHOST_IP, PDB_LISTEN_PORT, PDB_ACCESS_PASSWORD);
	if(nStatus != PK_SUCCESS)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "Control Process Task, Initialize Sub failed, ret:%d", nStatus);
	}

	nStatus = m_redisRW.Initialize(PK_LOCALHOST_IP, PDB_LISTEN_PORT, PDB_ACCESS_PASSWORD);	
	if(nStatus != PK_SUCCESS)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "Control Process Task, Initialize RW failed, ret:%d", nStatus);
	}
	return nStatus;
}

// 本任务停止时
void CCollectTask::OnStop()
{
	m_redisSub.Finalize();
	m_redisRW.Finalize();
}

int CCollectTask::Start()
{
	return activate(THR_NEW_LWP | THR_JOINABLE |THR_INHERIT_SCHED);
}

void CCollectTask::Stop()
{
	m_bStop = true;
	int nWaitResult = wait();

	ACE_Time_Value tvSleep;
	tvSleep.msec(100);
	while(!m_bStopped)
		ACE_OS::sleep(tvSleep);
}


