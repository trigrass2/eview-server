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

#ifndef _CTRL_ROUTE_TASK_H_
#define _CTRL_ROUTE_TASK_H_

#include <ace/Task.h>
#include <string>
#include <map>
#include "redisproxy/redisproxy.h"
#include "json/json.h"

using namespace  std;

class CCtrlRouteTask : public ACE_Task<ACE_MT_SYNCH>
{
	friend class ACE_Singleton<CCtrlRouteTask, ACE_Thread_Mutex>;
public:
	CCtrlRouteTask();
	virtual ~CCtrlRouteTask();

	int Start();
	void Stop();

protected:
	bool			m_bStop;
	bool			m_bStopped;

	CRedisProxy		m_redisSub; // 订阅
	CRedisProxy		m_redisRW; // 订阅

protected:
	virtual int svc();

	// 线程初始化
	int OnStart();
	// 线程中停止
	void OnStop();
	int RouteOrSetAlarm(string &strCtrlCmd, Json::Value &root);
};

#define CTRLROUTE_TASK ACE_Singleton<CCtrlRouteTask, ACE_Thread_Mutex>::instance()

#endif  // _CTRL_ROUTE_TASK_H_
