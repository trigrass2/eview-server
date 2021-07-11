/**************************************************************
 *  Filename:    ExecutionTak.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 执行预案的线程，接收的消息包括触发源触发，动作执行完毕等.
 *
 *  @author:     shijunpu
 *  @version     05/22/2008  shijunpu   Initial Version
 *  @version     08/09/2010  chenzhiquan  输出事件中细化预案项执行结果为执行“成功”or“失败”
 *  @version     08/10/2010  chenzhiquan  修改事件输出：预案整体成功、失败
 *  @version	 07/12/2013	 zhangqiang 
**************************************************************/
 
#ifndef _EXECUTION_TASK_EVENT_REQEST_HANDLER_
#define _EXECUTION_TASK_EVENT_REQEST_HANDLER_

#include <ace/Task.h>
#include "ace/Reactor.h"
#include "ace/Select_Reactor.h"
#include "ace/Acceptor.h"
#include <ace/SOCK_Acceptor.h>
#include "LinkService.h"
#include "redisproxy/redisproxy.h"
#include <vector>
#include "link/linkdefine.h"
#include "link/TriggerIntrl.h"
class CMainTask : public ACE_Task<ACE_MT_SYNCH>  
{
friend class ACE_Singleton<CMainTask, ACE_Thread_Mutex>;
UNITTEST(CMainTask);
public:
	CMainTask();
	virtual ~CMainTask();
	
public:
	long			StartTask();
	long			StopTask();
	int				OnStart();
	int				OnStop();
	virtual int		svc();
	int LoadPfnAndInit();
	CRedisProxy m_redisRW; 
	CRedisProxy m_redisPublish;
	ACE_Reactor*	m_pReactor;
	map<int ,string> m_mapID2ActionDllName ;		//dll类型到名称的map
	map<int ,string> m_mapID2EventTypeDllName;		//dll事件类型到名称的map
	map<int,TAG_LINK_ACTION *>	m_mapId2Event;		// 从数据库读取的事件Id到事件内容的map

	map<int,LINK_EVENT *>	m_vecEventExecuting;	// 当前正在执行的事件的map。key为临时生成的triggerId。一个事件可以正在执行多次

	PFN_InitAction m_pfn_initAction;
	PFN_ExecAction m_pfn_execAction;
	PFN_ExitAction m_pfn_exitAction;

public:
	long LoadConfigFromDB();
public:
	int ExecuteNextAction(LINK_EVENT *pCurEvent);
	int CheckEventTimeout();
	virtual int handle_timeout(const ACE_Time_Value &current_time, const void *act);	// collect data
public:

	void Publish2MenDB(char *szVal);

	//记录触发源事件 注释 ycd
//	void CRSEventTrigger(const CRS_CURTIRGGER* pCurTrigger, const char* szMsg);
	//记录动作事件   注释 ycd
//	void CRSEventAction(const CRS_CURACTION *curAction, const char* szMsg);
	// 将一个64位无符号整数拆为4个整数
	void SplitUINT64(ACE_UINT64 u64Src, int& nHH, int& nHL, int& nLH, int& nLL);

	// 将四个整数合并为一个64位无符号整数
	void MergeUINT64(int nHH, int nHL, int nLH, int nLL, ACE_UINT64& u64Dst);
	// 将找到的触发源放入到当前执行触发源列表 注释 ycd
//	long AddCurTrigger(CRS_CURTIRGGER *pTrigger);
	//action执行动作返回
	
};
typedef ACE_Singleton<CMainTask, ACE_Thread_Mutex> CExecutionTaskSingleton;
#define MAIN_TASK CExecutionTaskSingleton::instance()

#endif // _EXECUTION_TASK_EVENT_REQEST_HANDLER_

