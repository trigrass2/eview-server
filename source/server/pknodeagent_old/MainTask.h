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

#ifndef _CTRL_PROCESS_TASK_H_
#define _CTRL_PROCESS_TASK_H_

#include <ace/Task.h>
#include "tinyxml/tinyxml.h"
#include "json/json.h"
#include "redisproxy/redisproxy.h"

#include <map>
using namespace  std;

class CTaskGroup;
class CDevice;
class CMainTask : public ACE_Task<ACE_MT_SYNCH>
{
	friend class ACE_Singleton<CMainTask, ACE_Thread_Mutex>;
public:
	CMainTask();
	virtual ~CMainTask();

	int Start();
	void Stop();
protected:
	bool			m_bStop;
	bool			m_bStopped;
	CRedisProxy		m_redisRW;
protected:
	virtual int svc();

	// �̳߳�ʼ��
	int OnStart();

	// �߳���ֹͣ
	void OnStop();

	// �������ļ����غ����¼������е�����theTaskGroupList
	int LoadConfig();

	int InitRedis();
	int Signal2ToggleProcessShowHide(int nProcNo);
};

#define MAIN_TASK ACE_Singleton<CMainTask, ACE_Thread_Mutex>::instance()

#endif  // _CTRL_PROCESS_TASK_H_
