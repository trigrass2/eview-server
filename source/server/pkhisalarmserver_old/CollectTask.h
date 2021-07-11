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

#ifndef _COLLECT_TASK_H_
#define _COLLECT_TASK_H_

#include <ace/Task.h>
#include <string>
#include <map>
#include "redisproxy/redisproxy.h"
#include "json/json.h"

using namespace  std;

class CCollectTask : public ACE_Task<ACE_MT_SYNCH>
{
	friend class ACE_Singleton<CCollectTask, ACE_Thread_Mutex>;
public:
	CCollectTask();
	virtual ~CCollectTask();

	int Start();
	void Stop();

protected:
	bool			m_bStop;
	bool			m_bStopped;

	CRedisProxy		m_redisSub; // ����
	CRedisProxy		m_redisRW; // ����
protected:
	virtual int svc();

	// �̳߳�ʼ��
	int OnStart();
	// �߳���ֹͣ
	void OnStop();
};

#define COLLECT_TASK ACE_Singleton<CCollectTask, ACE_Thread_Mutex>::instance()

#endif  // _COLLECT_TASK_H_
