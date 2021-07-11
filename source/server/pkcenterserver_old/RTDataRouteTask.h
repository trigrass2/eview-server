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

#ifndef _RTDATAROUTE_TASK_H_
#define _RTDATAROUTE_TASK_H_

#include <ace/Task.h>
#include <string>
#include <map>
#include "redisproxy/redisproxy.h"
#include "json/json.h"

using namespace  std;

class CRTDataRouteTask : public ACE_Task<ACE_MT_SYNCH>
{
	friend class ACE_Singleton<CRTDataRouteTask, ACE_Thread_Mutex>;
public:
	CRTDataRouteTask();
	virtual ~CRTDataRouteTask();

	int Start();
	void Stop();

protected:
	bool			m_bStop;
	bool			m_bStopped;

	CRedisProxy		m_redisSub; // ����
	CRedisProxy		m_redisRW; // ����
	CRedisProxy		m_redisPublish; // ����
	unsigned int	m_nPackNumRecvRecent;
	unsigned int	m_nPackNumRecvFromStart;
	ACE_Time_Value  m_tvRecentRecvPack;		// �ϴδ�ӡ
	ACE_Time_Value  m_tvFirstRecvPack;		// ��һ�ν��յ����ݰ�
	unsigned int	m_nTimeoutPackNumFromStart;
	unsigned int	m_nTimeoutPackNumRecent;
protected:
	virtual int svc();

	// �̳߳�ʼ��
	int OnStart();
	// �߳���ֹͣ
	void OnStop();
	int RouteOrSetTagsData(string &strCtrlCmd,Json::Value &root);
};

#define RTDATAROUTE_TASK ACE_Singleton<CRTDataRouteTask, ACE_Thread_Mutex>::instance()

#endif  // _RTDATAROUTE_TASK_H_
