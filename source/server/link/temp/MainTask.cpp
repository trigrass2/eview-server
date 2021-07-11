/**************************************************************
 *  Filename:    ExecutionTak.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 执行预案的线程，接收的消息包括触发源触发，动作执行完毕等.
 *
 *  @author:     shijunpu
 *  @version     05/22/2008  shijunpu   Initial Version
 *  @version     08/09/2010  chenzhiquan  输出事件中细化动作执行结果为执行“成功”or“失败”
 *  @version     08/10/2010  chenzhiquan  修改事件输出：预案整体成功、失败
 *  @version	 07/12/2013	 zhangqiang 
**************************************************************/
#include "MainTask.h"
#include "ConfigLoader.h"
//#include "ServerActionTask.h"
#include "ServerActionTaskManager.h"
#include "common/gettimeofday.h"
#include "link/CrsApiServerDefines.h"
#include "json/json.h"
//#include "pkdbapi.h"
#include "sqlapi/SQLAPI.h"
//#include "TriggerTaskManager.h"

#define MAX_TRIGGER_NUMBER					3000
#define THREAD_COUNT_IN_EXECUTION_TASK		1	 

#define INIT_RESPONSE_VALUE_MAIN	0x00000000
#define INIT_RESPONSE_VALUE_BACK	0x40000000
#define MAX_RESPONSE_VALUE_MAIN		0x3FFFFFFF
#define MAX_RESPONSE_VALUE_BACK		0x7FFFFFFF
#define MAX_QUEUE_SIZE				100000
#define MAX_DELQUEUE_SIZE			10000

using namespace std;


#ifndef __sun
using std::map;
using std::string;
#endif

long ActionResultCallBack(int nLinkEventId, int nActionId,  char *szTriggerTime, long lResult, char *szResulltDesc)
{
	return 0;
}
// changed by ycd 手动触发(事件ID，事件名，优先级，事件参数)
long OnEvent(int nEventId, char *szEventName, int nPriority, char *szEventParam)
{
	//根据事件ID获取动作ID
	//SAConnection * m_EventActionDB = new SAConnection();
	SAConnection * m_EventActionDB = new SAConnection();
	try
	{
		char *szSql = new char();
		sprintf(szSql,"select id,actiontype_id,name,enable,event_id,param1,param2,param3,param4 from t_link_action where event_id=%d",nEventId);
		TAG_LINK_ACTION stru_Link_Action;
		
		
		ExecMysqlCmd(m_EventActionDB,szSql);
		delete szSql;
	}
	catch(SAException& e)
	{
		m_EventActionDB->Disconnect();
		delete m_EventActionDB;
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("Fail to read DB %d, ErrMsg:%s"), e.ErrPos() , e.ErrText());
		return 0;
	}
	map<int,TAG_LINK_ACTION *>::iterator itID2Event = cmainTask.m_mapId2Event.begin();
	ACE_DLL m_HDll;
	string strRecvResult;
	for(;itID2Event!=cmainTask.m_mapId2Event.end();itID2Event++)
	{
		switch (itID2Event->second->nActionTypeId)
		{
		case 0://写一个tag点
			m_HDll.open("writeATag.dll");
			cmainTask.m_pfn_initAction = (PFN_InitAction)m_HDll.symbol(ACE_TEXT(PFNNAME_INITACTION));
			cmainTask.m_pfn_exitAction = (PFN_ExitAction)m_HDll.symbol(ACE_TEXT(PFNNAME_EXITACTION));
			cmainTask.m_pfn_execAction = (PFN_ExecAction)m_HDll.symbol(ACE_TEXT(PFNNAME_EXECACTION));
			break;
		case 1://写多个tag点
			m_HDll.open("writeMultiTag.dll");
			cmainTask.m_pfn_initAction = (PFN_InitAction)m_HDll.symbol(ACE_TEXT(PFNNAME_INITACTION));
			cmainTask.m_pfn_exitAction = (PFN_ExitAction)m_HDll.symbol(ACE_TEXT(PFNNAME_EXITACTION));
			cmainTask.m_pfn_execAction = (PFN_ExecAction)m_HDll.symbol(ACE_TEXT(PFNNAME_EXECACTION));
			break;
		case 2://弹窗
			m_HDll.open("openPage.dll");
			cmainTask.m_pfn_initAction = (PFN_InitAction)m_HDll.symbol(ACE_TEXT(PFNNAME_INITACTION));
			cmainTask.m_pfn_exitAction = (PFN_ExitAction)m_HDll.symbol(ACE_TEXT(PFNNAME_EXITACTION));
			cmainTask.m_pfn_execAction = (PFN_ExecAction)m_HDll.symbol(ACE_TEXT(PFNNAME_EXECACTION));
			break;
		default:
			break;
		}
		//char *szEventActionId, char *szActionParam, char *szActionParam2, char *szActionParam3, char *szActionParam4, string &strResult
		char *szActionId= new char();
		sprintf(szActionId,"%d",itID2Event->second->nActionId);
		cmainTask.m_pfn_execAction(szActionId,(char*)(itID2Event->second->strParam1).c_str(),
			(char*)(itID2Event->second->strParam2).c_str(),(char*)(itID2Event->second->strParam3).c_str(),
			(char*)(itID2Event->second->strParam4).c_str(),strRecvResult);
		delete szActionId;
	}
}

CMainTask::CMainTask()
{
	ACE_Select_Reactor *pSelectReactor = new ACE_Select_Reactor();
	m_pReactor = new ACE_Reactor(pSelectReactor, true);
	this->reactor(m_pReactor);
}

CMainTask::~CMainTask()
{
	if(m_pReactor)	
	{
		delete m_pReactor;
		m_pReactor = NULL;
	}
}


/**
 *  Start the Task and threads in the task are activated.
 *
 *  @return 
 *	- ==0 success
 *	- !=0 failure
 *
 *  @version     05/23/2008  shijunpu  Initial Version.
 */
