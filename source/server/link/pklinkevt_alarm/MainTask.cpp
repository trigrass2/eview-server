/**************************************************************
 *  Filename:    ReqFetchTask.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: implementation of the CReqFetchTask class.
 *
 *  @author:     shijunpu
 *  @version     05/29/2008  shijunpu    Initial Version
 *  @version     02/18/2009  wangjian    icv server first release
 **************************************************************/
#include "json/json.h"
#include "errcode/error_code.h"
#include "MainTask.h"
#include "pkeviewdbapi/pkeviewdbapi.h"
#include "pklog/pklog.h"
#include "common/eviewdefine.h"
#include "pkcomm/pkcomm.h"
#include "../../pknodeserver/RedisFieldDefine.h"
extern CPKLog g_logger;

#define NULLASSTRING(x) x==NULL?"":x
#define DEFAULT_RECVREQUEST_TIMEOUT		200
#define _(x)	x

#define EVENT_TYPE_NAME_ALARM_PRODUCE		"alarm-produce"		// ������һ�鱨���������¼�����

#define EVENT_TYPE_NAME_ALARM_PRODUCE_PS		"alarm-produce-bypriority-subsys"		// ���ȼ���ϵͳ��Χ�����¼�����
#define EVENT_TYPE_NAME_ALARM_PRODUCE_PS_SIGN		-0xFF		//SMS ���¼���־

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

extern PFN_LinkEventCallback g_pfnLinkEvent;
extern int PushStringToBuffer(char *&pSendBuf, int &nLeftBufLen, const char *szStr, int nStrLen);
extern int PushInt32ToBuffer(char *&pBufer, int &nLeftBufLen, int nValue);
extern int PopInt32FromBuffer(char *&pBufer, int &nLeftBufLen, int &nValue);
extern int PopStringFromBuffer(char *&pBufer, int &nLeftBufLen, string &strValue);

// ����Ϣ�������̡߳� ����������һ���߳���ִ�е�!
/*
void OnChannel_AlarmMsg(const char *szChannelName, const char *szMessage, void *pCallbackParam) // ���͵Ļص�����������Ҫ��������ʱ�������м���Ҫ���ؽ��ȣ���Ҫ�趨ÿ�����ĳ���
{
CMainTask *pMainTask = (CMainTask *)pCallbackParam;
int nMsgLen = sizeof(int) + sizeof(int) * 2 + strlen(szChannelName) + strlen(szMessage);
ACE_Message_Block *pMsgBlk = new ACE_Message_Block(nMsgLen);
char *pSendBuf = pMsgBlk->wr_ptr();
int nLeftBufLen = nMsgLen;
char *pBufBegin = pMsgBlk->wr_ptr();
int nCmdNo = 0;
PushInt32ToBuffer(pSendBuf, nLeftBufLen, nCmdNo);
PushStringToBuffer(pSendBuf, nLeftBufLen, szChannelName, strlen(szChannelName));
PushStringToBuffer(pSendBuf, nLeftBufLen, szMessage, strlen(szMessage));
pMsgBlk->wr_ptr(pSendBuf - pBufBegin);
pMainTask->putq(pMsgBlk);

g_logger.LogMessage(PK_LOGLEVEL_INFO, "OnChannel_AlarmMsg recv an alarm msg,channel:%s,message:%s",szChannelName, szMessage);
}
*/

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


CMainTask::CMainTask()
{
  m_bShouldExit = false;
}

CMainTask::~CMainTask()
{
  ClearMap();
}

void CMainTask::ClearMap()
{
  m_mapSingleAlmAction2Event.clear();
}

