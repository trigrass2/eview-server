/**************************************************************
 *  Filename:    CtrlProcessTask.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: �����������.
 *
 *  @author:    shijunpu
 *  @version    05/28/2008  ȷ�Ϻ�ɾ������Ҳ���ڿ����������ִ�С�����Ҳ�Ƕ������ӣ�����ת�������ݴ��������
**************************************************************/

#include "common/OS.h"
#include <ace/Default_Constants.h>
#include "ace/Thread_Mutex.h"
#include "ace/Guard_T.h"

#include "math.h"

#include "SplitConfigTask.h"
#include "common/pkcomm.h"
#include "common/pkGlobalHelper.h"
#include "redisproxy/redisproxy.h"
#include "json/json.h"
#include "common/RMAPI.h"
#include "common/pkdefine.h"
#include "pklog/pklog.h"
#include "pkConfigChangeServer.h"

extern CConfigChangeServer* g_pServer;

#define	DEFAULT_SLEEP_TIME		100	

/**
 *  ���캯��.
 *
 *  @param  -[in,out]  CDriver*  pDriver: [comment]
 *
 *  @version     05/30/2008  lijingjing  Initial Version.
 */
CSplitConfigTask::CSplitConfigTask()
{
	m_bStop = false;
	m_bStopped = false;
}

/**
 *  ��������.
 *
 *
 *  @version     05/30/2008  lijingjing  Initial Version.
 */
CSplitConfigTask::~CSplitConfigTask()
{
	
}

/**
 *  �麯�����̳���ACE_Task.
 *
 *
 *  @version     05/30/2008  lijingjing  Initial Version.
 */
int CSplitConfigTask::svc()
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
		string strControlsChannel = CHANNELNAME_CONTROL_LOCAL;
		nRet = m_redisSub.SubChannel(strControlsChannel);
		// ����������״̬
		if(nRet != 0)
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "Control process task,Subscribe channle:%s failed,ret:%d", strControlsChannel.c_str(), nRet);
			g_pServer->UpdateServerStatus(-2,"subscribe control channel failed");
		}

		while(!m_bStop)
		{
			string strChannel = "";
			string strMessage;
			if(m_redisSub.RecvChannelMsg(strChannel, strMessage,100) != 0) // δ�յ���Ϣ
			{
				ACE_OS::sleep(tvSleep);
				continue;
			}

			PKLog.LogMessage(PK_LOGLEVEL_INFO, "SplitConfigTask recv an controls msg!");
			Json::Reader reader;
			Json::Value root;
			if (!reader.parse(strMessage, root, false))
			{
				PKLog.LogMessage(PK_LOGLEVEL_ERROR, "SplitConfigTask recv an msg(%s), invalid!",strMessage.c_str());
				g_pServer->UpdateServerStatus(-2,"control msg format invalid");
				continue;
			}

			if (!root.isObject() && !root.isArray())
			{
				PKLog.LogMessage(PK_LOGLEVEL_ERROR, "SplitConfigTask recv an msg(%s), not an object or array!",strMessage.c_str());
				g_pServer->UpdateServerStatus(-2,"control msg format invalid");
				continue;
			}

		}
	}

	PKLog.LogMessage(PK_LOGLEVEL_NOTICE,"SplitConfigTask �����˳���");
	OnStop();

	m_bStopped = true;
	return PK_SUCCESS;	
}

int CSplitConfigTask::OnStart()
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

// ������ֹͣʱ
void CSplitConfigTask::OnStop()
{
	m_redisSub.Finalize();
	m_redisRW.Finalize();
}

int CSplitConfigTask::Start()
{
	return activate(THR_NEW_LWP | THR_JOINABLE |THR_INHERIT_SCHED);
}

void CSplitConfigTask::Stop()
{
	m_bStop = true;
	int nWaitResult = wait();

	ACE_Time_Value tvSleep;
	tvSleep.msec(100);
	while(!m_bStopped)
		ACE_OS::sleep(tvSleep);
}

