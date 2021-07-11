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
#include <fstream> 

#include "python/Python.h"
#include "pythonerr.h"
extern CPKLog g_logger;
using namespace std;


#ifndef __sun
using std::map;
using std::string;
#endif

#define EVENT_ACTION_DLL_PREFIX_LEN		10		// pklinkact_ // pklinkevt_
#define EVENT_DLLNAME_PREFIX				"pklinkevt_"

#define MSGTYPE_ACTIONRESULT_FROM_ACTIONTASK			1		// �����̷߳��͹�������Ϣ���ͣ�����ִ�н����
#define MSGTYPE_EVENTOCCURE_FROM_EVENT				2		// �¼��̷߳��͹�������Ϣ���ͣ��¼���

#define NULLASSTRING(x) x==NULL?"":x

int GetStringFromMsg(ACE_Message_Block *pMsgBlk, string &strValue)
{
  // �����ַ�������
  int nStrLen = 0;
  memcpy(&nStrLen, pMsgBlk->rd_ptr(), 4);
  pMsgBlk->rd_ptr(4);

  // �����ַ���
  if (nStrLen > 0)
  {
    char *pValue = new char[nStrLen + 1]();
    memcpy(pValue, pMsgBlk->rd_ptr(), nStrLen);
    pMsgBlk->rd_ptr(nStrLen);

    strValue = std::string(pValue);
    delete[] pValue;
  }
  else
    strValue = "";

  return 0;
}
int PutStrToMsg(char *szStr, ACE_Message_Block *pMsg)
{
  // �ַ������� szActionId
  int nStrLen = strlen(szStr);
  memcpy(pMsg->wr_ptr(), &nStrLen, 4);
  pMsg->wr_ptr(4);
  // �ַ�������
  if (nStrLen > 0)
  {
    memcpy(pMsg->wr_ptr(), szStr, nStrLen);
    pMsg->wr_ptr(nStrLen);
  }
  return 0;
}

int OnEvent(char *szEventId, char *szEventName, char *szPriority, char *szProduceTime, char *szEventParam)
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
  if (m_pReactor)
  {
    delete m_pReactor;
    m_pReactor = NULL;
  }
}


long CMainTask::StartTask()
{
  long lRet = (long)activate(THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED, 1);
  if (lRet != PK_SUCCESS)
  {
    lRet = -2;
    g_logger.LogMessage(PK_LOGLEVEL_ERROR, _("CExecutionTask: StartTask() Failed, RetCode: %d"), lRet);
  }
  this->reactor()->register_handler(this, ACE_Event_Handler::READ_MASK | ACE_Event_Handler::WRITE_MASK);
  return PK_SUCCESS;
}

long CMainTask::StopTask()
{
  m_bStop = true;
  this->reactor()->end_reactor_event_loop();

  int nWaitResult = wait();

  return PK_SUCCESS;
}


