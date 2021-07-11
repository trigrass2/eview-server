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
#include "ace/High_Res_Timer.h"
#include "MainTask.h"
#include "common/gettimeofday.h"
#include "pkeviewdbapi/pkeviewdbapi.h"
#include "pklog/pklog.h"
#include "json/json.h"
#include "common/eviewdefine.h"
using namespace std;
extern CPKLog g_logger;

#ifndef __sun
using std::map;
using std::string;
#endif

#define EVENT_ACTION_DLL_PREFIX_LEN		10		// pklinkact_ // pklinkevt_
#define EVENT_DLLNAME_PREFIX				"pklinkevt_"
#define ACTION_DLLNAME_PREFIX				"pklinkact_"

#define MSGTYPE_ACTIONRESULT_FROM_ACTIONTASK			1		// �����̷߳��͹�������Ϣ���ͣ�����ִ�н����
#define MSGTYPE_EVENTOCCURE_FROM_EVENT				2		// �¼��̷߳��͹�������Ϣ���ͣ��¼���


#define NULLASSTRING(x) x==NULL?"":x
extern int GetStringFromMsg(ACE_Message_Block *pMsgBlk, string &strValue);

#include <fstream>  
#include <streambuf>  
#include <iostream>  

// �õ�PKMemDB�Ķ˿ں�PKMemDB������
int GetPKMemDBPort(int &nListenPort, string &strPassword)
{
	nListenPort = PKMEMDB_LISTEN_PORT;
	strPassword = PKMEMDB_ACCESS_PASSWORD;

	string strConfigPath = PKComm::GetConfigPath();
	strConfigPath = strConfigPath + PK_OS_DIR_SEPARATOR + "pkmemdb.conf";
	std::ifstream textFile(strConfigPath.c_str());
	if (!textFile.is_open())  // �����ڸ��ļ�������Ϊ��sqlite��eview.db
	{
		// g_logger.LogMessage(PK_LOGLEVEL_ERROR, "db config file:%s not exist", strConfigPath.c_str());
		return -1;
	}

	// �����ļ���С  
	textFile.seekg(0, std::ios::end);
	std::streampos len = textFile.tellg();
	textFile.seekg(0, std::ios::beg);

	// ����1  
	std::string strConfigData((std::istreambuf_iterator<char>(textFile)), std::istreambuf_iterator<char>());
	textFile.close();
	vector<string> vecLine = PKStringHelper::StriSplit(strConfigData, "\n");
	for (int iLine = 0; iLine < vecLine.size(); iLine++)
	{
		string strLine = vecLine[iLine];
		vector<string> vecContent = PKStringHelper::StriSplit(strLine, " ");
		if (vecContent.size() < 2)
			continue;
		string strKey = vecContent[0];
		string strValue = vecContent[1];
		if (strKey.compare("port") == 0)
			nListenPort = ::atoi(strValue.c_str());
		if (strKey.compare("requirepass") == 0)
			strPassword = strValue.c_str();
	}
	return 0;
}


int PutStrToMsg(char *szStr, ACE_Message_Block *pMsg)
{
	// �ַ������� szActionId
	int nStrLen = strlen(szStr);
	memcpy(pMsg->wr_ptr(), &nStrLen, 4);
	pMsg->wr_ptr(4);
	// �ַ�������
	if(nStrLen > 0)
	{
		memcpy(pMsg->wr_ptr(), szStr, nStrLen);
		pMsg->wr_ptr(nStrLen);
	}
	return 0;
}

// ActionTask��MainTask���Ͷ���ִ�н��
int CMainTask::PostActionResult(char *szActionId, int nResultCode, char *szResultDesc)
{
	// ÿ���ַ����������ȣ�4���ֽڣ�+���ݣ�N���ֽڣ�
	int nCtrlMsgLen = 4 * 3 + strlen(szActionId) + strlen(szResultDesc);
	ACE_Message_Block *pMsg = new ACE_Message_Block(nCtrlMsgLen);

	int nCmdCode = MSGTYPE_ACTIONRESULT_FROM_ACTIONTASK;
	// ����
	memcpy(pMsg->wr_ptr(), &nCmdCode, 4);
	pMsg->wr_ptr(4);

	PutStrToMsg(szActionId,pMsg);
	PutStrToMsg(szResultDesc,pMsg);

	putq(pMsg);
	this->reactor()->notify(this, ACE_Event_Handler::WRITE_MASK); 
	return PK_SUCCESS;
}

