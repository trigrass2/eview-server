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

#ifndef _CTRL_PROC_TASK_H_
#define _CTRL_PROC_TASK_H_

#include <ace/Task.h>
#include <string>
#include <map>
#include "pkmemdbapi/pkmemdbapi.h"
#include "json/json.h"
#include "ProcessMngr.h"

// 命令值的解释。如果是所有启动、所有进程停止、所有进程重启，那么tagname传入：pm:all
#define PMCONTROL_PROCESS_CMDNO_START			"1"
#define PMCONTROL_PROCESS_CMDNO_STOP			"0"
#define PMCONTROL_PROCESS_CMDNO_RESTART			"2"
#define PMCONTROL_PROCESS_CMD_START				"start"
#define PMCONTROL_PROCESS_CMD_STOP				"stop"
#define PMCONTROL_PROCESS_CMD_RESTART			"restart"
#define PMCONTROL_PROCESS_CMD_PRINTLIC			"printlicense"	// 主线程main.cpp发送给MainTask打印请求
#define PMCONTROL_PROCESS_CMD_PRINTPROC			"printproc"		// 主线程main.cpp发送给MainTask打印请求
#define PMCONTROL_PROCESS_CMD_RESRESH			"refresh"		// 主线程main.cpp发送给MainTask通知刷新请求。未实现
#define PMCONTROL_PROCESS_CMD_SHOWHIDE			"toggleshow"	// 未实现

using namespace  std;

class CMainTask : public ACE_Task<ACE_MT_SYNCH>
{
	friend class ACE_Singleton<CMainTask, ACE_Thread_Mutex>;
public:
	CMainTask();
	virtual ~CMainTask();

	int Start();
	void Stop();
	int Signal2StopAllProcess(bool bRestartProcesses); // 发出停止所有进程的通知
	int Signal2NotifyAllProcessRefresh();	// 发出通知刷新通知
	int Signal2PrintAllProcessState();
	int Signal2PrintLicenseState();
	int Signal2ToggleProcessShowHide(int nProcNo);

public:
#ifndef DSIABLE_PM_REMOTE_API
	CMemDBSubscriberSync	m_memDBSubSync; // 订阅
	CMemDBClientAsync		m_memDBClientAsync; // 读写
	CPKProcessMgr			m_cvProcMgr;
#endif
protected:
	bool			m_bStop;
	bool			m_bStopped;

protected:
	virtual int svc();

	// 线程初始化
	int OnStart();
	// 线程中停止
	void OnStop();

	int OnRemoteCmd(string &strCtrlCmd, Json::Value &jsonRoot); // 接收远程命令的处理
	int InitSysTags();
	int SignalCmd2MainTask(char *szCmdName, char *szProcessNameOrNo);
	int InitAndClearPMTags();
};

#define MAIN_TASK ACE_Singleton<CMainTask, ACE_Thread_Mutex>::instance()

#endif  // _CTRL_PROC_TASK_H_