char* szSql = new char[4 * 2048]();
//�������յ���event-alarm  �����ı�����Ϣmsg  ��ִ�ж����ķ���,�������� cmdcode
int CMainTask::handle_output(ACE_HANDLE fd /*= ACE_INVALID_HANDLE*/)
{
  //�������ݿ�ִ��
  int nTimeOut = ::atoi(strTimeout.c_str());
  if (nTimeOut <= 0)
    nTimeOut = 3;
  int nRet = m_dbAPI.SQLConnect(strConnStr.c_str(), strUserName.c_str(), strPassword.c_str(), m_dbAPI.ConvertDBTypeName2Id(strDbType.c_str()), nTimeOut, strCodingSet.c_str());
  if (nRet == 0)
    g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "�������ݿ�(%s,���Ӵ�:%s)�ɹ�", strDbType.c_str(), strConnStr.c_str());
  else
    g_logger.LogMessage(PK_LOGLEVEL_ERROR, "�������ݿ�(%s,���Ӵ�:%s)ʧ��, ����:%d", strDbType.c_str(), strConnStr.c_str(), nRet);

  ACE_Message_Block *pMsgBlk = NULL;
  ACE_Time_Value tvTimes = ACE_OS::gettimeofday();
  while (getq(pMsgBlk, &tvTimes) >= 0)
  {
    // �����
    int nCmdCode = 0;
    memcpy(&nCmdCode, pMsgBlk->rd_ptr(), 4);
    pMsgBlk->rd_ptr(4);

    if (nCmdCode == MSGTYPE_EVENTOCCURE_FROM_EVENT)
    {
      // char *szEventId, char *szEventName, char *szPriority, char *szEventParam;
      string strEventId, strEventName, strPriority, strProduceTime, strEventParam;
      GetStringFromMsg(pMsgBlk, strEventId);
      GetStringFromMsg(pMsgBlk, strEventName);
      GetStringFromMsg(pMsgBlk, strPriority);
      GetStringFromMsg(pMsgBlk, strProduceTime);
      GetStringFromMsg(pMsgBlk, strEventParam);

      Json::Reader jsonReader;
      Json::Value jsonEventParam;
      if (!jsonReader.parse(strEventParam.c_str(), jsonEventParam))
      {
        g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Parse Json Event Parameters <%s>.", strEventParam.c_str());
        pMsgBlk->release();
        continue;
      }
      //
      std::string strSubsys;
      Json::Value jsonSubsys;
      if (jsonEventParam.get(ALARM_SYSTEMNAME, jsonSubsys).isNull())
        strSubsys = "";
      else
        strSubsys = jsonEventParam[ALARM_SYSTEMNAME].asString();
      //
      std::string strAlarmValue;
      Json::Value jsonAlarmValue;
      if (jsonEventParam.get(ALARM_VALONPRODUCE, jsonAlarmValue).isNull())
        strAlarmValue = "";
      else
        strAlarmValue = jsonEventParam[ALARM_VALONPRODUCE].asString();

      //
      Py_Initialize();
      if (!Py_IsInitialized())
      {
        g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Py_IsInitialized failed.");
        pMsgBlk->release();
        continue;
      }
      PyRun_SimpleString("import sys");
      PyRun_SimpleString("import os");
      PyRun_SimpleString("import string");
      string strPyPath = string("sys.path.append('") + m_strPyLocation + string("')");
      PyRun_SimpleString(strPyPath.c_str());
      // ����ű�   
      PyObject* pModule = PyImport_ImportModule("pyalarm");// PyImport_Import(pName);
      if (!pModule)
      {
        g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Can't find pyalarm.py File.");
        continue;
      }

      //��ȡ���� 
      PyObject* pFunc = PyObject_GetAttrString(pModule, "OnAlarm"); //PyDict_GetItemString(pDict, "OnAlarm");
      if (!pFunc || !PyCallable_Check(pFunc))
      {
        g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Dictionary Func OnAlarm Error in pyalarm.py.");
        pMsgBlk->release();
        continue;
      }
      PyObject *pArgsT = PyTuple_New(1);

      //�������� 1.strProduceTime 2.strPriority 3.strSubsys 4.strAlarmName 5.strAlarmCName
      PyObject* pArgsD = PyDict_New();
      PyDict_SetItemString(pArgsD, "ProduceTime", Py_BuildValue("s", strProduceTime.c_str()));
      PyDict_SetItemString(pArgsD, "Priority", Py_BuildValue("s", strPriority.c_str()));
      PyDict_SetItemString(pArgsD, "Subsys", Py_BuildValue("s", strSubsys.c_str()));
      PyDict_SetItemString(pArgsD, "AlarmValue", Py_BuildValue("s", strAlarmValue.c_str())); 

      PyTuple_SetItem(pArgsT, 0, pArgsD);

      //���÷���
      PyObject *pReturn = PyEval_CallObject(pFunc, pArgsT);//PyObject_CallObject (pFunc, pArgs);
	  LogPythonError(); // ���Ӵ�����Ϣ��ӡ
	  if (pFunc)
		  Py_DECREF(pFunc);
	  if (pArgsT)
		  Py_DECREF(pArgsT);
      if (pReturn == NULL)
      {
        g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Call Func OnAlarm Return Error in pyalarm.py.");
        pMsgBlk->release();
        continue;
      }
	  if (pReturn)
		  Py_DECREF(pReturn);
      //��ȡSQL
      PyArg_Parse(pReturn, "s", &szSql);
      g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Call Func OnAlarm Success in pyalarm.py Return <%s>.", szSql);
      //close
      Py_Finalize();

      if (!m_dbAPI.IsConnected()) 
        nRet = m_dbAPI.SQLConnect(strConnStr.c_str(), strUserName.c_str(), strPassword.c_str(), m_dbAPI.ConvertDBTypeName2Id(strDbType.c_str()), nTimeOut, strCodingSet.c_str()); 
      if (nRet != 0)
        g_logger.LogMessage(PK_LOGLEVEL_ERROR, "�������ݿ�(%s,���Ӵ�:%s)ʧ��, �޷�ִ��SQL(%s), ����:%d", strDbType.c_str(), strConnStr.c_str(), szSql, nRet);
	  else
	  {
		  string strError;
		  int nRet = m_dbAPI.SQLExecute(szSql, &strError);
		  if (nRet != 0)
		  {

		  }
	  }
      // g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Pkpyalarm Broken TEST....");
    }

    pMsgBlk->release();
  } // while(getq(pMsgBlk, &tvTimes) >= 0)
  //���������ҵ�����action

  if (m_dbAPI.IsConnected())
    m_dbAPI.SQLDisconnect();

  return 0;
}

