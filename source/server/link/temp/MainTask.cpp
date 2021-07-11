/**************************************************************
 *  Filename:    ExecutionTak.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: ִ��Ԥ�����̣߳����յ���Ϣ��������Դ����������ִ����ϵ�.
 *
 *  @author:     shijunpu
 *  @version     05/22/2008  shijunpu   Initial Version
 *  @version     08/09/2010  chenzhiquan  ����¼���ϸ������ִ�н��Ϊִ�С��ɹ���or��ʧ�ܡ�
 *  @version     08/10/2010  chenzhiquan  �޸��¼������Ԥ������ɹ���ʧ��
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
// changed by ycd �ֶ�����(�¼�ID���¼��������ȼ����¼�����)
long OnEvent(int nEventId, char *szEventName, int nPriority, char *szEventParam)
{
	//�����¼�ID��ȡ����ID
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
		case 0://дһ��tag��
			m_HDll.open("writeATag.dll");
			cmainTask.m_pfn_initAction = (PFN_InitAction)m_HDll.symbol(ACE_TEXT(PFNNAME_INITACTION));
			cmainTask.m_pfn_exitAction = (PFN_ExitAction)m_HDll.symbol(ACE_TEXT(PFNNAME_EXITACTION));
			cmainTask.m_pfn_execAction = (PFN_ExecAction)m_HDll.symbol(ACE_TEXT(PFNNAME_EXECACTION));
			break;
		case 1://д���tag��
			m_HDll.open("writeMultiTag.dll");
			cmainTask.m_pfn_initAction = (PFN_InitAction)m_HDll.symbol(ACE_TEXT(PFNNAME_INITACTION));
			cmainTask.m_pfn_exitAction = (PFN_ExitAction)m_HDll.symbol(ACE_TEXT(PFNNAME_EXITACTION));
			cmainTask.m_pfn_execAction = (PFN_ExecAction)m_HDll.symbol(ACE_TEXT(PFNNAME_EXECACTION));
			break;
		case 2://����
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
	//��dll
	int nRet = m_EventDLL.open(ACE_DLL_PREFIX ACE_TEXT("evt.dll"));
	//����¼�dll
	m_pfnInitLinkEvent = (PFN_InitLinkEvent)m_EventDLL.symbol(ACE_TEXT(PFNNAME_INIT_LINKEVENT));
	m_pfnExitLinkEvent = (PFN_ExitLinkEvent)m_EventDLL.symbol(ACE_TEXT(PFNNAME_EXIT_LINKEVENT));
	//��ö���dll�б�

	if (!m_pfnInitLinkEvent)
	{
		m_pfnInitLinkEvent(OnEvent);
	}
	return 0;
}
/*�Ӵ����¼����������¼���Ϣ
 *  main procedure of the task.
 *	get a msgblk and 
 *  @return 
 *	- ==0 success
 *	- !=0 failure
 *
 *  @version     05/23/2008  shijunpu  Initial Version.

 //���� EventTypeTaskManager ActionTypeTaskManagers
 */