// �ֶ�����(�¼�ID���¼��������ȼ����¼�����) ������MainTask. EventParamΪÿ���¼�Դ��ͬ���籨������Ϊ��tagalarmcname+subsys+..
int OnEvent(char *szEventId, char *szEventName, char *szPriority, char *szProduceTime,char *szEventParam)
{
	//�����¼�ID��ȡ����ID
	// ÿ���ַ����������ȣ�4���ֽڣ�+���ݣ�N���ֽڣ�
	int nCtrlMsgLen = 4 * 6 + strlen(szEventId) + strlen(szEventName) + strlen(szPriority) + strlen(szEventParam) + strlen(szProduceTime);
	ACE_Message_Block *pMsg = new ACE_Message_Block(nCtrlMsgLen);

	int nCmdCode = MSGTYPE_EVENTOCCURE_FROM_EVENT;
	// ����
	memcpy(pMsg->wr_ptr(), &nCmdCode, 4);
	pMsg->wr_ptr(4);

	PutStrToMsg(szEventId, pMsg);
	PutStrToMsg(szEventName, pMsg);
	PutStrToMsg(szPriority, pMsg);
	PutStrToMsg(szProduceTime, pMsg);
	PutStrToMsg(szEventParam, pMsg);

	MAIN_TASK->putq(pMsg);
	MAIN_TASK->reactor()->notify(MAIN_TASK, ACE_Event_Handler::WRITE_MASK); 
	return PK_SUCCESS;
}

CMainTask::CMainTask()
{
	m_bStop = false;
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
	long lRet = (long)activate(THR_NEW_LWP | THR_JOINABLE |THR_INHERIT_SCHED, 1);
	if(lRet != PK_SUCCESS)
	{
		lRet = -2;
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, _("CExecutionTask: StartTask() Failed, RetCode: %d"), lRet);
	}

    this->reactor()->register_handler(this, ACE_Event_Handler::READ_MASK | ACE_Event_Handler::WRITE_MASK);
	return PK_SUCCESS;	
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
	m_bStop = true;
	this->reactor()->end_reactor_event_loop();

	int nWaitResult = wait();

	return PK_SUCCESS;	
}


// ��ʱ��ѯ�����ڵ���.json��ʽ����־��Ϣ��������level,time�����ֶ�
// int CMainTask::handle_timeout(const ACE_Time_Value &current_time, const void *act)
// {
// 	// ÿ5���ȡһ���Ƿ��ӡ��־�ı��
// 	int nRet = PK_SUCCESS;
// 	time_t tmNow;
// 	time(&tmNow);
// 	return PK_SUCCESS;
// }