int CMainTask::svc()
{
  ACE_High_Res_Timer::global_scale_factor();
  this->reactor()->owner(ACE_OS::thr_self());

  int nRet = PK_SUCCESS;
  nRet = OnStart();

  char szTime[PK_HOSTNAMESTRING_MAXLEN] = { '\0' };
  PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));

  ACE_Time_Value tvSleep;
  tvSleep.msec(100); // 100ms

  while (!m_bStop)
  {
    this->reactor()->reset_reactor_event_loop();
    this->reactor()->run_reactor_event_loop();
  }

  g_logger.LogMessage(PK_LOGLEVEL_INFO, "CMainTask stopped");
  OnStop();

  return PK_SUCCESS;
}

int CMainTask::OnStart()
{
  m_pReactor->cancel_timer((ACE_Event_Handler *)this);
  ACE_Time_Value	tvCheckConn;		// ɨ�����ڣ���λms
  tvCheckConn.msec(10 * 1000);
  ACE_Time_Value tvStart = ACE_Time_Value::zero;
  m_pReactor->schedule_timer((ACE_Event_Handler *)this, NULL, ACE_Time_Value::zero, tvCheckConn);

  LoadEventDll();
  //LoadConfigFromDB();
  LoadDBFile();

  LoadPyFile();

  return 0;
}

int CMainTask::LoadPyFile()
{
  m_strPyLocation = std::string(PKComm::GetConfigPath());
  return 0;
}

inline string& ltrim(string &str) {
  string::iterator p = find_if(str.begin(), str.end(), not1(ptr_fun<int, int>(isspace)));
  str.erase(str.begin(), p);
  return str;
}

inline string& rtrim(string &str) {
  string::reverse_iterator p = find_if(str.rbegin(), str.rend(), not1(ptr_fun<int, int>(isspace)));
  str.erase(p.base(), str.end());
  return str;
}

inline string& trim(string &str) {
  ltrim(rtrim(str));
  return str;
}

int CMainTask::LoadDBFile()
{
  // �õ������ļ���·��
  const char *szConfigPath = PKComm::GetConfigPath();
  string strConfigPath = szConfigPath;
  strConfigPath = strConfigPath + "\\db2.conf";

  std::ifstream textFile(strConfigPath.c_str());
  if (!textFile.is_open())  // �����ڸ��ļ�������Ϊ��sqlite��eview.db
  {
    g_logger.LogMessage(PK_LOGLEVEL_ERROR, "db2.conf file:%s not exist", strConfigPath.c_str());
    return  -1;
  }

  // �����ļ���С  
  textFile.seekg(0, std::ios::end);
  std::streampos len = textFile.tellg();
  textFile.seekg(0, std::ios::beg);

  std::string strConfigData((std::istreambuf_iterator<char>(textFile)), std::istreambuf_iterator<char>());

  textFile.close();

  strConnStr = "";
  strUserName = "";
  strPassword = "";
  strDbType = "";
  strTimeout = "";
  strCodingSet = "";

  while (strConfigData.length() > 0)
  {
    strConfigData = trim(strConfigData);
    if (strConfigData.empty())
      break;

    string strLine = "";
    int nPosLine = strConfigData.find('\n');
    if (nPosLine >= 0)
    {
      strLine = strConfigData.substr(0, nPosLine);
      strConfigData = strConfigData.substr(nPosLine + 1);
    }
    else
    {
      strLine = strConfigData;
      strConfigData = "";
    }

    int nPosSection = strLine.find('#');
    if (nPosSection == 0) // #�ſ�ͷ
    {
      continue;
    }

    // x=y
    nPosSection = strLine.find('=');
    if (nPosSection <= 0) // ��=�Ż�=�ſ�ͷ
    {
      g_logger.LogMessage(PK_LOGLEVEL_ERROR, "db2.conf file:%s, line: %s invalid, need =!", strConfigPath.c_str(), strLine.c_str());
      continue;
    }

    string strSecName = strLine.substr(0, nPosSection);
    string strSecValue = strLine.substr(nPosSection + 1);
    strSecName = trim(strSecName);
    strSecValue = trim(strSecValue);
    std::transform(strSecName.begin(), strSecName.end(), strSecName.begin(), ::tolower);
    if (strSecName.compare("dbtype") == 0)
      strDbType = strSecValue;
    else if (strSecName.compare("connection") == 0)
      strConnStr = strSecValue;
    else if (strSecName.compare("username") == 0)
      strUserName = strSecValue;
    else if (strSecName.compare("password") == 0)
      strPassword = strSecValue;
    else if (strSecName.compare("timeout") == 0)
      strTimeout = strSecValue;
    else if (strSecName.compare("codingset") == 0)
      strCodingSet = strSecValue;
    else
    {
      //g_logger.LogMessage(PK_LOGLEVEL_ERROR, "db config file:%s, key:%s unknown!", strConfigPath.c_str(), strSecName.c_str());
      continue;
    }
  }
  // MYSQL ��Ȼ��UTF8������������ΪGB2312���ܶ�ȡ����
  if (strCodingSet.empty())
    strCodingSet = "utf8";

  int nDatabaseTypeId = m_dbAPI.ConvertDBTypeName2Id(strDbType.c_str());
  if (nDatabaseTypeId == -1)
  {
    g_logger.LogMessage(PK_LOGLEVEL_ERROR, "db2.conf file:%s, ��֧�ֵ����ݿ�����:%s!", strConfigPath.c_str(), strDbType.c_str());
    return -1;
  }


  return 0;
}

