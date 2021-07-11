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

#define MSGTYPE_ACTIONRESULT_FROM_ACTIONTASK			1		// 动作线程发送过来的消息类型（动作执行结果）
#define MSGTYPE_EVENTOCCURE_FROM_EVENT				2		// 事件线程发送过来的消息类型（事件）


#define NULLASSTRING(x) x==NULL?"":x
extern int GetStringFromMsg(ACE_Message_Block *pMsgBlk, string &strValue);

#include <fstream>  
#include <streambuf>  
#include <iostream>  

// 得到PKMemDB的端口号PKMemDB的密码
int GetPKMemDBPort(int &nListenPort, string &strPassword)
{
	nListenPort = PKMEMDB_LISTEN_PORT;
	strPassword = PKMEMDB_ACCESS_PASSWORD;

	string strConfigPath = PKComm::GetConfigPath();
	strConfigPath = strConfigPath + PK_OS_DIR_SEPARATOR + "pkmemdb.conf";
	std::ifstream textFile(strConfigPath.c_str());
	if (!textFile.is_open())  // 不存在该文件，则认为是sqlite：eview.db
	{
		// g_logger.LogMessage(PK_LOGLEVEL_ERROR, "db config file:%s not exist", strConfigPath.c_str());
		return -1;
	}

	// 计算文件大小  
	textFile.seekg(0, std::ios::end);
	std::streampos len = textFile.tellg();
	textFile.seekg(0, std::ios::beg);

	// 方法1  
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
	// 字符串长度 szActionId
	int nStrLen = strlen(szStr);
	memcpy(pMsg->wr_ptr(), &nStrLen, 4);
	pMsg->wr_ptr(4);
	// 字符串内容
	if(nStrLen > 0)
	{
		memcpy(pMsg->wr_ptr(), szStr, nStrLen);
		pMsg->wr_ptr(nStrLen);
	}
	return 0;
}

// ActionTask向MainTask发送动作执行结果
int CMainTask::PostActionResult(char *szActionId, int nResultCode, char *szResultDesc)
{
	// 每个字符串包含长度（4个字节）+内容（N个字节）
	int nCtrlMsgLen = 4 * 3 + strlen(szActionId) + strlen(szResultDesc);
	ACE_Message_Block *pMsg = new ACE_Message_Block(nCtrlMsgLen);

	int nCmdCode = MSGTYPE_ACTIONRESULT_FROM_ACTIONTASK;
	// 类型
	memcpy(pMsg->wr_ptr(), &nCmdCode, 4);
	pMsg->wr_ptr(4);

	PutStrToMsg(szActionId,pMsg);
	PutStrToMsg(szResultDesc,pMsg);

	putq(pMsg);
	this->reactor()->notify(this, ACE_Event_Handler::WRITE_MASK); 
	return PK_SUCCESS;
}