long CMainTask::StartTask()
{
	long lRet = (long)activate(THR_NEW_LWP | THR_JOINABLE |THR_INHERIT_SCHED, THREAD_COUNT_IN_EXECUTION_TASK);
	if(lRet != ICV_SUCCESS)
	{
		lRet = EC_ICV_CRS_FAILTOSTARTTASK;
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("CExecutionTask: StartTask() Failed, RetCode: %d"), lRet);
	}

	lRet = m_redisRW.Initialize(PK_LOCALHOST_IP, PDB_LISTEN_PORT, PDB_ACCESS_PASSWORD);
	if(lRet != ICV_SUCCESS)
	{
		lRet = EC_ICV_CRS_FAILTOSTARTTASK;
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("CExecutionTask: StartTask() redis initialize Failed, RetCode: %d"), lRet);
	}

	lRet = m_redisPublish.Initialize(PK_LOCALHOST_IP, PDB_LISTEN_PORT, PDB_ACCESS_PASSWORD);
	if(lRet != ICV_SUCCESS)
	{
		lRet = EC_ICV_CRS_FAILTOSTARTTASK;
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("CExecutionTask: StartTask() redis initialize Failed, RetCode: %d"), lRet);
	}
	return ICV_SUCCESS;	
}

/**
 *  stop the request handler task and threads in it.
 *
 *  @return 
 *	- ==0 success
 *	- !=0 failure
 *
 *  @version     05/23/2008  shijunpu  Initial Version.
 */
long CMainTask::StopTask()
{
	long nWkr = 0;
	long MsgType = ACE_Message_Block::MB_STOP;
	for (; nWkr < THREAD_COUNT_IN_EXECUTION_TASK; nWkr++)
	{
		CRS_InternalMsg msg;
		msg.lMsgType = 0;
		ACE_Message_Block *pMsg = new ACE_Message_Block(sizeof(CRS_InternalMsg));
		memcpy(pMsg->wr_ptr(), &msg, sizeof(CRS_InternalMsg));
		this->putq(pMsg);
	}
	m_redisRW.Finalize();
	m_redisPublish.Finalize();
	return ICV_SUCCESS;	
}

//edited by ycd
int CMainTask::LoadPfnAndInit()
{
	ACE_DLL m_EventDLL;
	m_mapID2ActionDllName.clear();
	m_mapID2EventTypeDllName.clear();
	//打开dll
	int nRet = m_EventDLL.open(ACE_DLL_PREFIX ACE_TEXT("evt.dll"));
	//获得事件dll
	m_pfnInitLinkEvent = (PFN_InitLinkEvent)m_EventDLL.symbol(ACE_TEXT(PFNNAME_INIT_LINKEVENT));
	m_pfnExitLinkEvent = (PFN_ExitLinkEvent)m_EventDLL.symbol(ACE_TEXT(PFNNAME_EXIT_LINKEVENT));
	//获得动作dll列表。

	if (!m_pfnInitLinkEvent)
	{
		m_pfnInitLinkEvent(OnEvent);
	}
	return 0;
}
/*从触发事件发过来的事件信息
 *  main procedure of the task.
 *	get a msgblk and 
 *  @return 
 *	- ==0 success
 *	- !=0 failure
 *
 *  @version     05/23/2008  shijunpu  Initial Version.

 //启动 EventTypeTaskManager ActionTypeTaskManagers
 */
int CMainTask::svc()
{
	// 根据主备机计算最大值
	long lIsBake = PKComm.GetLocalSequnce();
	
	PKLog.LogMessage(PK_LOGLEVEL_INFO, _("CExecutionTask::svc() begin"));
	//加载驱动函数指针并初始化
	/*
	1、动作指针
	2、事件指针
	*/
	LoadPfnAndInit();

	//
	//启动事件管理任务;
	//->StartTriggerTasks();
	//启动动作管理任务
	//ACTIONTASK_MANAGER->StartActionTasks();
// 	CRS_InternalMsg msg;
// 	int  nResValueLen = 0;
// 
	ACE_Time_Value tvLastPrintLog = ACE_OS::gettimeofday();
	ACE_Message_Block *pMsgBlk = NULL;
	bool bContinue = true;
	while(bContinue)
	{
		ACE_Time_Value tvTimes = ACE_OS::gettimeofday() + ACE_Time_Value(0,500*1000); // 500ms
		//ycd 获取？？
		int nRet=getq(pMsgBlk,&tvTimes);
		if(nRet < 0)
		{
			ACE_Time_Value tmSpan = tvTimes - tvLastPrintLog;
			long nTmSpan = tmSpan.sec();
			if(labs(tmSpan.sec()) > 60) // 每30ms
			{
				PKLog.LogMessage(PK_LOGLEVEL_INFO, "CExecutionTask::svc(), 定时检查,60s内未获取到任何一个消息 getq(pMsgBlk, &tvTimes) nRet=%d", nRet);
				tvLastPrintLog = tvTimes;
			}
			continue;
		}
// 
		ACE_Time_Value tmSpan = tvTimes - tvLastPrintLog;
		long nTmSpan = tmSpan.sec();
		if(labs(tmSpan.sec()) > 60) // 每30ms
		{
			PKLog.LogMessage(PK_LOGLEVEL_INFO, "CExecutionTask::svc(), 定时(60s)检查,成功获取到消息, getq(pMsgBlk, &tvTimes) success, nRet=%d", nRet);
			tvLastPrintLog = tvTimes;
		}

		//ACE_OS::memcpy(&msg, pMsgBlk->rd_ptr(), sizeof(CRS_InternalMsg));
		//pMsgBlk->rd_ptr(sizeof(CRS_InternalMsg));
		//pMsgBlk->release();
// 		//事件类型
		//switch (msg.lMsgType)
 		//{
// 			//触发源被触发
// 		case LINK_MESSAGETYPE_TRIGGER_TRIGGERED:	// 触发源被触发，From TriggerTask
// 			if (m_bActive)
// 			{
// 				// 取出触发源ID,获取动作ID
// 				//putq(pMsgBlk);
// // 				long lTriggerID = msg.lParam;
// // 				
//  				CRS_CURTIRGGER* pTrigger = reinterpret_cast<CRS_CURTIRGGER*>(msg.pParam);
//  				HandleTriggerMessage(pTrigger);
// 			}
// 			else
// 				PKLog.LogMessage(PK_LOGLEVEL_INFO, _("CExecutionTask: this machine is not active, not process trigger message."));//本机为非活动机，不处理预案的触发
// 			
// 			break;
// 
// 		case CRS_MESSAGETYPE_TIMER_CHECK:	// 定时检查动作是否超时, From TimerTask
// 			CheckCurItemTimeOut();
// 			break;
// 
// 		case CRS_MESSAGETYPE_ACTION_RESULT:		// 动作执行完毕,服务端动作和客户端动作，From ActionTask和ClientManageTask,失败的来自本线程
// 
// 			break;
// 		case CRS_MESSAGETYPE_THREAD_EXIT:
// 			PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "case CRS_MESSAGETYPE_THREAD_EXIT, Recv Exit Msg! exiting.....");
// 			bContinue = false;
// 			break;
// 			// 活动机的冗余通知，非活动才处理
// 		case LINK_MESSAGETYPE_RESPONSE_LATESTRSVALUE_RM: 
// 			/*
// 				CRSXmlElement* pRspRoot = reinterpret_cast<CRSXmlElement*>(msg.pParam);
// 				if (!m_bActive) // 非活动机处理
// 				{
// 					PKLog.LogMessage(PK_LOGLEVEL_INFO, _("CExecutionTask: receive redundancy message from active computer, start synchronous"));//收到来自活动机的冗余信息，开始同步
// 
// 					OnRMNotify(pRspRoot);
// 				}
// 
// 				SAFE_DELETE(pRspRoot);
//			}
// 			*/
 		//	break;
 		//}
 	}

	PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "ExecutionTask exited...");

	return ICV_SUCCESS;
}
//
// long CMainTask::HandleTriggerMessage(CRS_CURTIRGGER *pTrigger)
// {
// 	//1.将触发消息添加到触发源列表
//  	if(!pTrigger)
//  	{
//  		PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("The trigger doesn't include valid information."));//触发源未包含有效信息
//  	}
//  	long lRet = AddCurTrigger(pTrigger);//加入到当前触发源列表
//  	if(lRet != ICV_SUCCESS)
//  	{
//  		SAFE_DELETE(pTrigger);
//  		return -1;
//  	}
// 
// 	IncrementResponseStateValue();
//  	// 将当前触发的信息通知给所有的客户端
//  	//BroadCastAllClients(pTrigger);
//  	pTrigger->triggerInfo.uTriggerStatus = TriggerState_None;
//  	int nSucNum = 0;
//  	CRS_CURSCHEME_LIST::iterator iterScheme = pTrigger->curSchemeList.begin();
//  	for(; iterScheme != pTrigger->curSchemeList.end(); iterScheme ++)
//  	{
// 		
//   		lRet = StartTrigger(pTrigger);//并发执行;
//  		if(lRet == ICV_SUCCESS)
//  		{
//  			++ nSucNum;
//  			//RemoveCurScheme(ppCurScheme[i]);//移除当前预案
//  			//SAFE_DELETE(ppCurScheme[i]);
//  		}
//  	}
//  	if (nSucNum > 0)
//  	{
//  		CRSEventTrigger(pTrigger, "触发");
//  	}
// 	return ICV_SUCCESS;
// }
	

