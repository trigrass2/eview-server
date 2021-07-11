/**************************************************************
 *  Filename:    CtrlProcessTask.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: �����������.
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
#include "common/pktagdef.h"

using namespace  std;

class CCtrlRouteTask : public ACE_Task<ACE_MT_SYNCH>
{
	friend class ACE_Singleton<CCtrlRouteTask, ACE_Thread_Mutex>;
public:
	CCtrlRouteTask();
	virtual ~CCtrlRouteTask();

	int Start();
	void Stop();

	map<string, CTag *>			m_mapName2Tag;

protected:
	bool			m_bStop;
	bool			m_bStopped;

	CRedisProxy		m_redisSub; // ����
	CRedisProxy		m_redisRW; // ����
	CRedisProxy		m_redisPublish; // ����
protected:
	virtual int svc();

	// �̳߳�ʼ��
	int OnStart();
	// �߳���ֹͣ
	void OnStop();
	// �������м̸��ڵ�, [{"name":tag1,"value":"100"},{"name":tag2,"value":"100"}]
	int RouteControl2Node(string &strCtrlCmd, Json::Value &root);
};

#define CTRLROUTE_TASK ACE_Singleton<CCtrlRouteTask, ACE_Thread_Mutex>::instance()

#endif  // _CTRL_ROUTE_TASK_H_