int CMainTask::OnStop()
{
  vector<LINK_EVENT_DLL *>::iterator itLinkEvtDll = m_vecEventDll.begin();
  for (; itLinkEvtDll != m_vecEventDll.end(); itLinkEvtDll++)
  {
    (*itLinkEvtDll)->dllEvent.close();
    (*itLinkEvtDll)->pfnExit = NULL;
    (*itLinkEvtDll)->pfnInit = NULL;
    (*itLinkEvtDll)->strEventDllName = "";
    (*itLinkEvtDll)->strEventName = "";
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

  return PK_SUCCESS;
}

//��ȡȫ���¼�
long CMainTask::LoadConfigFromDB()
{
  // ��ѯָ���������б�
  long lQueryId = -1;
  vector<vector<string> > vecEvents;
  CPKEviewDbApi PKEviewDbApi;
  int nRet = PKEviewDbApi.Init();
  char szSql[512] = { 0 };
  sprintf(szSql, "select A.id as id, A.actiontype_id as actiontypeid,A.name as actionname,B.name as actiontypename, event_id,param1,param2,param3,param4,description,failtip from t_link_action A,t_link_action_type B where A.actiontype_id=B.id and A.enable=1");
  int nDrvNum = 0;
  string strError;
  nRet = PKEviewDbApi.SQLExecute(szSql, vecEvents, &strError);
  if (nRet != 0)
  {
    PKEviewDbApi.Exit();
	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "��ѯ���ݿ�ʧ��:%s, error:%s", szSql, strError.c_str());
    return -2;
  }

  for (int iEvent = 0; iEvent < vecEvents.size(); iEvent++)
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

    if (strEventId.empty())
    {
      g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Event Action EventID Is NULL, Action Name<%s>.", strActionName.c_str());
      continue;
    }
  }

  PKEviewDbApi.Exit();
  g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "��ѯ���ݿ� %d !", vecEvents.size());
  return 0;
}

long CMainTask::LoadEventDll()
{
  string strBinPath = PKComm::GetBinPath();
  list<string> listFileNames;
  PKFileHelper::ListFilesInDir(strBinPath.c_str(), listFileNames);
  for (list<string>::iterator itFile = listFileNames.begin(); itFile != listFileNames.end(); itFile++)
  {
    string strFileName = *itFile;
    string strFileExt = "";

    int nPos = strFileName.find_last_of('.');
    if (nPos != std::string::npos)
    {
      strFileExt = strFileName.substr(nPos + 1);
      strFileName = strFileName.substr(0, nPos); // ȥ����չ��
    }

    if (strFileName.length() < EVENT_ACTION_DLL_PREFIX_LEN)
      continue;
    if (PKStringHelper::StriCmp(strFileExt.c_str(), "dll") != 0)
      continue;

    string strFilePrefix = strFileName.substr(0, EVENT_ACTION_DLL_PREFIX_LEN);
    if (PKStringHelper::StriCmp(strFilePrefix.c_str(), EVENT_DLLNAME_PREFIX) == 0) // �ҵ�һ���¼�ģ��
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
      string strFullEventDllPath = strBinPath + "\\" + (*itFile);
      long nErr = pEventDll->dllEvent.open(strFullEventDllPath.c_str());
      if (nErr != 0)
      {
        g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Load %s failed!\n", strFullEventDllPath.c_str());
        return nErr;
      }
      g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "Load %s success!\n", strFullEventDllPath.c_str());

      pEventDll->pfnInit = (PFN_InitEvent)pEventDll->dllEvent.symbol(PFNNAME_INITEVENT);
      pEventDll->pfnExit = (PFN_ExitEvent)pEventDll->dllEvent.symbol(PFNNAME_EXITEVENT);
      if ((pEventDll->pfnExit == NULL) || (pEventDll->pfnExit == NULL))
      {
        g_logger.LogMessage(PK_LOGLEVEL_ERROR, "�¼�DLL�ĵ���������ȡʧ��;");
        return -100;
      }
      pEventDll->pfnInit(OnEvent);
      //hEventDll.close();
      m_vecEventDll.push_back(pEventDll);
    }
  }
  return 0;
}
