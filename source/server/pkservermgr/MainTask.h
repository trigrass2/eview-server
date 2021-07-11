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
#include "ProcessMngr.h"

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

using namespace  std;

class CMainTask : public ACE_Task<ACE_MT_SYNCH>
{
	friend class ACE_Singleton<CMainTask, ACE_Thread_Mutex>;
public:
	CMainTask();
	virtual ~CMainTask();

	int Start();
	void Stop();
	int Signal2StopAllProcess(bool bRestartProcesses); // ����ֹͣ���н��̵�֪ͨ
	int Signal2NotifyAllProcessRefresh();	// ����֪ͨˢ��֪ͨ
	int Signal2PrintAllProcessState();
	int Signal2PrintLicenseState();
	int Signal2ToggleProcessShowHide(int nProcNo);

public:
#ifndef DSIABLE_PM_REMOTE_API
	CMemDBSubscriberSync	m_memDBSubSync; // ����
	CMemDBClientAsync		m_memDBClientAsync; // ��д
	CPKProcessMgr			m_cvProcMgr;
#endif
protected:
	bool			m_bStop;
	bool			m_bStopped;

protected:
	virtual int svc();

	// �̳߳�ʼ��
	int OnStart();
	// �߳���ֹͣ
	void OnStop();

	int OnRemoteCmd(string &strCtrlCmd, Json::Value &jsonRoot); // ����Զ������Ĵ���
	int InitSysTags();
	int SignalCmd2MainTask(char *szCmdName, char *szProcessNameOrNo);
	int InitAndClearPMTags();
};

#define MAIN_TASK ACE_Singleton<CMainTask, ACE_Thread_Mutex>::instance()

#endif  // _CTRL_PROC_TASK_H_
