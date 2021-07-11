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

#include "pkcomm/pkcomm.h"
#include <map>
#include <vector>
#include <string>
#include "pkdbapi/pkdbapi.h" 
#pragma comment(lib, "pkdbapi.lib")

using namespace std;


//�����ͻ���HMICRSINITʹ��
#define		PFNNAME_INITEVENT				"InitEvent"	//��ʼ������ģ��ĵ�������
#define		PFNNAME_EXITEVENT				"ExitEvent"	//�˳�����ģ��ĵ�������;
typedef int(*PFN_LinkEventCallback)(char* szEventId, char *szEventName, char *szPriority, char*szProduceTime, char *szEventParam);
typedef int(*PFN_InitEvent)(PFN_LinkEventCallback); //  ��ʼ��Action�ĺ���
typedef int(*PFN_ExitEvent)(); // �˳�ʱ���õĺ���

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

protected:
  ACE_Reactor*	m_pReactor;
  virtual int		handle_output(ACE_HANDLE fd = ACE_INVALID_HANDLE);
  int				OnStart();
  int				OnStop();
  virtual int		svc();
  virtual int handle_timeout(const ACE_Time_Value &current_time, const void *act);	// collect data

protected:
  vector<LINK_EVENT_DLL *> m_vecEventDll;		// �¼����Ƶ��¼���map	
  long LoadEventDll();
  long LoadConfigFromDB();
  int LoadPyFile();
  int LoadDBFile();

private:
  CPKDbApi m_dbAPI;
  string strConnStr;
  string strUserName;
  string strPassword;
  string strDbType;
  string strTimeout;
  string strCodingSet;

  string m_strPyLocation;

};
typedef ACE_Singleton<CMainTask, ACE_Thread_Mutex> CExecutionTaskSingleton;
#define MAIN_TASK CExecutionTaskSingleton::instance()

#endif // _EXECUTION_TASK_EVENT_REQEST_HANDLER_

