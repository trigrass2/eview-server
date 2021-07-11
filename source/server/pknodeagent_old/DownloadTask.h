/**************************************************************
 *  Filename:    CtrlProcessTask.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 控制命令处理类.
 *
 *  @author:     lijingjing
 *  @version     05/28/2008  lijingjing  Initial Version
 *  @version     01/28/2012  wanyingjie  Initial Version
**************************************************************/

#ifndef _DOWNLOAD_ROUTE_TASK_H_
#define _DOWNLOAD_ROUTE_TASK_H_

#include <ace/Task.h>
#include <string>
#include <map>
#include "redisproxy/redisproxy.h"
#include "json/json.h"

using namespace  std;

class CDownloadTask : public ACE_Task<ACE_MT_SYNCH>
{
	friend class ACE_Singleton<CDownloadTask, ACE_Thread_Mutex>;
public:
	CDownloadTask();
	virtual ~CDownloadTask();

	int Start();
	void Stop();

protected:
	bool			m_bStop;
	bool			m_bStopped;

protected:
	virtual int svc();

	// 线程初始化
	int OnStart();
	// 线程中停止
	void OnStop();
};

#define DOWNLOAD_TASK ACE_Singleton<CDownloadTask, ACE_Thread_Mutex>::instance()

#endif  // _DOWNLOAD_ROUTE_TASK_H_
