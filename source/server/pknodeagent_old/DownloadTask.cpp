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

#include "DownloadTask.h"
#include "common/pkcomm.h"
#include "common/pkGlobalHelper.h"
#include "json/json.h"
#include "common/RMAPI.h"
#include "common/pkdefine.h"
#include "pklog/pklog.h"
#include "pkNodeAgent.h"

extern CNodeAgent* g_pServer;

#define	DEFAULT_SLEEP_TIME		100	

/**
 *  ���캯��.
 *
 *  @param  -[in,out]  CDriver*  pDriver: [comment]
 *
 *  @version     05/30/2008  lijingjing  Initial Version.
 */
CDownloadTask::CDownloadTask()
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
CDownloadTask::~CDownloadTask()
{
	
}

/**
 *  �麯�����̳���ACE_Task.
 *
 *
 *  @version     05/30/2008  lijingjing  Initial Version.
 */
int CDownloadTask::svc()
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

	}

	PKLog.LogMessage(PK_LOGLEVEL_NOTICE,"DownloadTask �����˳���");
	OnStop();

	m_bStopped = true;
	return PK_SUCCESS;	
}

int CDownloadTask::OnStart()
{
	return 0;
}

// ������ֹͣʱ
void CDownloadTask::OnStop()
{
}

int CDownloadTask::Start()
{
	return activate(THR_NEW_LWP | THR_JOINABLE |THR_INHERIT_SCHED);
}

void CDownloadTask::Stop()
{
	m_bStop = true;
	int nWaitResult = wait();

	ACE_Time_Value tvSleep;
	tvSleep.msec(100);
	while(!m_bStopped)
		ACE_OS::sleep(tvSleep);
}