int CMainTask::ReadConfigFromDB()
{
  ClearMap();

  long lQueryId = -1;
  vector<vector<string> > vecEvents;
  CPKEviewDbApi PKEviewDbApi;
  int nRet = PKEviewDbApi.Init();
  if (nRet != 0)
  {
    g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "��ʼ�����ݿ�ʧ��!");
    PKEviewDbApi.Exit();
    return -1;
  }

  char szSql[512] = { 0 };
  sprintf(szSql, "select a.id as id,a.name as name,evtsource,b.name as eventtypename,a.description as description from t_link_event_list a,t_link_event_type b \
                 				  				  				  where a.eventtype_id=b.id and b.name='%s' and enable=1", EVENT_TYPE_NAME_ALARM_PRODUCE);
  int nDrvNum = 0;
  string strError;
  nRet = PKEviewDbApi.SQLExecute(szSql, vecEvents, &strError);
  if (nRet != 0)
  {
    PKEviewDbApi.Exit();
    g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "��ѯ���ݿ�ʧ��:%s, error:%s", szSql, strError.c_str());
    return -2;
  }

  for (int iEvent = 0; iEvent < vecEvents.size(); iEvent++)
  {
    vector<string> &row = vecEvents[iEvent];
    int iCol = 0;
    string strId = NULLASSTRING(row[iCol].c_str());
    iCol++;
    string strName = NULLASSTRING(row[iCol].c_str());
    iCol++;
    string strEventSrc = NULLASSTRING(row[iCol].c_str());
    iCol++;
    string strEvtTypeName = NULLASSTRING(row[iCol].c_str());
    iCol++;
    string strDescription = NULLASSTRING(row[iCol].c_str());

    if (strEventSrc.empty())
    {
      g_logger.LogMessage(PK_LOGLEVEL_ERROR, "�����¼�:%s��Tag������Ϊ��!", strName.c_str());
      continue;
    }
    vector<string> vecEventSrc = PKStringHelper::StriSplit(strEventSrc, ";"); // tag1.HH,tag2.HH;2

    string strTagAlarms = vecEventSrc[0];
    if (strTagAlarms.length() <= 0)
    {
      g_logger.LogMessage(PK_LOGLEVEL_ERROR, "�¼�δ�����¼�������Դ��id:%s,name:%s,eventtypename:%s", strId.c_str(), strName.c_str(), strEvtTypeName.c_str());
      continue;
    }

    int nAlarmingCountAsEvent = 1;
    if (vecEventSrc.size() > 1)
    {
      string strAlarmingCount = vecEventSrc[1];
      if (!strAlarmingCount.empty())
        nAlarmingCountAsEvent = ::atoi(strAlarmingCount.c_str());
    }
    if (nAlarmingCountAsEvent <= 0)
      nAlarmingCountAsEvent = 1;

    vector<string> vecTagAlarm = PKStringHelper::StriSplit(strTagAlarms, ","); // tag1.HH or tag2.HH
    if (vecTagAlarm.size() <= 0)
    {
      g_logger.LogMessage(PK_LOGLEVEL_ERROR, "�¼�δ�����¼�������Դ2��id:%s,name:%s,eventtypename:%s", strId.c_str(), strName.c_str(), strEvtTypeName.c_str());
      continue;
    }

    LINK_EVENT *pEvent = new LINK_EVENT();
    pEvent->strId = strId;
    pEvent->strName = strName;
    pEvent->strSource = strEventSrc;
    pEvent->strDescription = strDescription;
    pEvent->strEventTypeName = strEvtTypeName;
    pEvent->nAlarmingCountAsEvent = nAlarmingCountAsEvent;

    //if (strEvtTypeName.compare(EVENT_TYPE_NAME_ALARM_PRODUCE) == 0)
    if (nAlarmingCountAsEvent == 1) // ֻҪ��һ����������Ϊ�����˱���
    {
      for (vector<string>::iterator itTagName = vecTagAlarm.begin(); itTagName != vecTagAlarm.end(); itTagName++)
      {
        string strEventIdentifier = *itTagName;
        strEventIdentifier = strEventIdentifier + "-" + strEvtTypeName; // Tag1.HH-alarm-produce
        //���ݿ��еı������ö�ȡ���ڴ���
        m_mapSingleAlmAction2Event[strEventIdentifier] = pEvent;
        pEvent->mapAlarmName2Status[strEventIdentifier] = false;
      }
    }
    else //EVENT_TYPE_NAME_MULTIPLE_ALARM_PRODUCE
    {
      for (vector<string>::iterator itTagName = vecTagAlarm.begin(); itTagName != vecTagAlarm.end(); itTagName++)
      {
        string strAlarmName = *itTagName; // tag1.HH
        pEvent->mapAlarmName2Status[strAlarmName] = false; // �Ƿ����ڱ���
      }
      m_vecMultiAlmEvent.push_back(pEvent);
      g_logger.LogMessage(PK_LOGLEVEL_INFO, "PKLinkEvtAlarm,��ȡ���鱨��:%s,ͬʱ������������:%d", strTagAlarms.c_str(), nAlarmingCountAsEvent);
    }
  }

  g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "PKLinkEvtAlarm,��ȡ���������������¼�:%d��������,�鱨�������¼�(���㴥������>=2)��%d����", m_mapSingleAlmAction2Event.size(), m_vecMultiAlmEvent.size());

  //---------------alarm-produce-bypriority-subsys-------------
  char szSqlSMS[1024] = { 0 };
  sprintf(szSqlSMS, "select a.id as id,a.name as name,evtsource,b.name as eventtypename,a.description as description from t_link_event_list a,t_link_event_type b \
                                           where a.eventtype_id=b.id and b.name='%s' and enable=1", EVENT_TYPE_NAME_ALARM_PRODUCE_PS);

  vector<vector<string> > vecRows;
  nRet = PKEviewDbApi.SQLExecute(szSqlSMS, vecRows, &strError);
  if (nRet != 0)
  {
	  g_logger.LogMessage(PK_LOGLEVEL_ERROR, "PKLinkEvtAlarm,��ѯ���ݿ�ʧ��,sql:%s,error:%s", szSqlSMS, strError.c_str());
  }
  if (vecRows.size() > 0)
  {
	  for (int iEvent = 0; iEvent < vecRows.size(); iEvent++)
    {
		vector<string> &row = vecRows[iEvent];
      int iCol = 0;
      string strId = NULLASSTRING(row[iCol].c_str());
      iCol++;
      string strName = NULLASSTRING(row[iCol].c_str());
      iCol++;
      string strEventSrc = NULLASSTRING(row[iCol].c_str());
      iCol++;
      string strEvtTypeName = NULLASSTRING(row[iCol].c_str());
      iCol++;
      string strDescription = NULLASSTRING(row[iCol].c_str());

      if (strEventSrc.empty() || strName.empty())
      {
        g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Event Sorce Is NULL, ID<%s>.", strId.c_str());
        continue;
      }
      //vector<string> vecEventSrc = PKStringHelper::StriSplit(strEventSrc, ";");
	  row.clear();

      LINK_EVENT *pEvent = new LINK_EVENT();
      pEvent->strId = strId;
      pEvent->strName = strName;
      pEvent->strSource = strEventSrc;
      pEvent->strDescription = strDescription;
      pEvent->strEventTypeName = strEvtTypeName;
      pEvent->nAlarmingCountAsEvent = 1;// EVENT_TYPE_NAME_ALARM_PRODUCE_PS_SIGN;//SMS ���¼���־

      m_vecMultiEvent.push_back(pEvent);//SMS ���¼�
    }
  }
  g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "PKLinkEvtAlarm,Read Events %d.", vecRows.size());
  vecRows.clear();
  //---------------SMS-------------

  PKEviewDbApi.Exit();
  return 0;
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
int CMainTask::StartTask()
{
  m_bShouldExit = false;
  long lRet = PK_SUCCESS;
  ReadConfigFromDB();
  int nRet = (long)this->activate();
  if (nRet != 0)
  {
    lRet = EC_ICV_CRS_FAILTOSTARTTASK;
    g_logger.LogMessage(PK_LOGLEVEL_ERROR, _("StartTask() Failed, RetCode: %d"), lRet);
  }

  return lRet;
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
int CMainTask::StopTask()
{
  m_bShouldExit = true;

  return ICV_SUCCESS;
}

/**
 *  main procedure of the task.
 *	get a msg from CVNDK and put it to Request Handler Queue
 *
 *  @return
 *	- ==0 success
 *	- !=0 failure
 *
 *  @version     11/18  shijunpu  Initial Version.
 */
int CMainTask::svc()
{
  g_logger.LogMessage(PK_LOGLEVEL_DEBUG, _("CMainTask::svc() begin"));

  int nPKMemDBListenPort = PKMEMDB_LISTEN_PORT;
  string strPKMemDBPassword = PKMEMDB_ACCESS_PASSWORD;
  GetPKMemDBPort(nPKMemDBListenPort, strPKMemDBPassword);

  int nRet = m_memDBSubSync.initialize(PK_LOCALHOST_IP, nPKMemDBListenPort, strPKMemDBPassword.c_str());
  if (nRet != 0)
  {
    g_logger.LogMessage(PK_LOGLEVEL_ERROR, _("CMainTask::StartTask redis initialize failed!"));
  }
  m_memDBSubSync.subscribe(CHANNELNAME_ALARMING);// , OnChannel_AlarmMsg, this);

  ACE_Time_Value tvLastPrintLog = ACE_OS::gettimeofday();
  while (!m_bShouldExit)
  {
    string strChannel;
    string strMessage;
    nRet = m_memDBSubSync.receivemessage(strChannel, strMessage, 500); // 500 ms
    // ����������״̬
    if (nRet != 0)
    {
		PKComm::Sleep(200);
      continue;
    }

    OnRecvMessage(strMessage);
    //g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "PKLinkEvt_Alarm recv an alarm msg,message:%s from %s channel", strMessage.c_str(), CHANNELNAME_ALARMING);
  }

  m_memDBSubSync.finalize();
  g_logger.LogMessage(PK_LOGLEVEL_DEBUG, _("CMainTask::svc() end"));

  return ICV_SUCCESS;
}

int CMainTask::OnRecvMessage(string &strMessage)
{
  Json::Reader reader;
  Json::Value root;
  if (!reader.parse(strMessage, root, false))
  {
    g_logger.LogMessage(PK_LOGLEVEL_ERROR, "PKLinkEvt_Alm, recv an msg(),����json��ʽ invalid!", strMessage.c_str());
    return -1;
  }

  if (!root.isObject() && !root.isArray())
  {
    g_logger.LogMessage(PK_LOGLEVEL_ERROR, "PKLinkEvt_Alm, recv an msg(), not an object or array!����:%s", strMessage.c_str());
    return -1;
  }

  g_logger.LogMessage(PK_LOGLEVEL_INFO, "PKLinkEvt_Alm Envent Messages<%s>.", strMessage.c_str());

  vector<Json::Value *> vectorMsg;
  if (root.isObject())
  {
    vectorMsg.push_back(&root);
  }
  else if (root.isArray())
  {
    for (unsigned int i = 0; i < root.size(); i++)
      vectorMsg.push_back(&root[i]);
  }

  // �����յ�����Ϣ
  for (vector<Json::Value *>::iterator itCmd = vectorMsg.begin(); itCmd != vectorMsg.end(); itCmd++)
  {
    Json::Value &jsonMsg = **itCmd;
	Json::Value jsonObjName = jsonMsg[ALARM_OBJECT_NAME];//�������������
	Json::Value jsonPropName = jsonMsg[ALARM_PROP_NAME];//��������������
	Json::Value jsonType = jsonMsg[ALARM_TYPE];
	Json::Value jsonAlarDesc = jsonMsg[ALARM_DESC];				//����������

	Json::Value jsonActType = jsonMsg[ALARM_ACTION_TYPE];
    Json::Value jsonJudgeMethod = jsonMsg[ALARM_JUDGEMUTHOD];	//�������ͣ�HH
    Json::Value jsonLevel = jsonMsg[ALARM_LEVEL];//Ȩ�޵ȼ�
    Json::Value jsonSubsys = jsonMsg[ALARM_SYSTEMNAME];//��ϵͳ
    Json::Value jsonProduceTime = jsonMsg[ALARM_PRODUCETIME];//��������ʱ��

	if (jsonActType.isNull() || jsonObjName.isNull() || jsonJudgeMethod.isNull())
    {
      g_logger.LogMessage(PK_LOGLEVEL_ERROR, "PKLinkEvt_Alm, �յ�����,��ʽ���󣬲������жϷ�����������ƣ�����;%s", strMessage.c_str());
      return -3;
    }

    string strActType = jsonActType.asString();
	string strObjectName = jsonObjName.asString(); // Tag1.HH
	string strPropName = jsonPropName.asString(); // Tag1.HH
	string strAlarmType = jsonType.asString(); // Tag1.HH
	string strDesc = jsonAlarDesc.asString(); // Tag1.HH

    string strSubsys = jsonSubsys.asString();
    string strProduceTime = jsonProduceTime.asString();
    string strLevel = jsonLevel.asString();
    string strEventParam = jsonMsg.toStyledString();

	string strAlarmName = strObjectName + "." + strPropName + "." + strAlarmType;
    // �ж��¼��������ֶ���ʱ���Ƿ��е������������������߼�
    if (strActType.compare(ALARM_PRODUCE) == 0) // ���������Ĵ��������ǽ����������������������
    {
		string strKey = strAlarmName + "-" + EVENT_TYPE_NAME_ALARM_PRODUCE; // objname.propname.judgemethod.HH-alarm-recover

      map<string, LINK_EVENT *>::iterator itMap = m_mapSingleAlmAction2Event.find(strKey); // Tag1.HH-recover
      if (itMap != m_mapSingleAlmAction2Event.end())//�ҵ��¼�
      {
        g_logger.LogMessage(PK_LOGLEVEL_INFO, "PKLinkEvt_Alm, ---�յ����������������¼�%s", strKey.c_str());
        LINK_EVENT *pEvent = itMap->second;
        pEvent->strSubsys = strSubsys;
        // (int nEventId, char *szEventName, int nPriority, char *szEventParam)
        int nRet = g_pfnLinkEvent((char *)pEvent->strId.c_str(), (char *)pEvent->strName.c_str(), (char *)strLevel.c_str(), (char*)strProduceTime.c_str(), (char*)strEventParam.c_str());
        g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "������һ�������¼�,�¼�id:%s,����:%s,����ֵ:%d",
          pEvent->strId.c_str(), pEvent->strName.c_str(), nRet);
      }
    }

    // ��������������������
    if (strActType.compare(ALARM_PRODUCE) == 0 || strActType.compare(ALARM_RECOVER) == 0)
    {
      // �������ͬʱ�����Ž��������ı���
      for (vector<LINK_EVENT *>::iterator it = m_vecMultiAlmEvent.begin(); it != m_vecMultiAlmEvent.end(); it++)
      {
        LINK_EVENT *pEvent = *it;
        map<string, bool>::iterator itAlarm = pEvent->mapAlarmName2Status.find(strAlarmName);
        if (itAlarm == pEvent->mapAlarmName2Status.end())
          continue;

        int nPreAlarmingCount = pEvent->GetAlarmingCount();
        bool bPreEventTriggered = pEvent->IsEventTrigged();
        g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "�յ�һ��������ı���%s��%s����,��Ӧ���¼�id:%s,����:%s,֮ǰ�������ڱ����ĸ���:%d,������������:%d",
          strAlarmName.c_str(), strActType.c_str(), pEvent->strId.c_str(), pEvent->strName.c_str(), nPreAlarmingCount, pEvent->nAlarmingCountAsEvent);

        if (strActType.compare(ALARM_PRODUCE) == 0)
        {
          pEvent->mapAlarmName2Status[strAlarmName] = true; // ����������
          int nNowAlarmingCount = pEvent->GetAlarmingCount();
          bool bNowEventTriggered = pEvent->IsEventTrigged();
          if (!bPreEventTriggered && bNowEventTriggered) // ��ǰû�������ڴﵽ���������ˣ�
          {
            int nRet = g_pfnLinkEvent((char *)pEvent->strId.c_str(), (char *)pEvent->strName.c_str(), (char *)strLevel.c_str(), (char*)strProduceTime.c_str(), (char*)strEventParam.c_str());
            g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "������һ���������¼�,�¼�id:%s,����:%s,��ǰ�����ڱ����ĸ���:%d,������������:%d,����ֵ:%d",
              pEvent->strId.c_str(), pEvent->strName.c_str(), nNowAlarmingCount, pEvent->nAlarmingCountAsEvent, nRet);
          }
        }
        else
        {
          pEvent->mapAlarmName2Status[strAlarmName] = false; // �����ָ���
          int nNowAlarmingCount = pEvent->GetAlarmingCount();
        }
      }
    }

    //��ʱ�Ȳ�������, ֻҪ�ȼ�,��ϵͳ��������
    //------------SMS------------
    if (std::string::npos != strActType.find(ALARM_PRODUCE) || std::string::npos != strActType.find("priority"))
    {
      for (vector<LINK_EVENT *>::iterator item = m_vecMultiEvent.begin(); item < m_vecMultiEvent.end(); item++)
      {
        std::string strEventSource = (*item)->strSource;
        char szSubsystems[512] = { 0 };
        int nSMSPriorityStart = 0, nSMSPriorityEnd = 0;
        sscanf(strEventSource.c_str(), "PriorityStart=%d,PriorityEnd=%d,SubSys=%s", &nSMSPriorityStart, &nSMSPriorityEnd, szSubsystems);

        int nLevel = std::atoi(strLevel.c_str());
        if (nLevel >= nSMSPriorityStart && nLevel < nSMSPriorityEnd)
        {
          if (std::string(szSubsystems).size() > 0)
          {
            if (std::string(szSubsystems).find(strSubsys) == std::string::npos)
            {
              g_logger.LogMessage(PK_LOGLEVEL_WARN, "Sms Event Id:<%s>,Name:<%s>,Level:<%s>,Subsys:<%s> Mismatched.",
                (*item)->strId.c_str(), (*item)->strName.c_str(), strLevel.c_str(), strSubsys.c_str() );
              return -1;
            }
          }
          
          int nRet = g_pfnLinkEvent((char*)(*item)->strId.c_str(), (char*)(*item)->strName.c_str(), (char *)strLevel.c_str(), (char*)strProduceTime.c_str(), (char*)strEventParam.c_str());
          g_logger.LogMessage(PK_LOGLEVEL_WARN, "Sms Event Id:<%s>,Name:<%s>,Level:<%s>,Subsys:<%s> Triggered.",
            (*item)->strId.c_str(), (*item)->strName.c_str(), strLevel.c_str(), strSubsys.c_str());
        }
        else
          g_logger.LogMessage(PK_LOGLEVEL_WARN, "Sms Event Id:<%s>,Name:<%s>,Level:<%s>,Subsys:<%s> Not Triggered.",
          (*item)->strId.c_str(), (*item)->strName.c_str(), strLevel.c_str(), strSubsys.c_str() );
      }
    }
    //------------SMS------------
  }

  return 0;
}