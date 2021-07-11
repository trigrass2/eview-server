/**************************************************************
 *  Filename:    ExecutionTak.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: ִ��Ԥ�����̣߳����յ���Ϣ��������Դ����������ִ����ϵ�.
 *
 *  @author:     shijunpu
 *  @version     05/22/2008  shijunpu   Initial Version
 *  @version     08/09/2010  chenzhiquan  ����¼���ϸ��Ԥ����ִ�н��Ϊִ�С��ɹ���or��ʧ�ܡ�
 *  @version     08/10/2010  chenzhiquan  �޸��¼������Ԥ������ɹ���ʧ��
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
	map<int ,string> m_mapID2ActionDllName ;		//dll���͵����Ƶ�map
	map<int ,string> m_mapID2EventTypeDllName;		//dll�¼����͵����Ƶ�map
	map<int,TAG_LINK_ACTION *>	m_mapId2Event;		// �����ݿ��ȡ���¼�Id���¼����ݵ�map

	map<int,LINK_EVENT *>	m_vecEventExecuting;	// ��ǰ����ִ�е��¼���map��keyΪ��ʱ���ɵ�triggerId��һ���¼���������ִ�ж��

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

	//��¼����Դ�¼� ע�� ycd
//	void CRSEventTrigger(const CRS_CURTIRGGER* pCurTrigger, const char* szMsg);
	//��¼�����¼�   ע�� ycd
//	void CRSEventAction(const CRS_CURACTION *curAction, const char* szMsg);
	// ��һ��64λ�޷���������Ϊ4������
	void SplitUINT64(ACE_UINT64 u64Src, int& nHH, int& nHL, int& nLH, int& nLL);

	// ���ĸ������ϲ�Ϊһ��64λ�޷�������
	void MergeUINT64(int nHH, int nHL, int nLH, int nLL, ACE_UINT64& u64Dst);
	// ���ҵ��Ĵ���Դ���뵽��ǰִ�д���Դ�б� ע�� ycd
//	long AddCurTrigger(CRS_CURTIRGGER *pTrigger);
	//actionִ�ж�������
	
};
typedef ACE_Singleton<CMainTask, ACE_Thread_Mutex> CExecutionTaskSingleton;
#define MAIN_TASK CExecutionTaskSingleton::instance()

#endif // _EXECUTION_TASK_EVENT_REQEST_HANDLER_

