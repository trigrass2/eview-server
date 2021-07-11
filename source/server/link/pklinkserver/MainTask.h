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

#include "pkmemdbapi/pkmemdbapi.h"
#include "ActionTask.h"
#include "pkcomm/pkcomm.h"
#include <map>
#include <vector>
using namespace std;

typedef struct _LINK_ACTION_INFO
{
	string strActionId;
	string strActionName;
	int nActionTypeId;
	string strActionTypeName;
	int nEventId;
	int nExecuteOrder;
	string strSubsys;
	string strParam1;
	string strParam2;
	string strParam3;
	string strParam4;
	string strDesc;
	string strFailTip;
	string strActionTime;
	_LINK_ACTION_INFO()
	{
		strActionId = -1;
		nActionTypeId = -1;
		nEventId = -1;
		nExecuteOrder = -1;
	}
}LINK_ACTION_INFO;

typedef LINK_ACTION_INFO LINK_ACTIONING_INFO;

// char *szActionId, char *szTagNameList, char *szTagValue, char *szHoldSeconds, char *szActionParam4, char *szPriority, char* szJsonEventParams, char *szResult, int nResultBufLen
typedef struct _EVENT_ACTION_INFO
{
	int nEventId;
	vector<LINK_ACTION_INFO *> vecAction;
	
	_EVENT_ACTION_INFO()
	{
		nEventId = -1;
		vecAction.clear();
	}
	~_EVENT_ACTION_INFO()
	{
		for(vector<LINK_ACTION_INFO *>::iterator itAction = vecAction.begin(); itAction != vecAction.end(); itAction ++)
			delete *itAction;
		vecAction.clear();
	}
}EVENT_ACTION_INFO;

//�����ͻ���HMICRSINITʹ��
#define		PFNNAME_INITEVENT				"InitEvent"	//��ʼ������ģ��ĵ�������
#define		PFNNAME_EXITEVENT				"ExitEvent"	//�˳�����ģ��ĵ�������;
typedef int (*PFN_LinkEventCallback)(char* szEventId, char *szEventName, char *szPriority, char*szProduceTime,char *szEventParam);
typedef int  (*PFN_InitEvent)(PFN_LinkEventCallback); //  ��ʼ��Action�ĺ���
typedef int (*PFN_ExitEvent)(); // �˳�ʱ���õĺ���


typedef struct _LINK_EVENT_DLL
{
	string strEventName;
	string strEventDllName;
	ACE_DLL dllEvent;
	PFN_InitEvent pfnInit;
	PFN_ExitEvent pfnExit;
	_LINK_EVENT_DLL()
	{
		pfnInit = NULL;
		pfnExit = NULL;
	}
}LINK_EVENT_DLL;

class CMainTask : public ACE_Task<ACE_MT_SYNCH>  
{
	friend class ACE_Singleton<CMainTask, ACE_Thread_Mutex>;

public:
	CMainTask();
	virtual ~CMainTask();

public:
	bool			m_bStop;
	long			StartTask();
	long			StopTask();
	int				PostActionResult(char *szActionId, int nResultCode, char *szResultDesc);
	
protected:
	ACE_Reactor*	m_pReactor;
	virtual int		handle_output (ACE_HANDLE fd = ACE_INVALID_HANDLE);
	int				OnStart();
	int				OnStop();
	virtual int		svc();
	virtual int handle_timeout(const ACE_Time_Value &current_time, const void *act);	// collect data

protected:
	CMemDBClientAsync		m_redisRW;
	int InitRedis();
	int SendActionInfoToEventServer(LINK_ACTIONING_INFO *);
	int MSG2String(LINK_ACTIONING_INFO* linkActionInfo,string &str2EventLog);
protected:
	vector<LINK_EVENT_DLL *> m_vecEventDll;		// �¼����Ƶ��¼���map	
	map<string,  CActionTask *>	m_mapActionType2Task;		// �������Ƶ�����Task��map
	map<string, EVENT_ACTION_INFO *> m_mapId2Event;			// �¼�Id���¼���map

	map<string, LINK_ACTIONING_INFO*>	m_mapId2Actioning;		// ����ID������ִ�еĶ���
	long LoadConfigFromDB();
	long LoadEventAndActionDll();

protected:
	int CheckEventTimeout();
};
typedef ACE_Singleton<CMainTask, ACE_Thread_Mutex> CExecutionTaskSingleton;
#define MAIN_TASK CExecutionTaskSingleton::instance()

#endif // _EXECUTION_TASK_EVENT_REQEST_HANDLER_