int CMainTask::svc()
{
	// �����������������ֵ
	long lIsBake = PKComm.GetLocalSequnce();
	
	PKLog.LogMessage(PK_LOGLEVEL_INFO, _("CExecutionTask::svc() begin"));
	//������������ָ�벢��ʼ��
	/*
	1������ָ��
	2���¼�ָ��
	*/
	LoadPfnAndInit();

	//
	//�����¼���������;
	//->StartTriggerTasks();
	//����������������
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
		//ycd ��ȡ����
		int nRet=getq(pMsgBlk,&tvTimes);
		if(nRet < 0)
		{
			ACE_Time_Value tmSpan = tvTimes - tvLastPrintLog;
			long nTmSpan = tmSpan.sec();
			if(labs(tmSpan.sec()) > 60) // ÿ30ms
			{
				PKLog.LogMessage(PK_LOGLEVEL_INFO, "CExecutionTask::svc(), ��ʱ���,60s��δ��ȡ���κ�һ����Ϣ getq(pMsgBlk, &tvTimes) nRet=%d", nRet);
				tvLastPrintLog = tvTimes;
			}
			continue;
		}
// 
		ACE_Time_Value tmSpan = tvTimes - tvLastPrintLog;
		long nTmSpan = tmSpan.sec();
		if(labs(tmSpan.sec()) > 60) // ÿ30ms
		{
			PKLog.LogMessage(PK_LOGLEVEL_INFO, "CExecutionTask::svc(), ��ʱ(60s)���,�ɹ���ȡ����Ϣ, getq(pMsgBlk, &tvTimes) success, nRet=%d", nRet);
			tvLastPrintLog = tvTimes;
		}

		//ACE_OS::memcpy(&msg, pMsgBlk->rd_ptr(), sizeof(CRS_InternalMsg));
		//pMsgBlk->rd_ptr(sizeof(CRS_InternalMsg));
		//pMsgBlk->release();
// 		//�¼�����
		//switch (msg.lMsgType)
 		//{
// 			//����Դ������
// 		case LINK_MESSAGETYPE_TRIGGER_TRIGGERED:	// ����Դ��������From TriggerTask
// 			if (m_bActive)
// 			{
// 				// ȡ������ԴID,��ȡ����ID
// 				//putq(pMsgBlk);
// // 				long lTriggerID = msg.lParam;
// // 				
//  				CRS_CURTIRGGER* pTrigger = reinterpret_cast<CRS_CURTIRGGER*>(msg.pParam);
//  				HandleTriggerMessage(pTrigger);
// 			}
// 			else
// 				PKLog.LogMessage(PK_LOGLEVEL_INFO, _("CExecutionTask: this machine is not active, not process trigger message."));//����Ϊ�ǻ����������Ԥ���Ĵ���
// 			
// 			break;
// 
// 		case CRS_MESSAGETYPE_TIMER_CHECK:	// ��ʱ��鶯���Ƿ�ʱ, From TimerTask
// 			CheckCurItemTimeOut();
// 			break;
// 
// 		case CRS_MESSAGETYPE_ACTION_RESULT:		// ����ִ�����,����˶����Ϳͻ��˶�����From ActionTask��ClientManageTask,ʧ�ܵ����Ա��߳�
// 
// 			break;
// 		case CRS_MESSAGETYPE_THREAD_EXIT:
// 			PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "case CRS_MESSAGETYPE_THREAD_EXIT, Recv Exit Msg! exiting.....");
// 			bContinue = false;
// 			break;
// 			// ���������֪ͨ���ǻ�Ŵ���
// 		case LINK_MESSAGETYPE_RESPONSE_LATESTRSVALUE_RM: 
// 			/*
// 				CRSXmlElement* pRspRoot = reinterpret_cast<CRSXmlElement*>(msg.pParam);
// 				if (!m_bActive) // �ǻ������
// 				{
// 					PKLog.LogMessage(PK_LOGLEVEL_INFO, _("CExecutionTask: receive redundancy message from active computer, start synchronous"));//�յ����Ի����������Ϣ����ʼͬ��
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
// 	//1.��������Ϣ��ӵ�����Դ�б�
//  	if(!pTrigger)
//  	{
//  		PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("The trigger doesn't include valid information."));//����Դδ������Ч��Ϣ
//  	}
//  	long lRet = AddCurTrigger(pTrigger);//���뵽��ǰ����Դ�б�
//  	if(lRet != ICV_SUCCESS)
//  	{
//  		SAFE_DELETE(pTrigger);
//  		return -1;
//  	}
// 
// 	IncrementResponseStateValue();
//  	// ����ǰ��������Ϣ֪ͨ�����еĿͻ���
//  	//BroadCastAllClients(pTrigger);
//  	pTrigger->triggerInfo.uTriggerStatus = TriggerState_None;
//  	int nSucNum = 0;
//  	CRS_CURSCHEME_LIST::iterator iterScheme = pTrigger->curSchemeList.begin();
//  	for(; iterScheme != pTrigger->curSchemeList.end(); iterScheme ++)
//  	{
// 		
//   		lRet = StartTrigger(pTrigger);//����ִ��;
//  		if(lRet == ICV_SUCCESS)
//  		{
//  			++ nSucNum;
//  			//RemoveCurScheme(ppCurScheme[i]);//�Ƴ���ǰԤ��
//  			//SAFE_DELETE(ppCurScheme[i]);
//  		}
//  	}
//  	if (nSucNum > 0)
//  	{
//  		CRSEventTrigger(pTrigger, "����");
//  	}
// 	return ICV_SUCCESS;
// }
	

// ����Ԥ��ִ�й���
// long CMainTask::StartTrigger(CRS_CURTIRGGER *pTrigger)
// {	
// 	return ExecuteCurTrigger(pTrigger);
// }

/**
*
*���ݴ���Դ�б���Ϣ����ǰ����Դ��Ϣ��������
*����ǰ����Դ�б���
*
*/
// long CMainTask::HandleAlarmTrigger(int ID,char *szActType)
// {
// 	PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::HandleAlarmTrigger() �յ�һ����������[�������򱨾���ID:%d, ����:%s]",ID,szActType);
// 	long nRet = 0;
// 
// 	// ����û������ȷ��������ȷ���һָ������ı�����trigger�����յ�ȷ�ϻ�ȷ���һָ���Ϣ�󣬽���ռ�е���Դ�ͷ�
// 	UpdateCurTriggerList(ID,szActType);
// 
// 	// ���ݶ�����Դid�뱨����id��ͬ�����ݱ�����id�ҵ���Ӧ�Ĵ���Դ�����쵱ǰ����Դ��
// 	CRS_CURTIRGGER *pCurTrigger = NULL;
// 	nRet = CONFIG_LOADER->GetReturnTriggerByTriggerID(ID,szActType,pCurTrigger);
// 
// 	if (pCurTrigger == NULL)
// 	{
// 		PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::HandleAlarmTrigger()�ñ�������[�������򱨾���ID:%d, ����:%s]δ������������Դ����˲���Ҫִ����������",ID, szActType);
// 		return ICV_SUCCESS;
// 	}
// 
// 	// �жϴ���Դ�ǲ�����ʱִ�У�����ִ�еĴ���Դ
// 	if (pCurTrigger->ulDuration == 0)
// 	{
// 		PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::HandleAlarmTrigger(). ��������[�������򱨾���ID:%d, ����:%s], ��Ӧ����������ִ�����ͣ��Ѿ���ʼִ��:TriggerID:%d", ID, szActType, pCurTrigger->nTriggerID);
// 		ExecuteCurTrigger(pCurTrigger);	
// 		PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::HandleAlarmTrigger(). ��������[�������򱨾���ID:%d, ����:%s], ��Ӧ������������ִ�����ͣ�ִ�����:TriggerID:%d",ID, szActType, pCurTrigger->nTriggerID);
// 	}
// 	else  // ����ǣ��ж�ʱ���ǲ��ǵ���ִ��ʱ��
// 	{
// 		// �����ִ��
// 		// ��û�������뵱ǰ����Դ�б�
// 		PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::HandleAlarmTrigger() ��������[�������򱨾���ID:%d, ����:%s], ��Ӧ����������ʱִ�����ͣ���ʱ%d���Ѿ������ִ�д���Դ�б�TriggerID��%d��",ID, szActType, pCurTrigger->ulDuration,pCurTrigger->nTriggerID);
// 		pCurTrigger->nExecuteStatus = TRIGGER_EXECUTE_STATUS_WAITEXECUTE;
// 		AddCurTrigger(pCurTrigger);
// 	}
// 	
// 	return nRet;
// }

// ����ǰ����Դ
//ע�� by ycd
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
// 	// ����ִ�е�ǰ����Դ�µ�ÿһ������
// 	for (int i = 0;i < pTrigger->vecActionIDs.size();i++)
// 	{
// 		//������msg;
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
// 			if (m_nCurPopPageExecutePriority >= 0 && m_nCurPopPageExecutePriority > pCurTrigger->triggerInfo.nPriority)   // ������ڰ��д���Դ����ִ�������ȼ����ڵ�ǰ����Դ������ǰ����Դ��ӵ���ǰ����Դ�б��ȴ�ִ��
// 			{	
// 				PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::ExecuteCurTrigger()��ʼִ�д���Դ,�������ڵ�ǰ�����ȼ����ߵĴ���Դռ����Դδ�ͷ� \
// 					������ִ�иô���Դ�ĵڶ���������TriggerID��%d,FailActionID��%d,ActionParam:%s",pCurTrigger->nTriggerID,pCurTrigger->failaction.lItemID,pCurTrigger->failaction.lItemID);
// 				// added by ycd  pCurTrigger��������ID������ִ�нṹ
// 				//��һ��CRS_CURACTION�ṹ��
// 				//failaction �� pcurtrigger������ ;
// 				msg.pParam=(void*)(&pCurTrigger->failaction);
// 				ACE_Message_Block *pMsg = new ACE_Message_Block(sizeof(CRS_InternalMsg));
// 				memcpy(pMsg->wr_ptr(), &msg, sizeof(CRS_InternalMsg));
// 				putq(pMsg);
// 
// 				//ExecuteCurAction(pCurTrigger,pCurTrigger->failaction);
// 			}
// 			else
// 			{
// 				PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::ExecuteCurTrigger()��ʼִ�д���Դ��TriggerID��%d,FailActionID��%d",pCurTrigger->nTriggerID,pCurTrigger->action.lItemID);
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
// 			if (m_nCurCamtoTVExecutePriority >= 0 && m_nCurCamtoTVExecutePriority > pCurTrigger->triggerInfo.nPriority)   // ������ڰ��д���Դ����ִ�������ȼ����ڵ�ǰ����Դ������ǰ����Դ��ӵ���ǰ����Դ�б��ȴ�ִ��
// 			{		
// 				PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::ExecuteCurTrigger()��ʼִ�д���Դ,�������ڵ�ǰ�����ȼ����ߵĴ���Դռ����Դδ�ͷ� \
// 					������ִ�иô���Դ�ĵڶ���������TriggerID��%d,FailActionID��%d",pCurTrigger->nTriggerID,pCurTrigger->failaction.lItemID);
// 				ExecuteCurAction(pCurTrigger,pCurTrigger->failaction);
// 			}
// 			else
// 			{
// 				PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::ExecuteCurTrigger()��ʼִ�д���Դ��TriggerID��%d,FailActionID��%d",pCurTrigger->nTriggerID,pCurTrigger->action.lItemID);
// 				ExecuteCurAction(pCurTrigger,pCurTrigger->action);
// 			}
// 		}
// 
// 	}
// 
// 	pTrigger->nExecuteStatus = TRIGGER_EXECUTE_STATUS_EXECUTED;
// 	return ICV_SUCCESS;
// }

// ִ�е�ǰ����;
//ע�� by ycd
// long CMainTask::ExecuteCurAction(CRS_CURTIRGGER *pCurTrigger,LINK_ACTION& curAction)
// {
// 
// 	PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::ExecuteCurAction()��ʼִ�ж�����TriggerID��%d,ActionID��%d",pCurTrigger->nTriggerID,curAction.lItemID);
// 	long lRet = ICV_SUCCESS;
// 	// ��¼��ǰ��ʼִ�е�ʱ��;
// 	curAction.tvBegin = ACE_OS::gettimeofday(); 
// 	//ACE_Time_Value timeValue = ACE_OS::gettimeofday();
// 	curAction.lBeginTime = (unsigned int)curAction.tvBegin.sec();
// 
// 	IncrementResponseStateValue();
// 	BroadCastAllClients(&curAction);
// 
// 	long lFinishType = SchemeItemFailType_Success;
// 
// 	//����ڿͻ�������
// 	if(curAction.uRunAtScada == 0 && curAction.lItemID != 0)
// 	{
// 		PublishAction2MenDB(&curAction,pCurTrigger->nTriggerID,pCurTrigger->triggerInfo.nPriority);
// 		return lRet;
// 	}
// 
// 	// ��˵Ӧ�������ж϶������ͣ���ͬ�Ķ�����������ͬ�Ĳ�������������ʱ�䲻���ˣ����ж������дwritetag�Ļ������ݴ���������Ϣ����д����Ӧ��tag������
// 	// ������ֻ����writetag��opencam����������������opencam���ڿͻ���ִ�еģ�ֻ�轫cam��Ϣ���͵�ָ����redisͨ����������һ��if�жϹ��ˣ��������ж�writetag���
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
////ע�� ycd
// long CMainTask::AddCurTrigger(CRS_CURTIRGGER *pTrigger)
// {
// 	CRS_CURTIRGGER *Trigger = pTrigger;
// 	// �����ǰ����Դ����,��ɾ��ǰ��Ĵ���Դ
// 	if(m_curTriggerList.size() >= MAX_TRIGGER_NUMBER)//3000ָ��ǰ����Դ������
// 	{
// // 		CRS_CURTRIGGER_LIST::iterator iter = m_curTriggerList.begin();
// // 		CRS_CURTIRGGER pDelTrigger = *iter;
// // 		CRSEventTrigger(&pDelTrigger, "ɾ��ʧ��");
// // 		//�����пͻ��˺����෢����Ϣ���ô���Դ��ɾ��
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

// ����ǰ���ڴ���Ĵ���Դ�������Ϣ֪ͨ�����еĿͻ���
//ע�� ycd
// long CMainTask::BroadCastAllClients(LINK_ACTION *pAction)
// {
// 	char szMessage[PK_TAGDATA_MAXLEN] = {0};
// 	sprintf(szMessage,"{\"aname\":\"%s\",\"aid\":\"%d\",\"userlist\":\"%s\"}",pAction->szActionName,pAction->lItemID,pAction->szUserlist);
// 	Publish2MenDB(szMessage);
// 	return ICV_SUCCESS;
// }
// //ע�� ycd
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

// �ڵ�ǰ����Դ������ɾ��������Դִ����Ϣ
//ע�� ycd
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

//ע�� ycd
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

// ��鵱ǰ�����Ƿ�ʱ
// �����ʱΪ��������ʾһֱ�ȴ������ﲻ�ô���
// �����ʱΪ0��ȡȱʡֵ3��
// �����ʱ�ˣ������Ƿ��������д���
//ע�� by  ycd
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

//changed by ycd ����ִ�еķ���
// long CMainTask::ActionResultCallBack(int nLinkEventId, int nActionId,  char *szTriggerTime, long lResult, char *szResulltDesc)
// {
// 	CRS_ACTION_KEY pActionKeyCopy ;
// 
// 	//���²�����ֵ
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
// 	//������Ϣ
// 	long lRet = MAIN_TASK->putq(pMsg);
// 	return lRet;
// }
//ע�� ycd
// void CMainTask::CRSEventTrigger(const CRS_CURTIRGGER* pTrigger, const char* szMsg)
// {
// 	//�������Ϊ�գ��ȼ�Ϊ'i_c_v'�����¼���¼ʱ�ٸ�Ϊ�գ��˲�����Ҫ���sscanf����������ʱ�ͽ���
// 	char szTempTriggerDesc[CV_CRS_DESC_MAXLEN] = {0};
// 	if(pTrigger->triggerInfo.szTriggerDesc != NULL && strlen(pTrigger->triggerInfo.szTriggerDesc) == 0)
// 		strcpy(szTempTriggerDesc, REPLACESTR);
// 	else if(pTrigger->triggerInfo.szTriggerDesc != NULL)
// 		strcpy(szTempTriggerDesc, pTrigger->triggerInfo.szTriggerDesc);
// 	//_("����ԴID:%d; ����ʱ��:%d; ����:%s; ����:%s; ����:%s; ����Դ����:%d; �Ƿ��ֶ�:%d; ״̬:%s; ����:%s")
// 	CRSEvent(pTrigger->triggerInfo.szOperator, EA_SUBTYPE_CRS_TRIGGER, "", "triggerID:%d; triggertime:%d; triggername:%s; triggerdesc:%s; triggergroup:%s; triggerype:%d; ismanual:%d; triggerstatus:%s; triggerconfig:%s",  
// 		pTrigger->triggerInfo.nTriggerID, pTrigger->triggerInfo.lTriggerTime, pTrigger->triggerInfo.szTriggerName, 
// 		szTempTriggerDesc, pTrigger->triggerInfo.szTriggerPath, pTrigger->triggerInfo.uTriggerType,
// 		pTrigger->triggerInfo.uExecuteType, szMsg, pTrigger->triggerInfo.szTriggerCfg);
// }
//ע�� ycd
// void CMainTask::CRSEventAction(const LINK_ACTION* curAction, const char* szMsg)
// {
// 	//�������Ϊ�գ��ȼ�Ϊ'i_c_v'�����¼���¼ʱ�ٸ�Ϊ�գ��˲�����Ҫ���sscanf����������ʱ�ͽ���
// 	char szTempActionDesc[CV_CRS_ITEMDESC_MAXLEN] = { 0 };
// 	if(curAction->szActionName != NULL && strlen(curAction->szActionDesc) == 0)
// 		strcpy(szTempActionDesc, REPLACESTR);
// 	else if(curAction->szActionDesc != NULL)
// 		strcpy(szTempActionDesc, curAction->szActionDesc);
// 
// 	//_("����ԴID:%d; ����ʱ��:%d; ����ID:%d; ��������:%s; ��������:%s; ִ��ʱ��:%d; Ԥ������:%s; Ԥ��ID:%d; Ԥ������:%s; Ԥ������:%s; Ԥ�����ȼ�:%d; Ԥ��Ȩ��:%s; ִ������:%d; ״̬:%s; ��������:%s"), 
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

// ���ڿͻ���ִ�еĶ������͵�web����
//ע�� ycd
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
// 	PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::PublishAction2MenDB() ִ�ж�����ActionID��%d,ActionType��%s",curAction->lItemID,curAction->szActionType);
// 	if (PKStringHelper::StriCmp(curAction->szActionType,"poppage") == 0)   // �����ǰ���������ǵ�������
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
// 		PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::PublishAction2MenDB()����һ������ִ����Ϣ�� action-client�����ݣ�%s",strjsonAction.c_str());
// 		m_redisPublish.PublishMsg(CHANNELNAME_LOG,strjsonAction);
// 		//PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::PublishAction2MenDB()����һ������ִ����Ϣ�� log-channel�����ݣ�%s",strjsonAction.c_str());
// 	}else if (PKStringHelper::StriCmp(curAction->szActionType,"popmessage") == 0)   // �����ǰ�����ǵ�����Ϣ��
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
// 		PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::PublishAction2MenDB()����һ������ִ����Ϣ�� action-client�����ݣ�%s",strjsonAction.c_str());
// 		m_redisPublish.PublishMsg(CHANNELNAME_LOG,strjsonAction);
// 		//PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::PublishAction2MenDB()����һ������ִ����Ϣ�� log-channel�����ݣ�%s",strjsonAction.c_str());
// 	}else if (PKStringHelper::StriCmp(curAction->szActionType,"noaction") == 0)
// 	{
// 		PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::PublishAction2MenDB()���������ǲ����������Ѿ��ɹ����� actionID:%d,actionType:%s",curAction->lItemID,curAction->szActionType);
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
// дtag��
/*
	//ע�� ycd
	�Ժ�ο�
*/
// int CMainTask::WriteTag2MenDB(LINK_ACTION &curAction,int nTriggerID,int nPriority)
// {
// 	Json::Value jsonAction;
// 	string strAction;
// 
// 	char szTmLink[PK_TAGDATA_MAXLEN] = {'\0'};
// 	PKTimeHelper::GetCurTimeString(szTmLink,sizeof(szTmLink));
// 
// 	PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::WriteTag2MenDB() ִ�ж�����ActionID��%d,ActionType��%s",curAction.lItemID,curAction.szActionType);
// 
// 	if (PKStringHelper::StriCmp(curAction.szActionType,"writetag") == 0)
// 	{
// 		jsonAction.clear();
// 		jsonAction["action"] = curAction.szActionType;
// 		jsonAction["n"] = curAction.szParam;
// 		jsonAction["v"] = curAction.szParam1;
// 		strAction = jsonAction.toStyledString();
// 		// ����д��redis
// 		m_redisPublish.set(curAction.szParam,strAction.c_str());
// 		PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::WriteTag2MenDB()����һ����Ϣ�� control�����ݣ�%s",strAction.c_str());
// 		// ������־
// 		jsonAction["logtype"] = "link";
// 		jsonAction["actionid"] = curAction.lItemID;
// 		jsonAction["triggerid"] = nTriggerID;
// 		jsonAction["linktime"] = szTmLink;
// 		strAction = jsonAction.toStyledString();
// 		m_redisPublish.PublishMsg(CHANNELNAME_LOG,strAction);
// 		//PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::WriteTag2MenDB()����һ����־��Ϣ��log-channel�����ݣ�%s",strAction.c_str());
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
// 		for (;iter != CONFIG_LOADER->m_lstUsableTagList.end();iter++)     //���ݿ��õ�tag����IDѰ����õ�tag����Ϣ��������tag����
// 		{
// 			USABLETAG *usableTag = &*iter;
// 
// 			if (usableTag->nID == nUsableTagID)     // �ҵ��˿��õ�TAG����
// 			{
// 				PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::WriteTag2MenDB()��ʼִ�п���tag����:%d",nUsableTagID);
// 				for (int i = 0;i < vecCams.size();i++)     // ����action��ÿһ��ֵ
// 				{
// 					
// 					int nLastUsed = -1;   // ����ǵ�һ��ʹ�ã�֮ǰû��tag�㱻ʹ�õĻ���������ֵ��ʹ��Ĭ�ϵģ����һ����-1+1=0�����պ�
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
// 							PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::WriteTag2MenDB()��ʼִ�п���tag����:%d",nUsableTagID);
// 							// ���õ�ǰʹ�õ�tag����Ϣ
// 							usableTag->vecUsenableTagCurInfos[nLastUsed+1].nCurPriority = nPriority;
// 							usableTag->vecUsenableTagCurInfos[nLastUsed+1].nLastAcionID = nTriggerID;
// 							usableTag->vecUsenableTagCurInfos[nLastUsed+1].bLastUsed = 1;
// 
// 							strTagName = usableTag->vecUsenableTagCurInfos[nLastUsed+1].szTagName;
// 							strTagValue = vecCams[i];
// 							// ������Ϣ
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
// 							// ����д��redis
// 							m_redisPublish.PublishMsg("control",strAction);
// 							PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::WriteTag2MenDB()����һ���л�����ͷ��Ϣ�� control�����ݣ�%s",strAction.c_str());
// 
// 							// ������־
// 							jsonAction["logtype"] = "link";
// 							jsonAction["action"] = curAction.szActionType;
// 							jsonAction["actionid"] = curAction.lItemID;
// 							jsonAction["triggerid"] = nTriggerID;
// 							jsonAction["usablecname"] = szUsableTagCName;
// 							jsonAction["tagcname"] = szTagCName;
// 							jsonAction["linktime"] = szTmLink;
// 							strAction = jsonAction.toStyledString();
// 							m_redisPublish.PublishMsg(CHANNELNAME_LOG,strAction);
// 							//PKLog.LogMessage(PK_LOGLEVEL_INFO,"CExecutionTask::WriteTag2MenDB()����һ����־��Ϣ��log-channel�����ݣ�%s",strAction.c_str());
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
	ACE_Time_Value	tvCheckConn;		// ɨ�����ڣ���λms
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

// ��ʱ��ѯ�����ڵ���.json��ʽ����־��Ϣ��������level,time�����ֶ�
int CMainTask::handle_timeout(const ACE_Time_Value &current_time, const void *act)
{
	// ÿ5���ȡһ���Ƿ��ӡ��־�ı��
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
		//��ȡ����id��
		stru_Link_Action.nActionId = cmd.Field(1).asInterval();
		//��ȡ�¼���
		stru_Link_Action.strName = cmd.Field(3).asString();
		//��ȡ��������
		stru_Link_Action.nActionTypeId = cmd.Field(2).asInterval();
		//��ȡʹ��
		stru_Link_Action.nActionEnable = cmd.Field(4).asInterval();
		//��ȡ����ID
		stru_Link_Action.nEventId = cmd.Field(5).asInterval();
		//��ȡ����1-4
		stru_Link_Action.strParam1 = cmd.Field(6).asString();
		stru_Link_Action.strParam2 = cmd.Field(7).asString();
		stru_Link_Action.strParam3 = cmd.Field(8).asString();
		stru_Link_Action.strParam4 = cmd.Field(9).asString();

		cmainTask.m_mapId2Event.insert(map<int,TAG_LINK_ACTION*>::value_type(stru_Link_Action.nActionId,&stru_Link_Action));
	}
	return 0;
}