// 启动预案执行过程
// long CMainTask::StartTrigger(CRS_CURTIRGGER *pTrigger)
// {	
// 	return ExecuteCurTrigger(pTrigger);
// }

/**
*
*根据触发源列表信息将当前触发源信息补充完整
*到当前触发源列表中
*
*/
// long CMainTask::HandleAlarmTrigger(int ID,char *szActType)
// {
// 	PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::HandleAlarmTrigger() 收到一个报警动作[报警区或报警点ID:%d, 类型:%s]",ID,szActType);
// 	long nRet = 0;
// 
// 	// 对于没有配置确认联动和确认且恢复联动的报警点trigger，在收到确认或确认且恢复消息后，将所占有的资源释放
// 	UpdateCurTriggerList(ID,szActType);
// 
// 	// （暂定触发源id与报警点id相同，根据报警点id找到对应的触发源，构造当前触发源）
// 	CRS_CURTIRGGER *pCurTrigger = NULL;
// 	nRet = CONFIG_LOADER->GetReturnTriggerByTriggerID(ID,szActType,pCurTrigger);
// 
// 	if (pCurTrigger == NULL)
// 	{
// 		PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::HandleAlarmTrigger()该报警动作[报警区或报警点ID:%d, 类型:%s]未配置联动触发源，因此不需要执行联动动作",ID, szActType);
// 		return ICV_SUCCESS;
// 	}
// 
// 	// 判断触发源是不是延时执行，立即执行的触发源
// 	if (pCurTrigger->ulDuration == 0)
// 	{
// 		PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::HandleAlarmTrigger(). 报警动作[报警区或报警点ID:%d, 类型:%s], 对应联动是立即执行类型，已经开始执行:TriggerID:%d", ID, szActType, pCurTrigger->nTriggerID);
// 		ExecuteCurTrigger(pCurTrigger);	
// 		PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::HandleAlarmTrigger(). 报警动作[报警区或报警点ID:%d, 类型:%s], 对应的联动是立即执行类型，执行完毕:TriggerID:%d",ID, szActType, pCurTrigger->nTriggerID);
// 	}
// 	else  // 如果是，判断时间是不是到达执行时间
// 	{
// 		// 若到达，执行
// 		// 若没到，加入当前触发源列表
// 		PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::HandleAlarmTrigger() 报警动作[报警区或报警点ID:%d, 类型:%s], 对应的联动是延时执行类型，延时%d，已经加入待执行触发源列表，TriggerID：%d，",ID, szActType, pCurTrigger->ulDuration,pCurTrigger->nTriggerID);
// 		pCurTrigger->nExecuteStatus = TRIGGER_EXECUTE_STATUS_WAITEXECUTE;
// 		AddCurTrigger(pCurTrigger);
// 	}
// 	
// 	return nRet;
// }

