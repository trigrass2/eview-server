/**************************************************************
 *  Filename:    ServiceMainTask.h
 *  Copyright:   Shanghai Peakinfo Co., Ltd.
 *
 *  Description: 服务主进程头文件.
 *
 *  @author:     wanghongwei
 *  @version     09/@dd/2016  @Reviser  Initial Version
 **************************************************************/

#pragma once

#ifndef _SOCK_SERVER_TASK_
#define _SOCK_SERVER_TASK_

#include "PKSockCliSvr.h"
 
#include <ace/Singleton.h>
#include <ace/Null_Mutex.h>
#include <ace/Task.h>
#include <vector>
#include <map>
#include "pksock/pksockserver.h"

using namespace std;

class CSockServerTask : public ACE_Task<ACE_NULL_SYNCH>
{
friend class ACE_Singleton<CSockServerTask, ACE_Null_Mutex>;
public:
	CSockServerTask();
	virtual ~CSockServerTask();
	 
public:
	int				StartTask();
	int				StopTask();
	virtual int		svc();
	 
protected:
	int				ReadConfig();
	int				OnStart();
	int				OnStop();

	int				RecvFromCliAndResponse();
private:	
	bool			m_bStop; // 是否退出的标识
	HQUEUE			m_hRemoteQueue;

};
typedef ACE_Singleton<CSockServerTask, ACE_Null_Mutex> CSockServerTaskSingleton;
#define SOCKSERVER_TASK CSockServerTaskSingleton::instance()

#endif //_SOCK_SERVER_TASK_