//�������յ���event-alarm  �����ı�����Ϣmsg  ��ִ�ж����ķ���,�������� cmdcode
int CMainTask::handle_output(ACE_HANDLE fd /*= ACE_INVALID_HANDLE*/ )
{
	ACE_Message_Block *pMsgBlk = NULL;
    ACE_Time_Value tvTimes = ACE_OS::gettimeofday();
	while(getq(pMsgBlk, &tvTimes) >= 0)
	{
		// �����
		int nCmdCode = 0;
		memcpy(&nCmdCode, pMsgBlk->rd_ptr(), 4);
		pMsgBlk->rd_ptr(4);
		//����������Ϣ
		if(nCmdCode == MSGTYPE_ACTIONRESULT_FROM_ACTIONTASK)
		{
			// char *szActionId, int nResultCode, char *szResultDesc
			string strActionId, strResultDesc;
			int nResultCode = 0;
			GetStringFromMsg(pMsgBlk, strActionId);
			//memcpy(&nResultCode, pMsgBlk->rd_ptr(), sizeof(int));
			GetStringFromMsg(pMsgBlk, strResultDesc);

			// �ҵ���Ӧ������ִ�еĶ���
			map<string, LINK_ACTIONING_INFO*>::iterator itActioning = m_mapId2Actioning.find(strActionId);
			if(itActioning == m_mapId2Actioning.end())
            {
                pMsgBlk->release();
				g_logger.LogMessage(PK_LOGLEVEL_ERROR,"���صĶ�������ִ���У�����");
				continue;
			}

			// ��¼�����������ʷ�¼�
			LINK_ACTIONING_INFO pActioning = *(itActioning->second);
			pActioning.strDesc = strResultDesc;
			int nRet = SendActionInfoToEventServer(&pActioning);
			m_mapId2Actioning.erase(itActioning);
		}
		//�¼�
		else if(nCmdCode == MSGTYPE_EVENTOCCURE_FROM_EVENT)
		{
			// char *szEventId, char *szEventName, char *szPriority, char *szEventParam;
            string strEventId, strEventName, strPriority, strProduceTime, strEventParam;
			GetStringFromMsg(pMsgBlk, strEventId);
			GetStringFromMsg(pMsgBlk, strEventName);
			GetStringFromMsg(pMsgBlk, strPriority);
            GetStringFromMsg(pMsgBlk, strProduceTime);
			GetStringFromMsg(pMsgBlk, strEventParam);

            //-----------------SMS----------------
            Json::Reader jsonReader;
            Json::Value jsonEventParam;
            if (!jsonReader.parse(strEventParam.c_str(), jsonEventParam))
            {
              pMsgBlk->release();
              g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Parse Json Event Parameters <%s>.", strEventParam.c_str());
              return -5;
            }

            std::string strSubsys, strAlarmName;
            Json::Value jsonSubsys;
            if (jsonEventParam.get(ALARM_SYSTEMNAME, jsonSubsys).isNull())
              strSubsys = "";
            else
              strSubsys = jsonEventParam[ALARM_SYSTEMNAME].asString();
            
            g_logger.LogMessage(PK_LOGLEVEL_INFO, "Json Event Parameters Subsys<%s> AlarmName<%s>.", strSubsys.c_str(), strAlarmName.c_str());

            for (map<string, EVENT_ACTION_INFO *>::iterator item = m_mapId2Event.begin(); item != m_mapId2Event.end(); item++)
            {
              if (item->second->nEventId != ::atoi(strEventId.c_str()))
                continue;

              for (vector<LINK_ACTION_INFO *>::iterator itemAction = item->second->vecAction.begin(); itemAction != item->second->vecAction.end(); itemAction++)
              {
                LINK_ACTION_INFO *pAction = *itemAction;

                g_logger.LogMessage(PK_LOGLEVEL_INFO, "SMS Event Action <%s> Found.", pAction->strActionName.c_str());

                LINK_ACTIONING_INFO *pActioning = new LINK_ACTIONING_INFO();
                pActioning->strActionId = pAction->strActionId;
                pActioning->nActionTypeId = pAction->nActionTypeId;
                pActioning->strActionName = pAction->strActionName;
                pActioning->nEventId = pAction->nEventId;
                pActioning->nExecuteOrder = pAction->nExecuteOrder;
                pActioning->strParam1 = pAction->strParam1;
                pActioning->strParam2 = pAction->strParam2;
                pActioning->strParam3 = pAction->strParam3;
                pActioning->strParam4 = pAction->strParam4;
                pActioning->strDesc = pAction->strDesc;
                pActioning->strFailTip = pAction->strFailTip;

                pActioning->strSubsys = strSubsys;
                pActioning->strActionTime = pAction->strActionTime;
                pActioning->strActionTypeName = pAction->strActionTypeName;

                pAction->strSubsys = strSubsys;

                m_mapId2Actioning[pActioning->strActionId] = pActioning;

                map<string, CActionTask *>::iterator actionImplement = m_mapActionType2Task.find(pActioning->strActionTypeName);
                if (actionImplement != m_mapActionType2Task.end())
                {
                  actionImplement->second->PostAction((char*)pAction->strActionId.c_str(), (char*)pAction->strParam1.c_str(), (char*)pAction->strParam2.c_str(), (char*)pAction->strParam3.c_str(), (char*)pAction->strParam4.c_str(), (char*)strPriority.c_str(), (char*)strEventParam.c_str());
                  g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "Action Name<%s>, Event Name<%s> Parame<%s> Done.", pAction->strActionName.c_str(), strEventName.c_str(), pAction->strParam1.c_str());
                }
                else
                {
                  g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Action Name<%s>, Event Name<%s> Not Implement.", pAction->strActionName.c_str(), strEventName.c_str());
                }
              }
            }
            //-----------------SMS----------------
          

            //------------�о��߼�������------------
			// ���Ҷ�Ӧ���¼��Ķ���
			map<string, EVENT_ACTION_INFO *>::iterator itEventAction = m_mapId2Event.find(strEventId);
			if(itEventAction == m_mapId2Event.end())
            {
                pMsgBlk->release();
				g_logger.LogMessage(PK_LOGLEVEL_ERROR,"û���ҵ�EventId = %s �Ķ���",strEventId.c_str());
				continue;
			}
			EVENT_ACTION_INFO *pEventAction = itEventAction->second;
			
			// ��˳��ִ�ж���������������Ϣ��¼����
			for(vector<LINK_ACTION_INFO *>::iterator itEvtAct = pEventAction->vecAction.begin(); itEvtAct != pEventAction->vecAction.end(); itEvtAct ++)
			{
				LINK_ACTION_INFO *pAction = *itEvtAct;
				
				LINK_ACTIONING_INFO *pActioning =new LINK_ACTIONING_INFO();
				pActioning->nActionTypeId = pAction->nActionTypeId;
				pActioning->nEventId = pAction->nEventId;
				pActioning->nExecuteOrder = pAction->nExecuteOrder;
				pActioning->strActionId = pAction->strActionId;
				pActioning->strActionName= pAction->strActionName;
				pActioning->strActionTime = pAction->strActionTime;
				pActioning->strActionTypeName = pAction->strActionTypeName;
				pActioning->strDesc = pAction->strDesc;
				pActioning->strFailTip = pAction->strFailTip;
				pActioning->strParam1= pAction->strParam1;
				pActioning->strParam2 = pAction->strParam2;
				pActioning->strParam3 = pAction->strParam3;
				pActioning->strParam4 = pAction->strParam4;
				pActioning->strSubsys = strSubsys;
				pAction->strSubsys = strSubsys;
				m_mapId2Actioning[pActioning->strActionId] = pActioning;


				map<string,  CActionTask *>::iterator itType2Dll = m_mapActionType2Task.find(pActioning->strActionTypeName);
				if(itType2Dll != m_mapActionType2Task.end())
				{
					// "writetag"//дһ����,"cyclewritetag"//д�����,"poppage"//��������
                  itType2Dll->second->PostAction((char*)pAction->strActionId.c_str(), (char*)pAction->strParam1.c_str(), (char*)pAction->strParam2.c_str(), (char*)pAction->strParam3.c_str(), (char*)pAction->strParam4.c_str(), (char*)strPriority.c_str(), (char*)strEventParam.c_str());
					g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "�¼�id:%s,�¼�����:%s, ��������:%s, ��������:%s,����2:%s", 
                      strEventId.c_str(), strEventName.c_str(), pActioning->strActionTypeName.c_str(), pAction->strParam1.c_str(), pAction->strParam2.c_str());
				}
				else
				{
					g_logger.LogMessage(PK_LOGLEVEL_ERROR, "�¼�id:%s,�¼�����:%s, ���ݶ�������:%s δ�ҵ���������ģ��, ��������:%s,����2:%s", 
                      strEventId.c_str(), strEventName.c_str(), pActioning->strActionTypeName.c_str(), pAction->strParam1.c_str(), pAction->strParam2.c_str());
				}
			} 		

            //------------�о��߼�������------------
		}
		 
		pMsgBlk->release();
	} // while(getq(pMsgBlk, &tvTimes) >= 0)
	//���������ҵ�����action

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
	ACE_High_Res_Timer::global_scale_factor();
	this->reactor()->owner(ACE_OS::thr_self ());

	int nRet = PK_SUCCESS;
	nRet = OnStart();

	char szTime[PK_HOSTNAMESTRING_MAXLEN] = {'\0'};
	PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));

	ACE_Time_Value tvSleep;
	tvSleep.msec(100); // 100ms

	while(!m_bStop)
	{
		this->reactor()->reset_reactor_event_loop();
		this->reactor()->run_reactor_event_loop();
	}

	g_logger.LogMessage(PK_LOGLEVEL_INFO,"CMainTask stopped");
	OnStop();

	return PK_SUCCESS;	
}