// 处理当前触发源
//注释 by ycd
// long CMainTask::ExecuteCurTrigger(CRS_CURTIRGGER *pTrigger)
// {
// 	long lRet = ICV_SUCCESS;
// 	ACE_Message_Block* pMsgBlk = NULL;
// 	if(pTrigger == NULL)
// 	{
// 		PKLog.LogMessage(PK_LOGLEVEL_WARN, _("CExecutionTask: pTrigger is invalid. "));
// 		return EC_ICV_COMM_INVALIDPARA;
// 	}
// 
// 	// 遍历执行当前触发源下的每一个动作
// 	for (int i = 0;i < pTrigger->vecActionIDs.size();i++)
// 	{
// 		//待传送msg;
// 		CRS_InternalMsg msg;
// 		LINK_ACTION *pAction = NULL;
// 		LINK_ACTION *pFailAction = NULL;
// 		CONFIG_LOADER->getActionInfo(pTrigger->vecActionIDs[i],pAction,pFailAction);
// 	//	CRS_SCHEMEITEM_ACTION_INFO *pItemAction= new CRS_SCHEMEITEM_ACTION_INFO();
// 		CRS_CURTIRGGER *pCurTrigger = pTrigger;
// 		pCurTrigger->action = *pAction;
// 		pCurTrigger->failaction = *pFailAction;
// 	//	pItemAction->actionKey.nTriggerID=pCurTrigger->nTriggerID;
// 		msg.lMsgType=ACE_Message_Block::MB_EVENT;
// 		msg.nTriggerID = pCurTrigger->nTriggerID;
// 		bool IsExecuted = FALSE;
// 		if (!IsExecuted)
// 		{
// 			if (m_nCurPopPageExecutePriority >= 0 && m_nCurPopPageExecutePriority > pCurTrigger->triggerInfo.nPriority)   // 如果当期啊有触发源正在执行切优先级高于当前触发源，将当前触发源添加到当前触发源列表，等待执行
// 			{	
// 				PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::ExecuteCurTrigger()开始执行触发源,但是由于当前有优先级更高的触发源占用资源未释放 \
// 					，所以执行该触发源的第二个动作，TriggerID：%d,FailActionID：%d,ActionParam:%s",pCurTrigger->nTriggerID,pCurTrigger->failaction.lItemID,pCurTrigger->failaction.lItemID);
// 				// added by ycd  pCurTrigger包含动作ID的完整执行结构
// 				//传一个CRS_CURACTION结构体
// 				//failaction 和 pcurtrigger的区别 ;
// 				msg.pParam=(void*)(&pCurTrigger->failaction);
// 				ACE_Message_Block *pMsg = new ACE_Message_Block(sizeof(CRS_InternalMsg));
// 				memcpy(pMsg->wr_ptr(), &msg, sizeof(CRS_InternalMsg));
// 				putq(pMsg);
// 
// 				//ExecuteCurAction(pCurTrigger,pCurTrigger->failaction);
// 			}
// 			else
// 			{
// 				PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::ExecuteCurTrigger()开始执行触发源，TriggerID：%d,FailActionID：%d",pCurTrigger->nTriggerID,pCurTrigger->action.lItemID);
// 				msg.pParam=(void*)(&pCurTrigger->action);
// 				ACE_Message_Block *pMsg = new ACE_Message_Block(sizeof(CRS_InternalMsg));
// 				memcpy(pMsg->wr_ptr(), &msg, sizeof(CRS_InternalMsg));
// 				putq(pMsg);
// 			//	ExecuteCurAction(pCurTrigger,pCurTrigger->action);
// 			}
// 
// 			IsExecuted = TRUE;
// 		}
// 
// 		if (!IsExecuted)
// 		{
// 			if (m_nCurCamtoTVExecutePriority >= 0 && m_nCurCamtoTVExecutePriority > pCurTrigger->triggerInfo.nPriority)   // 如果当期啊有触发源正在执行切优先级高于当前触发源，将当前触发源添加到当前触发源列表，等待执行
// 			{		
// 				PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::ExecuteCurTrigger()开始执行触发源,但是由于当前有优先级更高的触发源占用资源未释放 \
// 					，所以执行该触发源的第二个动作，TriggerID：%d,FailActionID：%d",pCurTrigger->nTriggerID,pCurTrigger->failaction.lItemID);
// 				ExecuteCurAction(pCurTrigger,pCurTrigger->failaction);
// 			}
// 			else
// 			{
// 				PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::ExecuteCurTrigger()开始执行触发源，TriggerID：%d,FailActionID：%d",pCurTrigger->nTriggerID,pCurTrigger->action.lItemID);
// 				ExecuteCurAction(pCurTrigger,pCurTrigger->action);
// 			}
// 		}
// 
// 	}
// 
// 	pTrigger->nExecuteStatus = TRIGGER_EXECUTE_STATUS_EXECUTED;
// 	return ICV_SUCCESS;
// }

// 执行当前动作;
//注释 by ycd
// long CMainTask::ExecuteCurAction(CRS_CURTIRGGER *pCurTrigger,LINK_ACTION& curAction)
// {
// 
// 	PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::ExecuteCurAction()开始执行动作，TriggerID：%d,ActionID：%d",pCurTrigger->nTriggerID,curAction.lItemID);
// 	long lRet = ICV_SUCCESS;
// 	// 记录当前开始执行的时间;
// 	curAction.tvBegin = ACE_OS::gettimeofday(); 
// 	//ACE_Time_Value timeValue = ACE_OS::gettimeofday();
// 	curAction.lBeginTime = (unsigned int)curAction.tvBegin.sec();
// 
// 	IncrementResponseStateValue();
// 	BroadCastAllClients(&curAction);
// 
// 	long lFinishType = SchemeItemFailType_Success;
// 
// 	//如果在客户端运行
// 	if(curAction.uRunAtScada == 0 && curAction.lItemID != 0)
// 	{
// 		PublishAction2MenDB(&curAction,pCurTrigger->nTriggerID,pCurTrigger->triggerInfo.nPriority);
// 		return lRet;
// 	}
// 
// 	// 按说应该是先判断动作类型，不同的动作类型做不同的操作，但是现在时间不够了，先判断如果是写writetag的话，根据传过来的信息将其写到对应的tag点里面
// 	// 现在先只做了writetag和opencam两个联动动作，而opencam是在客户端执行的，只需将cam信息推送到指定的redis通道，在上面一个if判断过了，接下来判断writetag情况
// 	 if (curAction.uRunAtScada == 1)
// 	 {
// 		 WriteTag2MenDB(curAction,pCurTrigger->nTriggerID,pCurTrigger->triggerInfo.nPriority);
// 	 }
// 
// /*	lRet = ACTIONTASK_MANAGER->PutCurActionToActionTask(&curAction, lFinishType);
// 	if (lRet != ICV_SUCCESS)
// 	{
// 		//ProcessSelfActionResult(pCurScheme, lFinishType);
// 		PKLog.LogMessage(PK_LOGLEVEL_NOTICE,"CExecutionTask::ExecuteCurAction trigger:%s,action:%s execute failed!",
// 			pCurTrigger->triggerInfo.szTriggerName,curAction.szActionName);
// 	}
// 	*/
// 	return lRet;
// }

