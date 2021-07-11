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

typedef int(*PFN_OnControl)(char *szJsonControlInfo); // 接收到控制命令的处理

using namespace  std;

class CMemDBTask : public ACE_Task<ACE_MT_SYNCH>
{
	friend class ACE_Singleton<CMemDBTask, ACE_Thread_Mutex>;
public:
	CMemDBTask();
	virtual ~CMemDBTask();

public:
	//CMemDBSubscriberSync	m_memDBSubSync; // 订阅, 总是收不到消息！！！
	CMemDBSubscriberAsync	m_memDBSubAsync; // 订阅

	CMemDBClientAsync		m_memDBClientAsync; // 读写

protected:
	bool			m_bStop;
	bool			m_bStopped;

public:
	int				StartTask();
	void			StopTask();

protected:
	virtual int svc();

	// 线程初始化
	int		OnStart();
	// 线程中停止
	void	OnStop();
	int		DelProcessStatusTags();
};

#define MEMDB_TASK ACE_Singleton<CMemDBTask, ACE_Thread_Mutex>::instance()

#endif  // _CTRL_PROC_TASK_H_