// 手动触发(事件ID，事件名，优先级，事件参数) 发送至MainTask. EventParam为每个事件源不同。如报警产生为：tagalarmcname+subsys+..
int OnEvent(char *szEventId, char *szEventName, char *szPriority, char *szProduceTime,char *szEventParam)
{
	//根据事件ID获取动作ID
	// 每个字符串包含长度（4个字节）+内容（N个字节）
	int nCtrlMsgLen = 4 * 6 + strlen(szEventId) + strlen(szEventName) + strlen(szPriority) + strlen(szEventParam) + strlen(szProduceTime);
	ACE_Message_Block *pMsg = new ACE_Message_Block(nCtrlMsgLen);

	int nCmdCode = MSGTYPE_EVENTOCCURE_FROM_EVENT;
	// 类型
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


// 定时轮询的周期到了.json格式的日志信息，包括：level,time两个字段
// int CMainTask::handle_timeout(const ACE_Time_Value &current_time, const void *act)
// {
// 	// 每5秒读取一次是否打印日志的标记
// 	int nRet = PK_SUCCESS;
// 	time_t tmNow;
// 	time(&tmNow);
// 	return PK_SUCCESS;
// }

//解析接收到的event-alarm  发来的报警信息msg  和执行动作的返回,根据类型 cmdcode
int CMainTask::handle_output(ACE_HANDLE fd /*= ACE_INVALID_HANDLE*/ )
{
	ACE_Message_Block *pMsgBlk = NULL;
    ACE_Time_Value tvTimes = ACE_OS::gettimeofday();
	while(getq(pMsgBlk, &tvTimes) >= 0)
	{
		// 命令号
		int nCmdCode = 0;
		memcpy(&nCmdCode, pMsgBlk->rd_ptr(), 4);
		pMsgBlk->rd_ptr(4);
		//动作类型消息
		if(nCmdCode == MSGTYPE_ACTIONRESULT_FROM_ACTIONTASK)
		{
			// char *szActionId, int nResultCode, char *szResultDesc
			string strActionId, strResultDesc;
			int nResultCode = 0;
			GetStringFromMsg(pMsgBlk, strActionId);
			//memcpy(&nResultCode, pMsgBlk->rd_ptr(), sizeof(int));
			GetStringFromMsg(pMsgBlk, strResultDesc);

			// 找到对应的正在执行的动作
			map<string, LINK_ACTIONING_INFO*>::iterator itActioning = m_mapId2Actioning.find(strActionId);
			if(itActioning == m_mapId2Actioning.end())
            {
                pMsgBlk->release();
				g_logger.LogMessage(PK_LOGLEVEL_ERROR,"返回的动作不在执行中，出错");
				continue;
			}

			// 记录联动结果到历史事件
			LINK_ACTIONING_INFO pActioning = *(itActioning->second);
			pActioning.strDesc = strResultDesc;
			int nRet = SendActionInfoToEventServer(&pActioning);
			m_mapId2Actioning.erase(itActioning);
		}
		//事件
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
          

            //------------感觉逻辑有问题------------
			// 查找对应的事件的动作
			map<string, EVENT_ACTION_INFO *>::iterator itEventAction = m_mapId2Event.find(strEventId);
			if(itEventAction == m_mapId2Event.end())
            {
                pMsgBlk->release();
				g_logger.LogMessage(PK_LOGLEVEL_ERROR,"没有找到EventId = %s 的动作",strEventId.c_str());
				continue;
			}
			EVENT_ACTION_INFO *pEventAction = itEventAction->second;
			
			// 按顺序执行动作，并将动作信息记录下来
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
					// "writetag"//写一个点,"cyclewritetag"//写多个点,"poppage"//弹出界面
                  itType2Dll->second->PostAction((char*)pAction->strActionId.c_str(), (char*)pAction->strParam1.c_str(), (char*)pAction->strParam2.c_str(), (char*)pAction->strParam3.c_str(), (char*)pAction->strParam4.c_str(), (char*)strPriority.c_str(), (char*)strEventParam.c_str());
					g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "事件id:%s,事件名称:%s, 动作类型:%s, 动作参数:%s,参数2:%s", 
                      strEventId.c_str(), strEventName.c_str(), pActioning->strActionTypeName.c_str(), pAction->strParam1.c_str(), pAction->strParam2.c_str());
				}
				else
				{
					g_logger.LogMessage(PK_LOGLEVEL_ERROR, "事件id:%s,事件名称:%s, 根据动作类型:%s 未找到联动动作模块, 动作参数:%s,参数2:%s", 
                      strEventId.c_str(), strEventName.c_str(), pActioning->strActionTypeName.c_str(), pAction->strParam1.c_str(), pAction->strParam2.c_str());
				}
			} 		

            //------------感觉逻辑有问题------------
		}
		 
		pMsgBlk->release();
	} // while(getq(pMsgBlk, &tvTimes) >= 0)
	//根据类型找到动作action

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
	ACE_Time_Value	tvCheckConn;		// 扫描周期，单位ms
	tvCheckConn.msec(10 * 1000); 
	ACE_Time_Value tvStart = ACE_Time_Value::zero; 
	m_pReactor->schedule_timer((ACE_Event_Handler *)this, NULL, ACE_Time_Value::zero, tvCheckConn);

	// 从数据库加载事件和动作的关系信息
	LoadConfigFromDB();

	// 从本地文件搜索事件和动作dll并启动
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

bool compareLinkAction(LINK_ACTION_INFO * left, LINK_ACTION_INFO *right)
{
	// 从小到大排序
	return left->nExecuteOrder < right->nExecuteOrder;
}
//读取全部事件
long CMainTask::LoadConfigFromDB()
{
	// 查询指定的驱动列表
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
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "查询数据库失败,sql:%s, error:%s", szSql, strError.c_str());
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
    g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "查询数据库 %d !", vecEvents.size());
	return 0;
}


