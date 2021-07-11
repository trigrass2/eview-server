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

#ifndef _CTRL_PROC_TASK_H_
#define _CTRL_PROC_TASK_H_

#include <ace/Task.h>
#include <string>
#include <map>
#include "pkmemdbapi/pkmemdbapi.h"
#include "json/json.h"

// ����ֵ�Ľ��͡�������������������н���ֹͣ�����н�����������ôtagname���룺pm:all
#define PMCONTROL_PROCESS_CMDNO_START			"1"
#define PMCONTROL_PROCESS_CMDNO_STOP			"0"
#define PMCONTROL_PROCESS_CMDNO_RESTART			"2"
#define PMCONTROL_PROCESS_CMD_START				"start"
#define PMCONTROL_PROCESS_CMD_STOP				"stop"
#define PMCONTROL_PROCESS_CMD_RESTART			"restart"
#define PMCONTROL_PROCESS_CMD_PRINTLIC			"printlicense"	// ���߳�main.cpp���͸�MainTask��ӡ����
#define PMCONTROL_PROCESS_CMD_PRINTPROC			"printproc"		// ���߳�main.cpp���͸�MainTask��ӡ����
#define PMCONTROL_PROCESS_CMD_RESRESH			"refresh"		// ���߳�main.cpp���͸�MainTask֪ͨˢ������δʵ��
#define PMCONTROL_PROCESS_CMD_SHOWHIDE			"toggleshow"	// δʵ��

typedef int(*PFN_OnControl)(char *szJsonControlInfo); // ���յ���������Ĵ���

using namespace  std;

class CMemDBTask : public ACE_Task<ACE_MT_SYNCH>
{
	friend class ACE_Singleton<CMemDBTask, ACE_Thread_Mutex>;
public:
	CMemDBTask();
	virtual ~CMemDBTask();

public:
	//CMemDBSubscriberSync	m_memDBSubSync; // ����, �����ղ�����Ϣ������
	CMemDBSubscriberAsync	m_memDBSubAsync; // ����

	CMemDBClientAsync		m_memDBClientAsync; // ��д

protected:
	bool			m_bStop;
	bool			m_bStopped;

public:
	int				StartTask();
	void			StopTask();

protected:
	virtual int svc();

	// �̳߳�ʼ��
	int		OnStart();
	// �߳���ֹͣ
	void	OnStop();
	int		DelProcessStatusTags();
};

#define MEMDB_TASK ACE_Singleton<CMemDBTask, ACE_Thread_Mutex>::instance()

#endif  // _CTRL_PROC_TASK_H_