int CMainTask::OnStart()
{ 
	InitRedis();
	m_pReactor->cancel_timer((ACE_Event_Handler *)this);
	ACE_Time_Value	tvCheckConn;		// ɨ�����ڣ���λms
	tvCheckConn.msec(10 * 1000); 
	ACE_Time_Value tvStart = ACE_Time_Value::zero; 
	m_pReactor->schedule_timer((ACE_Event_Handler *)this, NULL, ACE_Time_Value::zero, tvCheckConn);

	// �����ݿ�����¼��Ͷ����Ĺ�ϵ��Ϣ
	LoadConfigFromDB();

	// �ӱ����ļ������¼��Ͷ���dll������
	LoadEventAndActionDll();

	return 0;
}

int CMainTask::OnStop()
{
	vector<LINK_EVENT_DLL *>::iterator itLinkEvtDll = m_vecEventDll.begin();
	for(;itLinkEvtDll != m_vecEventDll.end();itLinkEvtDll++)
	{
		(*itLinkEvtDll)->dllEvent.close();
		(*itLinkEvtDll)->pfnExit = NULL;
		(*itLinkEvtDll)->pfnInit = NULL;
		(*itLinkEvtDll)->strEventDllName = "";
		(*itLinkEvtDll)->strEventName="";
	}
	m_vecEventDll.clear();
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

bool compareLinkAction(LINK_ACTION_INFO * left, LINK_ACTION_INFO *right)
{
	// ��С��������
	return left->nExecuteOrder < right->nExecuteOrder;
}
//��ȡȫ���¼�
long CMainTask::LoadConfigFromDB()
{
	// ��ѯָ���������б�
	long lQueryId = -1;
    vector<vector<string> > vecEvents;
	CPKEviewDbApi PKEviewDbApi;
	int nRet = PKEviewDbApi.Init();
	char szSql[512] = {0};
	sprintf(szSql,"select A.id as id, A.actiontype_id as actiontypeid,A.name as actionname,B.name as actiontypename, event_id,param1,param2,param3,param4,description,failtip from t_link_action A,t_link_action_type B where A.actiontype_id=B.id and A.enable=1");
	int nDrvNum = 0;
	string strError;
	nRet = PKEviewDbApi.SQLExecute(szSql, vecEvents, &strError);
	if(nRet != 0)
	{
		PKEviewDbApi.Exit();
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "��ѯ���ݿ�ʧ��,sql:%s, error:%s", szSql, strError.c_str());
		return -2;
	}

	for(int iEvent = 0; iEvent < vecEvents.size(); iEvent ++)
	{
		vector<string> &row = vecEvents[iEvent];
		int iCol = 0;
		string strId = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strActionTypeId = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strActionName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strActionTypeName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strEventId = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strParam1 = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strParam2 = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strParam3 = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strParam4 = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDescription = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strFailTip = NULLASSTRING(row[iCol].c_str());

		if(strEventId.empty())
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Event Action EventID Is NULL, Action Name<%s>.", strActionName.c_str());
			continue;
		}

		EVENT_ACTION_INFO *pEvtAction = new EVENT_ACTION_INFO();
        pEvtAction->nEventId = atoi(strEventId.c_str());

		LINK_ACTION_INFO *pLinkActioninfo = new LINK_ACTION_INFO();
		pLinkActioninfo->strActionId = strId;
		pLinkActioninfo->nActionTypeId =atoi(strActionTypeId.c_str());
		pLinkActioninfo->strActionName = strActionName;
		pLinkActioninfo->nEventId = atoi(strEventId.c_str());
		pLinkActioninfo->strParam1 = strParam1;
		pLinkActioninfo->strParam2 = strParam2;
		pLinkActioninfo->strParam3 = strParam3;
		pLinkActioninfo->strParam4 = strParam4;
		pLinkActioninfo->strDesc = strDescription;
		pLinkActioninfo->strFailTip = strFailTip;
		pLinkActioninfo->strActionTypeName = strActionTypeName;

		(pEvtAction->vecAction).push_back(pLinkActioninfo);
		std::sort(pEvtAction->vecAction.begin(), pEvtAction->vecAction.end(), compareLinkAction);
        
        m_mapId2Event[strId] = pEvtAction;
	}
    
    PKEviewDbApi.Exit();
    g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "��ѯ���ݿ� %d !", vecEvents.size());
	return 0;
}