// / 从本地文件搜索事件和动作dll并启动
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
			strFileName = strFileName.substr(0, nPos); // 去除扩展名
		}

		if(strFileName.length() < EVENT_ACTION_DLL_PREFIX_LEN)
			continue;
		if(PKStringHelper::StriCmp(strFileExt.c_str(), "dll") != 0)
			continue;

		string strFilePrefix = strFileName.substr(0, EVENT_ACTION_DLL_PREFIX_LEN);
		if(PKStringHelper::StriCmp(strFilePrefix.c_str(), EVENT_DLLNAME_PREFIX) == 0) // 找到一个事件模块
		{
			string strEventName = strFileName.substr(EVENT_ACTION_DLL_PREFIX_LEN);
			
            int nPos = strEventName.find_last_of('.');
            if (nPos != std::string::npos)
			{
				strEventName = strEventName.substr(0, nPos); // 去除扩展名
			}

			LINK_EVENT_DLL *pEventDll = new LINK_EVENT_DLL();
			pEventDll->strEventName = strEventName;
			pEventDll->strEventDllName = strFileName;
			//获取事件DLL
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
				g_logger.LogMessage(PK_LOGLEVEL_ERROR,"事件DLL的导出函数获取失败;");
				return -100;
			}
			pEventDll->pfnInit(OnEvent);
			//hEventDll.close();
			m_vecEventDll.push_back(pEventDll);
		}
		else if(PKStringHelper::StriCmp(strFilePrefix.c_str(), ACTION_DLLNAME_PREFIX) == 0) // 找到一个动作模块
		{
			string strActionName = strFileName.substr(EVENT_ACTION_DLL_PREFIX_LEN);
            int nPos = strActionName.find_last_of('.');
            if (nPos != std::string::npos)
				strActionName = strActionName.substr(0, nPos); // 去除扩展名
			CActionTask *pActionTask = new CActionTask();
			pActionTask->SetActionInfo(strActionName.c_str(), strFileName.c_str());
			m_mapActionType2Task[strActionName] = pActionTask;
			pActionTask->Start();
		}
	}
	return 0;
}
//发送到logchannel
int CMainTask::SendActionInfoToEventServer(LINK_ACTIONING_INFO *linkActionInfo)
{
	string str2EventLog="";
	MSG2String(linkActionInfo,str2EventLog);
	//m_redisRW.PublishMsg(strEventLogChannel,str2EventLog);
	m_redisRW.publish(CHANNELNAME_EVENT, str2EventLog.c_str());
	m_redisRW.commit();
	g_logger.LogMessage(PK_LOGLEVEL_INFO,"联动服务发送联动事件成功,EventId = %d",linkActionInfo->nEventId);
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
	//产生
	root[EVENT_TYPE_MSGID] = EVENT_TYPE_LINK;
	root["username"]="admin";
	root["maincat"]=linkActionInfo->strActionTypeName; // t_link_action_type表的name字段。包括：writrtag，ZTAction，TimerAction等
	root["actionname"] = linkActionInfo->strActionName; // t_link_action表的name字段
	char szDateTime[PK_NAME_MAXLEN] = {0};
	string strCurTime = PKTimeHelper::GetCurTimeString(szDateTime, sizeof(szDateTime));
	root["rectime"] = strCurTime; //m_vecDesc[1]; 
	root["content"] = linkActionInfo->strDesc; //m_vecDesc[0];
	root["subsys"] = linkActionInfo->strSubsys; // 控制时传来的tag所属的子系统
	//if (linkActionInfo->nActionTypeId==0)
	//{
	//	root["opertype"]="产生报警";
	//}
	////确认
	//if (linkActionInfo->nActionTypeId==1)
	//{
	//	root["opertype"]="确认报警";
	//}
	////回复
	//if (linkActionInfo->nActionTypeId==2)
	//{
	//	root["opertype"]="恢复报警";
	//}
	//else
	//	root["opertype"]="其他";
	str2EventLog = root.toStyledString();
	return 0;
}
int CMainTask::CheckEventTimeout()
{
	return 0;
}