void CMainTask::SplitUINT64(ACE_UINT64 u64Src, int& nHH, int& nHL, int& nLH, int& nLL)
{
	ACE_UINT32 u32High = (ACE_UINT32)(u64Src >> 32);
	ACE_UINT64 u64Mid = u64Src << 32;
	ACE_UINT32 u32Low = (ACE_UINT32)(u64Mid >> 32);

	nHH = (int)(u32High >> 16);
	ACE_UINT32 u32Mid = u32High << 16;
	nHL = (int)(u32Mid >> 16);
	
	nLH = (int)(u32Low >> 16);
	u32Mid = u32Low << 16;
	nLL = (int)(u32Mid >> 16);
}

void CMainTask::MergeUINT64(int nHH, int nHL, int nLH, int nLL, ACE_UINT64& u64Dst)
{
	ACE_UINT32 u32High = nHH;
	u32High = u32High << 16;
	u32High = u32High | ((ACE_UINT32)nHL);

	ACE_UINT32 u32Low = nLH;
	u32Low = u32Low << 16;
	u32Low = u32Low | ((ACE_UINT32)nLL);

	u64Dst = u32High;
	u64Dst = u64Dst << 32;
	u64Dst = u64Dst | u32Low;
}
////注释 ycd
// long CMainTask::AddCurTrigger(CRS_CURTIRGGER *pTrigger)
// {
// 	CRS_CURTIRGGER *Trigger = pTrigger;
// 	// 如果当前触发源过多,则删除前面的触发源
// 	if(m_curTriggerList.size() >= MAX_TRIGGER_NUMBER)//3000指当前触发源最大个数
// 	{
// // 		CRS_CURTRIGGER_LIST::iterator iter = m_curTriggerList.begin();
// // 		CRS_CURTIRGGER pDelTrigger = *iter;
// // 		CRSEventTrigger(&pDelTrigger, "删除失败");
// // 		//向所有客户端和冗余发送消息，该触发源被删除
// // 		pDelTrigger->triggerInfo.uTriggerStatus = TriggerState_Deleted;
// // 		IncrementResponseStateValue();
// // 		BroadCastAllClients(&pDelTrigger);
// // 		RemoveCurTrigger(&pDelTrigger);
// 	}
// 
// 	bool bExsit = false;
// 	CRS_CURTRIGGER_LIST::iterator iter(m_curTriggerList.begin());
// 	for (;iter != m_curTriggerList.end();++ iter)
// 	{
// 		CRS_CURTIRGGER *trigger = *iter;
// 		if (trigger->nTriggerID == Trigger->nTriggerID)
// 		{
// 			bExsit = true;
// 			continue;
// 		}
// 	}
// 
// 	if (!bExsit)
// 	{
// 		Trigger->tvBegin = ACE_OS::gettimeofday();
// 		m_curTriggerList.push_back(Trigger);
// 	}
// 	
// 	return ICV_SUCCESS;
// }

// 将当前正在处理的触发源的相关信息通知给所有的客户端
//注释 ycd
// long CMainTask::BroadCastAllClients(LINK_ACTION *pAction)
// {
// 	char szMessage[PK_TAGDATA_MAXLEN] = {0};
// 	sprintf(szMessage,"{\"aname\":\"%s\",\"aid\":\"%d\",\"userlist\":\"%s\"}",pAction->szActionName,pAction->lItemID,pAction->szUserlist);
// 	Publish2MenDB(szMessage);
// 	return ICV_SUCCESS;
// }
// //注释 ycd
// long CMainTask::FindCurTrigger(const CRS_ACTION_KEY *pActionID, CRS_CURTIRGGER*& pTrigger)
// {
// 	pTrigger = NULL;
// 	CRS_CURTRIGGER_LIST::iterator iterTrigger(m_curTriggerList.begin());
// 	for(; iterTrigger != m_curTriggerList.end(); iterTrigger ++)
// 	{
// 		CRS_CURTIRGGER *pTempTrigger = *iterTrigger;
// 		if( pTempTrigger->nTriggerID == pActionID->nTriggerID)
// 		{
// 			pTrigger = pTempTrigger;
// 			return ICV_SUCCESS;
// 		}
// 	}
// 	return EC_ICV_CRS_SCHEME_NOTEXISTS;
// }

// 在当前触发源链表中删除本触发源执行信息
//注释 ycd
// long CMainTask::RemoveCurTrigger(CRS_CURTIRGGER *pTrigger)
// {
// //	const char* pszSchName = pScheme->szSchemePathName;
// 	CRS_CURTIRGGER *tempTrigger = pTrigger;
// 	try
// 	{
// 		m_curTriggerList.remove(tempTrigger);
// 		PKLog.LogMessage(PK_LOGLEVEL_INFO, _("CExecutionTask: trigger %s (ID = %d) completed, deleted from the current trigger list."), 
// 			pTrigger->triggerInfo.szTriggerName, pTrigger->triggerInfo.nTriggerID);
// 		
// 		SAFE_DELETE(pTrigger);
// 	}
// 	catch(...)
// 	{
// 		//PKLog.LogMessage(PK_LOGLEVEL_ERROR, "CExecutionTask: Fail to Remove Scheme(Name: %s) form CurSchemeList! Error Type: %s", 
// 		//	pszSchName, e.what());
// 		SAFE_DELETE(pTrigger);
// 		return EC_ICV_CRS_FAILTOREMOVESCHEME;
// 	}
// 
// 	return ICV_SUCCESS;
// }

//注释 ycd
// void CMainTask::SafelyClearCurTriggerList()
// {
// 	CRS_CURTRIGGER_LIST::iterator iter(m_curTriggerList.begin());
// 	for (; iter != m_curTriggerList.end(); ++ iter)
// 	{
// 		CRS_CURTIRGGER *pCurTrigger = *iter;
// 		SAFE_DELETE(pCurTrigger)
// 	}
// 
// 	m_curTriggerList.clear();
// }