// / �ӱ����ļ������¼��Ͷ���dll������
long CMainTask::LoadEventAndActionDll()
{
	string strBinPath = PKComm::GetBinPath();
	list<string> listFileNames;
	PKFileHelper::ListFilesInDir(strBinPath.c_str(), listFileNames);
	for(list<string>::iterator itFile = listFileNames.begin(); itFile != listFileNames.end(); itFile ++)
	{
		string strFileName = *itFile;
		string strFileExt = "";

		int nPos = strFileName.find_last_of('.');
		if(nPos != std::string::npos)
		{
			strFileExt = strFileName.substr(nPos + 1);
			strFileName = strFileName.substr(0, nPos); // ȥ����չ��
		}

		if(strFileName.length() < EVENT_ACTION_DLL_PREFIX_LEN)
			continue;
		if(PKStringHelper::StriCmp(strFileExt.c_str(), "dll") != 0)
			continue;

		string strFilePrefix = strFileName.substr(0, EVENT_ACTION_DLL_PREFIX_LEN);
		if(PKStringHelper::StriCmp(strFilePrefix.c_str(), EVENT_DLLNAME_PREFIX) == 0) // �ҵ�һ���¼�ģ��
		{
			string strEventName = strFileName.substr(EVENT_ACTION_DLL_PREFIX_LEN);
			
            int nPos = strEventName.find_last_of('.');
            if (nPos != std::string::npos)
			{
				strEventName = strEventName.substr(0, nPos); // ȥ����չ��
			}

			LINK_EVENT_DLL *pEventDll = new LINK_EVENT_DLL();
			pEventDll->strEventName = strEventName;
			pEventDll->strEventDllName = strFileName;
			//��ȡ�¼�DLL
			string strFullEventDllPath = strBinPath+"\\"+(*itFile);
			long nErr = pEventDll->dllEvent.open(strFullEventDllPath.c_str());
			if (nErr != 0)
			{
				g_logger.LogMessage(PK_LOGLEVEL_ERROR,"Load %s failed!\n", strFullEventDllPath.c_str());
				return nErr;
			}
			g_logger.LogMessage(PK_LOGLEVEL_NOTICE,"Load %s success!\n", strFullEventDllPath.c_str());

			pEventDll->pfnInit = (PFN_InitEvent)pEventDll->dllEvent.symbol(PFNNAME_INITEVENT);
			pEventDll->pfnExit =(PFN_ExitEvent)pEventDll->dllEvent.symbol(PFNNAME_EXITEVENT);
			if((pEventDll->pfnExit == NULL )||(pEventDll->pfnExit == NULL))
			{
				g_logger.LogMessage(PK_LOGLEVEL_ERROR,"�¼�DLL�ĵ���������ȡʧ��;");
				return -100;
			}
			pEventDll->pfnInit(OnEvent);
			//hEventDll.close();
			m_vecEventDll.push_back(pEventDll);
		}
		else if(PKStringHelper::StriCmp(strFilePrefix.c_str(), ACTION_DLLNAME_PREFIX) == 0) // �ҵ�һ������ģ��
		{
			string strActionName = strFileName.substr(EVENT_ACTION_DLL_PREFIX_LEN);
            int nPos = strActionName.find_last_of('.');
            if (nPos != std::string::npos)
				strActionName = strActionName.substr(0, nPos); // ȥ����չ��
			CActionTask *pActionTask = new CActionTask();
			pActionTask->SetActionInfo(strActionName.c_str(), strFileName.c_str());
			m_mapActionType2Task[strActionName] = pActionTask;
			pActionTask->Start();
		}
	}
	return 0;
}
//���͵�logchannel
int CMainTask::SendActionInfoToEventServer(LINK_ACTIONING_INFO *linkActionInfo)
{
	string str2EventLog="";
	MSG2String(linkActionInfo,str2EventLog);
	//m_redisRW.PublishMsg(strEventLogChannel,str2EventLog);
	m_redisRW.publish(CHANNELNAME_EVENT, str2EventLog.c_str());
	m_redisRW.commit();
	g_logger.LogMessage(PK_LOGLEVEL_INFO,"���������������¼��ɹ�,EventId = %d",linkActionInfo->nEventId);
	return 0;
}
int CMainTask::InitRedis()
{
	int nRet = 0;

	int nPKMemDBListenPort = PKMEMDB_LISTEN_PORT;
	string strPKMemDBPassword = PKMEMDB_ACCESS_PASSWORD;
	GetPKMemDBPort(nPKMemDBListenPort, strPKMemDBPassword);

	nRet = m_redisRW.initialize(PK_LOCALHOST_IP, nPKMemDBListenPort, strPKMemDBPassword.c_str(), 0);
	if(nRet != PK_SUCCESS)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR,"m_redisSub.Initialize failed,ret:%d",nRet);
		return nRet;
	}

	return nRet;
}
/*
Json::Value jUserName = jsonCmd["username"];
Json::Value jTagName = jsonCmd["isconfirmed"];
Json::Value jOperType = jsonCmd["opertype"];
Json::Value jOperTime = jsonCmd["opertime"];
Json::Value jModule = jsonCmd["module"];
Json::Value jOperContext = jsonCmd["opercontext"];
linkActionInfo.desc ,desc;time
*/
int CMainTask::MSG2String(LINK_ACTIONING_INFO* linkActionInfo,string &str2EventLog)
{
	Json::Value root;
	//vector<string> m_vecDesc = PKStringHelper::StriSplit(linkActionInfo->strDesc,";");
	//����
	root[EVENT_TYPE_MSGID] = EVENT_TYPE_LINK;
	root["username"]="admin";
	root["maincat"]=linkActionInfo->strActionTypeName; // t_link_action_type���name�ֶΡ�������writrtag��ZTAction��TimerAction��
	root["actionname"] = linkActionInfo->strActionName; // t_link_action���name�ֶ�
	char szDateTime[PK_NAME_MAXLEN] = {0};
	string strCurTime = PKTimeHelper::GetCurTimeString(szDateTime, sizeof(szDateTime));
	root["rectime"] = strCurTime; //m_vecDesc[1]; 
	root["content"] = linkActionInfo->strDesc; //m_vecDesc[0];
	root["subsys"] = linkActionInfo->strSubsys; // ����ʱ������tag��������ϵͳ
	//if (linkActionInfo->nActionTypeId==0)
	//{
	//	root["opertype"]="��������";
	//}
	////ȷ��
	//if (linkActionInfo->nActionTypeId==1)
	//{
	//	root["opertype"]="ȷ�ϱ���";
	//}
	////�ظ�
	//if (linkActionInfo->nActionTypeId==2)
	//{
	//	root["opertype"]="�ָ�����";
	//}
	//else
	//	root["opertype"]="����";
	str2EventLog = root.toStyledString();
	return 0;
}
int CMainTask::CheckEventTimeout()
{
	return 0;
}
