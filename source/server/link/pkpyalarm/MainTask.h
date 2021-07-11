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

#include "pkcomm/pkcomm.h"
#include <map>
#include <vector>
#include <string>
#include "pkdbapi/pkdbapi.h" 
#pragma comment(lib, "pkdbapi.lib")

using namespace std;


//动作客户端HMICRSINIT使用
#define		PFNNAME_INITEVENT				"InitEvent"	//初始化动作模块的导出函数
#define		PFNNAME_EXITEVENT				"ExitEvent"	//退出动作模块的导出函数;
typedef int(*PFN_LinkEventCallback)(char* szEventId, char *szEventName, char *szPriority, char*szProduceTime, char *szEventParam);
typedef int(*PFN_InitEvent)(PFN_LinkEventCallback); //  初始化Action的函数
typedef int(*PFN_ExitEvent)(); // 退出时调用的函数

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
  vector<LINK_EVENT_DLL *> m_vecEventDll;		// 事件名称到事件的map	
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