// 检查当前动作是否超时
// 如果超时为负数，表示一直等待，这里不用处理
// 如果超时为0，取缺省值3秒
// 如果超时了，根据是否跳过进行处理
//注释 by  ycd
// void CMainTask::CheckCurItemTimeOut()
// {
// 	ACE_Message_Block* pMsgBlk = NULL;
// 	ACE_Time_Value tvNow = ACE_OS::gettimeofday();;
// 
// 	CRS_CURTRIGGER_LIST::iterator iterTrigger(m_curTriggerList.begin());
// 	for(; iterTrigger != m_curTriggerList.end(); iterTrigger ++)
// 	{
// 		CRS_CURTIRGGER* pCurTrigger = *iterTrigger;
// 
// 		unsigned long uDuration = (tvNow - pCurTrigger->tvBegin).msec();
// 		if (uDuration >= pCurTrigger->ulDuration && pCurTrigger->nExecuteStatus == TRIGGER_EXECUTE_STATUS_WAITEXECUTE)
// 		{
// 			ExecuteCurTrigger(pCurTrigger);
// 		}
// 	}
// }

//changed by ycd 动作执行的返回
// long CMainTask::ActionResultCallBack(int nLinkEventId, int nActionId,  char *szTriggerTime, long lResult, char *szResulltDesc)
// {
// 	CRS_ACTION_KEY pActionKeyCopy ;
// 
// 	//更新参数的值
// 	// 	if(actionKey.szReturnValue != NULL && strlen(actionKey.szReturnValue) > 0)
// 	// 		CONFIG_LOADER->UpdateVarialbeValue(actionKey.szReturnValue);
// 	pActionKeyCopy.nTriggerID=nLinkEventId;
// 	pActionKeyCopy.lTriggerTime=atoi(szTriggerTime);
// 	pActionKeyCopy.nSchemeItemID=nActionId;
// 	memcpy(pActionKeyCopy.szDesc,szResulltDesc,strlen(szResulltDesc));
// 	CRS_InternalMsg msg;
// 	msg.lMsgType = CRS_MESSAGETYPE_ACTION_RESULT;
// 	msg.pParam = &pActionKeyCopy;
// 	msg.lParam = lResult;
// 	ACE_Message_Block *pMsg = new ACE_Message_Block(sizeof(CRS_InternalMsg));
// 	memcpy(pMsg->wr_ptr(), &msg, sizeof(CRS_InternalMsg));
// 	//发送消息
// 	long lRet = MAIN_TASK->putq(pMsg);
// 	return lRet;
// }
//注释 ycd
// void CMainTask::CRSEventTrigger(const CRS_CURTIRGGER* pTrigger, const char* szMsg)
// {
// 	//如果描述为空，先记为'i_c_v'，到事件记录时再改为空，此策略主要针对sscanf函数解析空时就结束
// 	char szTempTriggerDesc[CV_CRS_DESC_MAXLEN] = {0};
// 	if(pTrigger->triggerInfo.szTriggerDesc != NULL && strlen(pTrigger->triggerInfo.szTriggerDesc) == 0)
// 		strcpy(szTempTriggerDesc, REPLACESTR);
// 	else if(pTrigger->triggerInfo.szTriggerDesc != NULL)
// 		strcpy(szTempTriggerDesc, pTrigger->triggerInfo.szTriggerDesc);
// 	//_("触发源ID:%d; 触发时间:%d; 名称:%s; 描述:%s; 分类:%s; 触发源类型:%d; 是否手动:%d; 状态:%s; 条件:%s")
// 	CRSEvent(pTrigger->triggerInfo.szOperator, EA_SUBTYPE_CRS_TRIGGER, "", "triggerID:%d; triggertime:%d; triggername:%s; triggerdesc:%s; triggergroup:%s; triggerype:%d; ismanual:%d; triggerstatus:%s; triggerconfig:%s",  
// 		pTrigger->triggerInfo.nTriggerID, pTrigger->triggerInfo.lTriggerTime, pTrigger->triggerInfo.szTriggerName, 
// 		szTempTriggerDesc, pTrigger->triggerInfo.szTriggerPath, pTrigger->triggerInfo.uTriggerType,
// 		pTrigger->triggerInfo.uExecuteType, szMsg, pTrigger->triggerInfo.szTriggerCfg);
// }
//注释 ycd
// void CMainTask::CRSEventAction(const LINK_ACTION* curAction, const char* szMsg)
// {
// 	//如果描述为空，先记为'i_c_v'，到事件记录时再改为空，此策略主要针对sscanf函数解析空时就结束
// 	char szTempActionDesc[CV_CRS_ITEMDESC_MAXLEN] = { 0 };
// 	if(curAction->szActionName != NULL && strlen(curAction->szActionDesc) == 0)
// 		strcpy(szTempActionDesc, REPLACESTR);
// 	else if(curAction->szActionDesc != NULL)
// 		strcpy(szTempActionDesc, curAction->szActionDesc);
// 
// 	//_("触发源ID:%d; 触发时间:%d; 动作ID:%d; 动作名称:%s; 动作描述:%s; 执行时间:%d; 预案分类:%s; 预案ID:%d; 预案名称:%s; 预案描述:%s; 预案优先级:%d; 预案权限:%s; 执行类型:%d; 状态:%s; 动作配置:%s"), 
// 	CRSEvent(curAction->szConfirmUserName, EA_SUBTYPE_CRS_EXECUTE, "", 
// 		"triggerID:%d; triggertime:%d; actionID:%d; actionname:%s; actiondesc:%s; exectime:%d; exectype:%d; status:%s; actionconfig:%s", 
// 		curAction->nTriggerID, curAction->lTriggerTime, curAction->lItemID,
// 		curAction->szActionName, szTempActionDesc, curAction->lBeginTime,
// 		curAction->uExecuteType, szMsg, curAction->szResponseVal);
// }

void CMainTask::Publish2MenDB(char *szVal)
{
	string strChannel = CHANNELNAME_LINK_BROADCAST;
	string strVal = szVal;
	m_redisPublish.PublishMsg(strChannel,strVal);
}

// 将在客户端执行的动作推送到web服务
//注释 ycd
// int CMainTask::PublishAction2MenDB(LINK_ACTION *curAction,int nTriggerID,int nPriority)
// {
// 
// 	Json::Value jsonAction;
// 	string strjsonAction ;
// 	int nRet;
// 
// 	char szTmLink[PK_TAGDATA_MAXLEN] = {'\0'};
// 	PKTimeHelper::GetCurTimeString(szTmLink,sizeof(szTmLink));
// 
// 	PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::PublishAction2MenDB() 执行动作，ActionID：%d,ActionType：%s",curAction->lItemID,curAction->szActionType);
// 	if (PKStringHelper::StriCmp(curAction->szActionType,"poppage") == 0)   // 如果当前动作类型是弹出画面
// 	{	
// 		m_nCurPopPageExecutePriority = nPriority;
// 		m_nCurPopPageExecuteTriggerID = nTriggerID;
// 
// 		jsonAction["action"] = curAction->szActionType;
// 		jsonAction["pagename"] = curAction->szPage;
// 		jsonAction["paramtype"] = curAction->szActionType1;
// 		jsonAction["paramvalue"] = curAction->szParam;
// 		strjsonAction = jsonAction.toStyledString();
// 		jsonAction["logtype"] = "link";
// 		jsonAction["actionid"] = curAction->lItemID;
// 		jsonAction["triggerid"] = nTriggerID;
// 		jsonAction["linktime"] = szTmLink;
// 		strjsonAction = jsonAction.toStyledString();
// 		nRet = m_redisPublish.PublishMsg(CHANNELNAME_LINK_ACTION2CLIENT,strjsonAction);
// 		PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::PublishAction2MenDB()发送一条动作执行信息到 action-client，内容：%s",strjsonAction.c_str());
// 		m_redisPublish.PublishMsg(CHANNELNAME_LOG,strjsonAction);
// 		//PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::PublishAction2MenDB()发送一条动作执行信息到 log-channel，内容：%s",strjsonAction.c_str());
// 	}else if (PKStringHelper::StriCmp(curAction->szActionType,"popmessage") == 0)   // 如果当前动作是弹出消息框
// 	{
// 		jsonAction["action"] = curAction->szActionType;
// 		jsonAction["message"] = curAction->szParam;
// 		strjsonAction = jsonAction.toStyledString();
// 		jsonAction["logtype"] = "link";
// 		jsonAction["actionid"] = curAction->lItemID;
// 		jsonAction["triggerid"] = nTriggerID;
// 		jsonAction["linktime"] = szTmLink;
// 		strjsonAction = jsonAction.toStyledString();
// 		nRet = m_redisPublish.PublishMsg(CHANNELNAME_LINK_ACTION2CLIENT,strjsonAction);
// 		PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::PublishAction2MenDB()发送一条动作执行信息到 action-client，内容：%s",strjsonAction.c_str());
// 		m_redisPublish.PublishMsg(CHANNELNAME_LOG,strjsonAction);
// 		//PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::PublishAction2MenDB()发送一条动作执行信息到 log-channel，内容：%s",strjsonAction.c_str());
// 	}else if (PKStringHelper::StriCmp(curAction->szActionType,"noaction") == 0)
// 	{
// 		PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::PublishAction2MenDB()动作类型是不做动作，已经成功返回 actionID:%d,actionType:%s",curAction->lItemID,curAction->szActionType);
// 		return ICV_SUCCESS;
// 	}else
// 	{
// 		return -1;
// 	}
// 
// 	if (nRet != 0)
// 	{
// 		PKLog.LogMessage(PK_LOGLEVEL_ERROR,"Publish to client failed");
// 	}
// 	return ICV_SUCCESS;
// }
// 写tag点
/*
	//注释 ycd
	以后参考
*/
// int CMainTask::WriteTag2MenDB(LINK_ACTION &curAction,int nTriggerID,int nPriority)
// {
// 	Json::Value jsonAction;
// 	string strAction;
// 
// 	char szTmLink[PK_TAGDATA_MAXLEN] = {'\0'};
// 	PKTimeHelper::GetCurTimeString(szTmLink,sizeof(szTmLink));
// 
// 	PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::WriteTag2MenDB() 执行动作，ActionID：%d,ActionType：%s",curAction.lItemID,curAction.szActionType);
// 
// 	if (PKStringHelper::StriCmp(curAction.szActionType,"writetag") == 0)
// 	{
// 		jsonAction.clear();
// 		jsonAction["action"] = curAction.szActionType;
// 		jsonAction["n"] = curAction.szParam;
// 		jsonAction["v"] = curAction.szParam1;
// 		strAction = jsonAction.toStyledString();
// 		// 将点写到redis
// 		m_redisPublish.set(curAction.szParam,strAction.c_str());
// 		PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::WriteTag2MenDB()推送一条消息到 control，内容：%s",strAction.c_str());
// 		// 发送日志
// 		jsonAction["logtype"] = "link";
// 		jsonAction["actionid"] = curAction.lItemID;
// 		jsonAction["triggerid"] = nTriggerID;
// 		jsonAction["linktime"] = szTmLink;
// 		strAction = jsonAction.toStyledString();
// 		m_redisPublish.PublishMsg(CHANNELNAME_LOG,strAction);
// 		//PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::WriteTag2MenDB()推送一条日志信息到log-channel，内容：%s",strAction.c_str());
// 	}else if (PKStringHelper::StriCmp(curAction.szActionType,"camtotv") == 0)
// 	{
// 		jsonAction.clear();
// 		string strTagName;
// 		string strTagValue;
// 		vector<string> vecCams;
// 		int nUsableTagID = atoi(curAction.szParam);
// 		string strCams = curAction.szParam1;
// 		vecCams = PKStringHelper::StriSplit(strCams,",");
// 		
// 		CRSUsableTagsList::iterator iter = CONFIG_LOADER->m_lstUsableTagList.begin();
// 		for (;iter != CONFIG_LOADER->m_lstUsableTagList.end();iter++)     //根据可用的tag点组ID寻早可用的tag点信息，里面有tag点名
// 		{
// 			USABLETAG *usableTag = &*iter;
// 
// 			if (usableTag->nID == nUsableTagID)     // 找到了可用的TAG点组
// 			{
// 				PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::WriteTag2MenDB()开始执行可用tag点组:%d",nUsableTagID);
// 				for (int i = 0;i < vecCams.size();i++)     // 遍历action的每一个值
// 				{
// 					
// 					int nLastUsed = -1;   // 如果是第一次使用，之前没有tag点被使用的话，不被赋值，使用默认的，则第一次用-1+1=0个，刚好
// 					for (int j = 0;j < usableTag->vecUsenableTagCurInfos.size();j ++)
// 					{
// 						if (usableTag->vecUsenableTagCurInfos[j].bLastUsed == 1)
// 						{
// 							nLastUsed = j;
// 							usableTag->vecUsenableTagCurInfos[j].bLastUsed = 0;
// 						}
// 					}
// 
// 					int nUsableTagCount = usableTag->vecUsenableTagCurInfos.size();
// 					for (int k = 0; k < nUsableTagCount; k ++)
// 					{
// 						if (nLastUsed+1 == nUsableTagCount)
// 						{
// 							nLastUsed = -1;
// 						}
// 
// 						if (nPriority >= usableTag->vecUsenableTagCurInfos[nLastUsed+1].nCurPriority)
// 						{
// 							PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::WriteTag2MenDB()开始执行可用tag点组:%d",nUsableTagID);
// 							// 设置当前使用的tag点信息
// 							usableTag->vecUsenableTagCurInfos[nLastUsed+1].nCurPriority = nPriority;
// 							usableTag->vecUsenableTagCurInfos[nLastUsed+1].nLastAcionID = nTriggerID;
// 							usableTag->vecUsenableTagCurInfos[nLastUsed+1].bLastUsed = 1;
// 
// 							strTagName = usableTag->vecUsenableTagCurInfos[nLastUsed+1].szTagName;
// 							strTagValue = vecCams[i];
// 							// 构建信息
// 							string strCamName = "cam";
// 							strCamName.append(vecCams[i]);
// 							char szUsableTagCName[128];
// 							char szTagCName[128];
// 							string strUsableTagAdd = CONFIG_LOADER->GetCameraID(strTagName,szUsableTagCName);
// 							string strTagAdd = CONFIG_LOADER->GetCameraID(strCamName,szTagCName);
// 							Json::Value jsonAction;
// 							jsonAction["name"] = usableTag->vecUsenableTagCurInfos[nLastUsed+1].szTagName;
// 							jsonAction["value"] = strTagAdd;
// 							strAction = jsonAction.toStyledString();
// 
// 							// 将点写到redis
// 							m_redisPublish.PublishMsg("control",strAction);
// 							PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::WriteTag2MenDB()推送一条切换摄像头消息到 control，内容：%s",strAction.c_str());
// 
// 							// 发送日志
// 							jsonAction["logtype"] = "link";
// 							jsonAction["action"] = curAction.szActionType;
// 							jsonAction["actionid"] = curAction.lItemID;
// 							jsonAction["triggerid"] = nTriggerID;
// 							jsonAction["usablecname"] = szUsableTagCName;
// 							jsonAction["tagcname"] = szTagCName;
// 							jsonAction["linktime"] = szTmLink;
// 							strAction = jsonAction.toStyledString();
// 							m_redisPublish.PublishMsg(CHANNELNAME_LOG,strAction);
// 							//PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::WriteTag2MenDB()推送一条日志信息到log-channel，内容：%s",strAction.c_str());
// 							break;
// 						}
// 						break;
// 					}
// 				}
// 			}
// 		}
// 	}
// 	return ICV_SUCCESS;
// }

int CMainTask::OnStart()
{
	m_pReactor->cancel_timer((ACE_Event_Handler *)this);
	ACE_Time_Value	tvCheckConn;		// 扫描周期，单位ms
	tvCheckConn.msec(10 * 1000); 
	ACE_Time_Value tvStart = ACE_Time_Value::zero; 
	m_pReactor->schedule_timer((ACE_Event_Handler *)this, NULL, ACE_Time_Value::zero, tvCheckConn);
	return 0;
}

int CMainTask::OnStop()
{
	m_pReactor->close();
	return 0;
}

// 定时轮询的周期到了.json格式的日志信息，包括：level,time两个字段
int CMainTask::handle_timeout(const ACE_Time_Value &current_time, const void *act)
{
	// 每5秒读取一次是否打印日志的标记
	int nRet = PK_SUCCESS;
	time_t tmNow;
	time(&tmNow);
	CheckEventTimeout();

	return PK_SUCCESS;
}


//select id,actiontype_id,name,enable,event_id,param1,param2,param3,param4 from t_link_action where event_id
long CMainTask::LoadConfigFromDB()
{
	SACommand cmd;
	TAG_LINK_ACTION stru_Link_Action;
	cmd.setCommandText(szSql);
	if(m_EventActionDB->isConnected())
		m_EventActionDB->Disconnect();
	m_EventActionDB->Connect("192.168.10.2:13306@eview","root","r123456",SA_MySQL_Client);
	if(m_EventActionDB->isConnected())
	{
		cmd.setConnection(m_EventActionDB);
		cmd.Execute();
	}
	while (cmd.FetchNext())
	{
		if (cmd.FieldCount()<9)
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("execute sql command error should have 9 rows ,but return %d rows "), cmd.FieldCount());
			return -1;
		}
		memset(&stru_Link_Action, 0x00, sizeof(TAG_LINK_ACTION));
		//获取动作id；
		stru_Link_Action.nActionId = cmd.Field(1).asInterval();
		//获取事件名
		stru_Link_Action.strName = cmd.Field(3).asString();
		//获取动作类型
		stru_Link_Action.nActionTypeId = cmd.Field(2).asInterval();
		//获取使能
		stru_Link_Action.nActionEnable = cmd.Field(4).asInterval();
		//获取动作ID
		stru_Link_Action.nEventId = cmd.Field(5).asInterval();
		//获取变量1-4
		stru_Link_Action.strParam1 = cmd.Field(6).asString();
		stru_Link_Action.strParam2 = cmd.Field(7).asString();
		stru_Link_Action.strParam3 = cmd.Field(8).asString();
		stru_Link_Action.strParam4 = cmd.Field(9).asString();

		cmainTask.m_mapId2Event.insert(map<int,TAG_LINK_ACTION*>::value_type(stru_Link_Action.nActionId,&stru_Link_Action));
	}
	return 0;
}
