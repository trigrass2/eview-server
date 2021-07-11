/**************************************************************
 *  Filename:    CtrlProcessTask.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: �����������.
 *
 *  @author:    shijunpu
 *  @version    05/28/2008  ȷ�Ϻ�ɾ������Ҳ���ڿ����������ִ�С�����Ҳ�Ƕ������ӣ�����ת�������ݴ��������
**************************************************************/
#include "common/OS.h"
#include <ace/Default_Constants.h>
#include "ace/Thread_Mutex.h"
#include "ace/Guard_T.h"
#include "math.h"
#include "CtrlProcTask.h"
#include "pkcomm/pkcomm.h"
#include "pkmemdbapi/pkmemdbapi.h"
#include "json/json.h"
#include "common/RMAPI.h"
#include "common/eviewdefine.h"
#include "pklog/pklog.h"
#include "PKNodeServer.h"
#include "DataProcTask.h"
#include "boost/regex.hpp"
#include "MainTask.h"
#include "TagProcessTask.h"
#include "UserAuthTask.h"

extern CPKNodeServer* g_pPKNodeServer;
extern CProcessQueue * g_pQueData2NodeSrv;
extern CPKLog g_logger;
int GetPKMemDBPort(int &nListenPort, string &strPassword);

#define	DEFAULT_SLEEP_TIME		100	

// ���json��object��ôasString���쳣
// ���json��string��toStyledString����˫���ţ��ʶ���Ҫд������ʵ��
int Json2String(Json::Value &json, string &strJson)
{
	strJson = "";
	if (json.isNull())
	{
		strJson = "";
	}
	else if (json.isString())
	{
		strJson = json.asString();
		return 0;
	}
	else if (json.isDouble())
	{
		char szTmp[32] = { 0 };
		sprintf(szTmp, "%f", json.asDouble());
		strJson = szTmp;
	}
	else if (json.isInt() || json.isNumeric())
	{
		char szTmp[32] = { 0 };
		sprintf(szTmp, "%d", json.asInt());
		strJson = szTmp;
	}
	else if (json.isUInt())
	{
		char szTmp[32] = { 0 };
		sprintf(szTmp, "%d", json.asUInt());
		strJson = szTmp;
	}
	else if (json.isBool())
	{
		char szTmp[32] = { 0 };
		sprintf(szTmp, "%d", json.asBool() ? 1 : 0);
		strJson = szTmp;
	}
	else if (json.isObject() || json.isArray())
	{
		strJson = CONTROLPROCESS_TASK->m_jsonWriter.write(json); // ���ܸ���
		// strJson = json.toStyledString();
	}

	return 0;
}

// ����Ϣ�������̡߳� ����������һ���߳���ִ�е�!
/*
void OnChannel_ControlMsg(const char *szChannelName, const char *szMessage, void *pCallbackParam) // ���͵Ļص�����������Ҫ��������ʱ�������м���Ҫ���ؽ��ȣ���Ҫ�趨ÿ�����ĳ���
{
	CCtrlProcTask *pControlProcess = (CCtrlProcTask *)pCallbackParam;
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "CtrlProcTask recv an controls msg:%s",szMessage);

	Json::Reader reader;
	Json::Value root;
	if (!reader.parse(szMessage, root, false))
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "CtrlProcTask recv an msg(%s), invalid!",szMessage);
		// g_pPbServer->UpdateServerStatus(-2,"control msg format invalid");
		return;
	}

	if (!root.isObject() && !root.isArray())
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "CtrlProcTask recv an msg(%s), not an object or array!",szMessage);
		// g_pPbServer->UpdateServerStatus(-2,"control msg format invalid");
		return;
	}

	// ����tag���ƣ��ҵ���Ӧ��������Ӧ�Ķ���ͨ�������������������� ��ȷ�ϱ����Ϳ��ƶ������ﴦ���ˡ�
	pControlProcess->RelayControlCmd2Driver(szMessage, root);
}
*/


/**
 *  ���캯��.
 *
 *  @param  -[in,out]  CDriver*  pDriver: [comment]
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
CCtrlProcTask::CCtrlProcTask()
{
	m_bStop = false;
}

/**
 *  ��������.
 *
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
CCtrlProcTask::~CCtrlProcTask()
{
	
}

/**
 *  �麯�����̳���ACE_Task.
 *
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
int CCtrlProcTask::svc()
{	
	ACE_High_Res_Timer::global_scale_factor();

	int nRet = 0;
	nRet = OnStart();

	char szTime[PK_HOSTNAMESTRING_MAXLEN] = {'\0'};
	PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));

	while(!m_bStop)
	{
		//ACE_Time_Value tvSleep;
		//tvSleep.msec(DEFAULT_SLEEP_TIME); // 100ms
		//ACE_OS::sleep(tvSleep);		

		// channelname:simensdrv,value:[{"name":tag1,"value":"100"},{"name":tag2,"value":"100"}]
		string strChannel;
		string strMessage;
		nRet = m_redisSub.receivemessage(strChannel, strMessage, 500); // 500 ms
		// ����������״̬
		if(nRet != 0)
		{
			PKComm::Sleep(200); // receivermessage may not sleep!
			continue;
		}

		if (strChannel.compare(CHANNELNAME_CONTROL) != 0)
		{
			g_logger.LogMessage(PK_LOGLEVEL_INFO, "CtrlProcTask recv an message but not CONTROL msg!!channel:%s, message:%s", strChannel.c_str(), strMessage.c_str());
			continue;
		}

		//g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "pknodeserver �յ�����,����:%s", strMessage.c_str());

		Json::Reader reader;
		Json::Value root;
		if (!reader.parse(strMessage, root, false))
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "CtrlProcTask recv an msg(%s), not json format, invalid!", strMessage.c_str());
			continue;
		}

		if (!root.isObject() && !root.isArray())
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "CtrlProcTask recv an msg . type invalid:%d,not an object or array.message:%s!", root.type(), strMessage.c_str());
			continue;
		}

		// ����tag���ƣ��ҵ���Ӧ��������Ӧ�Ķ���ͨ�������������������� ��ȷ�ϱ����Ϳ��ƶ������ﴦ���ˡ�
		this->RelayControlCmd2Driver(strMessage.c_str(), root);
	}

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE,"nodeserver CtrlProcTask exit normally!");
	OnStop();

	return PK_SUCCESS;	
}


int CCtrlProcTask::Start()
{
	return activate(THR_NEW_LWP | THR_JOINABLE |THR_INHERIT_SCHED);
}

void CCtrlProcTask::Stop()
{
	m_bStop = true;
	int nWaitResult = wait();
}


int CCtrlProcTask::OnStart()
{
	int nStatus = PK_SUCCESS;
	int nPKMemDBListenPort = PKMEMDB_LISTEN_PORT;
	string strPKMemDBPassword = PKMEMDB_ACCESS_PASSWORD;
	GetPKMemDBPort(nPKMemDBListenPort, strPKMemDBPassword);

	nStatus = m_redisPub.initialize(PK_LOCALHOST_IP, nPKMemDBListenPort, strPKMemDBPassword.c_str(), 0);	
	if(nStatus != PK_SUCCESS)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Control Process Task, Initialize RW failed, ret:%d", nStatus);
	}	
	
	nStatus = m_redisSub.initialize(PK_LOCALHOST_IP, nPKMemDBListenPort, strPKMemDBPassword.c_str());
	if(nStatus != PK_SUCCESS)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Control Process Task, Initialize Sub failed, ret:%d", nStatus);
	}

	nStatus = m_redisSub.subscribe(CHANNELNAME_CONTROL);
	
	return nStatus;
}

// ������ֹͣʱ
void CCtrlProcTask::OnStop()
{
	m_redisSub.finalize();
	m_redisPub.finalize();
}

// �������ݸ��¼�������м�¼
int CCtrlProcTask::LogControl2Event(const char *szUserName, CDataTag *pDataTag, const char *szTagVal, const char *szTime)
{
	Json::Value	jsonCtrl;
	
	string strValueLabel = GetControlLabel(pDataTag->strName.c_str(), atoi(szTagVal));
	if (strValueLabel.length() == 0) // ���û������ ����ֵ��Ӧ����ʾ���ݣ�label����ȡȱʡ����
	{
		strValueLabel =  szTagVal;
	}
	string strContent = pDataTag->strName;
	strContent = strContent + "," + strValueLabel;

	jsonCtrl[EVENT_TYPE_MSGID] = EVENT_TYPE_CONTROL;
	jsonCtrl["username"] = szUserName;
	jsonCtrl["name"] = pDataTag->strName.c_str();
	jsonCtrl["value"] = szTagVal;

	jsonCtrl["content"] = strContent;
	jsonCtrl["rectime"] = szTime;
	jsonCtrl["subsys"] = pDataTag->strSubsysName.c_str();
	string strLogCtrl = m_jsonWriter.write(jsonCtrl); // ���ܸ���

	m_redisPub.publish(CHANNELNAME_EVENT, strLogCtrl.c_str());
	m_redisPub.commit();
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "����������־, tagname:%s, value:%s, username:%s to log", pDataTag->strName.c_str(), szTagVal, szUserName);
	return 0;
}

// Json������Ϊ��[{"name":tag1,"value":"100"},{"name":tag2,"value":"100"}]  {\"name\":\"tag1\",\"value\":\"100\"}
int CCtrlProcTask::RelayControlCmd2Driver(const char *szCtrlCmd, Json::Value &root)
{
	// g_logger.LogMessage(PK_LOGLEVEL_INFO,"pknodeserver�յ����%s", szCtrlCmd);
	int nRet = PK_SUCCESS;
	vector<Json::Value *> vectorCmds;
	if (root.isObject())
	{
		vectorCmds.push_back(&root);
	}
	else if(root.isArray())
	{
		for(unsigned int i = 0; i < root.size(); i ++)
			vectorCmds.push_back(&root[i]);
	}
	else
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "ctrls(%s),format error��ӦΪjson array or object��{\"type\":\"control\",\"data\":{\"name\":\"tag1\",\"value\":\"100\"}",szCtrlCmd);
		//g_pPKNodeServer->UpdateServerStatus(-12,"control format invalid");
		return -12;
	}

	for(vector<Json::Value *>::iterator itCmd = vectorCmds.begin(); itCmd != vectorCmds.end(); itCmd ++)
	{
		Json::Value &jsonCmd = **itCmd;

		string strActionType = "";
		Json::Value jsonActionType = jsonCmd["action"]; // ���ֿ��ƻ���ȷ�ϱ���
		// �������������������Ϊ�ǿ���
		if(!jsonActionType.isNull())
			Json2String(jsonActionType, strActionType);	
		
		if(PKStringHelper::StriCmp(strActionType.c_str(), ACTIONNAME_CLI2SERVER_ALARM_CONFIRM) == 0)
		{
			// ȷ�ϵı�������,(����ȷ�ϱ�������Ϣ��ʽ��������),�������controlͨ����
			// {"action" : "confirmalarm","tagname" : "hk_nvr220_chn1_status","judgemethod" : "HH", "username" : "admin2",  "process" : "how to process?"}
			string strUserName, strTagFilter, strJudgemethod, strLevelFilter, strProduceTime, strHowToProcess;
			Json::Value jsonTagFilter = jsonCmd["tagname"];
			Json::Value jsonJudgemethod = jsonCmd["judgemethod"];
			Json::Value jsonLevelFilter = jsonCmd["level"];
			Json::Value jsonUserName = jsonCmd["username"];
			Json::Value jsonProduceTime = jsonCmd["producetime"]; // ��ô����
			//Json::Value jsonHowToProcess = jsonCmd["process"]; // ��ô����
			Json2String(jsonUserName, strUserName);	
			Json2String(jsonTagFilter, strTagFilter);	
			Json2String(jsonJudgemethod, strJudgemethod);	
			Json2String(jsonLevelFilter, strLevelFilter);	
			Json2String(jsonProduceTime, strProduceTime);
			//Json2String(jsonHowToProcess, strHowToProcess);

			ConfirmAlarm(strTagFilter, strJudgemethod, strLevelFilter, strProduceTime, strUserName, strHowToProcess); // ֻ�б������˹�ȥ��������Ϊ�ǿ���
			continue;
		}
		else if (PKStringHelper::StriCmp(strActionType.c_str(), ACTIONNAME_CLI2SERVER_ALARM_SET_PARAM) == 0) // �޸�tag���ĳ�������Ĳ���
		{
			// �޸ı�������
			string strUserName, strTagName, strJudgemethod, strAlarmParam;
			Json::Value jsonTagName = jsonCmd["tagname"];
			Json::Value jsonJudgemethod = jsonCmd["judgemethod"];
			Json::Value jsonAlarmParam = jsonCmd["alarmparam"];
			Json::Value jsonUserName = jsonCmd["username"];
			Json2String(jsonUserName, strUserName);
			Json2String(jsonTagName, strTagName);
			Json2String(jsonJudgemethod, strJudgemethod);
			Json2String(jsonAlarmParam, strAlarmParam);

			this->m_eaprocessor.ProcessAlarmParamChange(strTagName, strJudgemethod, strAlarmParam, strUserName); // �޸ı�������
			continue;
		}
		else if (PKStringHelper::StriCmp(strActionType.c_str(), ACTIONNAME_CLI2SERVER_ALARM_SUPPRESS) == 0) // ���Ʊ���һ��ʱ��
		{
			// �������controlͨ������ʽ���£�
			// {"action":"suppressalarm", "judgemethod" : "HH", "tagname" : "hk_nvr220_chn1_status", "suppresstime" : "2018-01-28 19:15:00" }

			// ���Ʊ���һ��ʱ��
			string strUserName, strTagName, strJudgemethod, strProduceTime, strSuppressTime;
			Json::Value jsonTagName = jsonCmd["tagname"];
			Json::Value jsonJudgemethod = jsonCmd["judgemethod"];
			Json::Value jsonProuceTime = jsonCmd["producetime"];
			Json::Value jsonSuppressTime = jsonCmd["suppresstime"];
			Json::Value jsonUserName = jsonCmd["username"];
			Json2String(jsonUserName, strUserName);
			Json2String(jsonTagName, strTagName);
			Json2String(jsonJudgemethod, strJudgemethod);
			Json2String(jsonSuppressTime, strSuppressTime);
			Json2String(jsonProuceTime, strProduceTime);

			SuppressAlarm(strTagName, strJudgemethod, strProduceTime, strSuppressTime, strUserName); // �޸ı�������
			continue;
		}

		// ���ݵ�ַ�õ��豸��Ϣ�������ַ������ð��:����
		Json::Value tagName = jsonCmd["name"];
		string strTagNames = tagName.asString();

		// ����˵��ǽҪ����ǽ������ͷidд����cycle_TV_wall0��Ҳ�ǲ��������߼��ˣ��������޸ģ�ֻ������������⣡����֮��Ӧ���ǣ���Ϣ��������type�ֶΣ���������ǽ��
		// [{"name":"cycle_TV_wall0","value":{"cameras":[{"name":"testVideo","id":"1"}],"operatePriority":"","operateTimeout":""},"username":"admin"}]
		Json::Value tagValue = jsonCmd["value"];
		Json::Value jsonUserName = jsonCmd["username"];
		if(tagName.isNull() || tagValue.isNull()){
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "RelayControlCmd2Driver��cannot find name and value attributes in data, ctrl:%s", szCtrlCmd);
			nRet = -2;
			continue;
		}

		string strTagName,strTagValue;
		Json2String(tagName, strTagName);	
		Json2String(tagValue, strTagValue);	
		string strUserName = "ϵͳ";
		if(!jsonUserName.isNull())
			Json2String(jsonUserName, strUserName);	

		// ����tag�����ҵ�tag���Ӧ������
		map<string, CDataTag *>::iterator itMap = MAIN_TASK->m_mapName2Tag.find(strTagName);
		if (itMap == MAIN_TASK->m_mapName2Tag.end()){
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "RelayControlCmd2Driver��cannot find taginfo by name:%s, ctrl:%s", strTagName.c_str(), szCtrlCmd);
			nRet = -3;
			continue;
		}

		CDataTag *pDataTag = itMap->second;
		if(pDataTag == NULL){
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "RelayControlCmd2Driver, find taginfo by name:%s, pServerTag==null, ctrl:%s", strTagName.c_str(), szCtrlCmd);
			nRet = -4;
			continue;
		}

		// �ڴ������Ԥ���������ģ������������ͱ����Ŀ��������
		if (pDataTag->nTagType == VARIABLE_TYPE_MEMVAR || pDataTag->nTagType == VARIABLE_TYPE_PREDEFINE || pDataTag->nTagType == VARIABLE_TYPE_SIMULATE)
		{
			nRet = TAGPROC_TASK->PostWriteTagCmd(pDataTag, strTagValue);
			continue;
		}

		if (pDataTag->nTagType == VARIABLE_TYPE_ACCUMULATE_TIME || pDataTag->nTagType == VARIABLE_TYPE_ACCUMULATE_COUNT)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "RelayControlCmd2Driver, username:%s, tagname:%s, value:%s, �ۼƱ������ܿ���!", strUserName.c_str(), strTagName.c_str(), szCtrlCmd);
			continue;
		}

		if (pDataTag->nTagType != VARIABLE_TYPE_DEVICE)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "RelayControlCmd2Driver, username:%s, tagname:%s, value:%s, δ֪��������:%d, ���ܿ���!", strUserName.c_str(), strTagName.c_str(), szCtrlCmd, pDataTag->nTagType);
			continue;
		}

		if (!USERAUTH_TASK->HasAuthByTagAndLoginName(strUserName, pDataTag))
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "RelayControlCmd2Driver, username:%s, tagname:%s, value:%s no auth", strUserName.c_str(), strTagName.c_str(), szCtrlCmd);
			continue;
		}

		CDeviceTag *pDeviceTag = (CDeviceTag *)pDataTag;
		char szTime[PK_HOSTNAMESTRING_MAXLEN] = {'\0'};
		PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));
		Json::Value	jsonCtrl;
		jsonCtrl["action"] = strActionType;
		jsonCtrl["name"] = strTagName.c_str();
		jsonCtrl["value"] = strTagValue.c_str();
		jsonCtrl["username"] = strUserName.c_str();

		CMyDriverPhysicalDevice *pDriver = (CMyDriverPhysicalDevice *)pDeviceTag->pDevice->m_pDriver;
		CProcessQueue * pDrvProcQue = pDriver->m_pDrvShm;
		if(!pDrvProcQue)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "RelayControlCmd2Driver, driver shm by name:%s == NLL,ctrl:%s", pDriver->m_strName.c_str(), szCtrlCmd);
			nRet = -5;
			continue;
		}

		// ʵ���豸����������Ҫ���͸��豸ִ������
		string strNewCtrl = m_jsonWriter.write(jsonCtrl); // ���ܸ���
		nRet = pDrvProcQue->EnQueue(strNewCtrl.c_str(), strNewCtrl.length());
		if(nRet != 0){
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "�յ���������, ת��������: ctrl to :%s shm failed, tagname:%s, is null,ctrl:%s", pDriver->m_strName.c_str(), strTagName.c_str(), szCtrlCmd);
			nRet = -6;
			continue;
		}

		m_redisPub.commit();
		LogControl2Event(strUserName.c_str(), pDataTag, strTagValue.c_str(), szTime);
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "�յ���������, tagname:%s, value:%s, ת�ָ� ����:%s successfully!", strTagName.c_str(), strTagValue.c_str(), pDriver->m_strName.c_str());
	}
	return nRet;
}

string CCtrlProcTask::GetControlLabel(string strTagName, int nControlVal)
{
	string strLabel = "";
	map<string, CDataTag *>::iterator itmapTag = MAIN_TASK->m_mapName2Tag.find(strTagName);
	// ����tag�����ҵ�labelid
	if (itmapTag != MAIN_TASK->m_mapName2Tag.end())
	{
		CDataTag *pTag = itmapTag->second;
		// ����labelid �ҵ�=��Ӧ��map
		map<int, map<int, CLabel *> >::iterator itmapLabelMap = MAIN_TASK->m_mapId2Label.find(pTag->nLabelID);
		if (itmapLabelMap != MAIN_TASK->m_mapId2Label.end())
		{
			// ��map��map������ҵ�ǰ����ֵ��Ӧ��label
			map<int, CLabel *> mapLabel = itmapLabelMap->second;
			map<int, CLabel *>::iterator itmapLabel = mapLabel.find(nControlVal);
			if (itmapLabel != mapLabel.end())
			{
				CLabel *pLabel = itmapLabel->second;
				strLabel = pLabel->strLabelName;
			}	
		}
	}
	return strLabel;	
}

void string_replace(string&s1,const string&s2,const string&s3)
{
	string::size_type pos=0;
	string::size_type a=s2.size();
	string::size_type b=s3.size();
	while((pos=s1.find(s2,pos))!=string::npos)
	{
		s1.replace(pos,a,s3);
		pos+=b;
	}
}

// ����ȷ�ϱ���
int CCtrlProcTask::ConfirmAlarm(string &strTagFilter, string &strJudgeMethod, string &strLevelFilter, string strProduceTime, string &strUsername, string &strHowToProcess)
{
	int nRet = 0;
	int nMatchPos = strTagFilter.find("*");

	// ��ȷƥ���tag��
	if (nMatchPos == string::npos) // δ�ҵ� * �ţ��Ǿ���ƥ�䣬��һ���ж���ȷ�����м�����ȷ��ĳ������
	{
		string strTagName = strTagFilter; // ������һ��tag��
		map<string, CDataTag *>::iterator itMapTag = MAIN_TASK->m_mapName2Tag.find(strTagName);
		if (itMapTag == MAIN_TASK->m_mapName2Tag.end())
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "ȷ�ϵ�ı���ʱ��δ�ҵ���Ӧ�ĵ�:%s,judgemethod:%s", strTagName.c_str(), strJudgeMethod.c_str());
			return -1;
		}

		CDataTag *pTag = itMapTag->second;

		bool hasAlarmConfirmed = false;
		// ����������б����㣬�����ڱ����ĵ���Ѿ��ָ��ĵ����ȷ��
		for (int i = 0; i < pTag->vecAllAlarmTags.size(); i++)
		{
			CAlarmTag *pAlarmTag = pTag->vecAllAlarmTags[i];
			if (strJudgeMethod.compare("*") != 0 && PKStringHelper::StriCmp(strJudgeMethod.c_str(), pAlarmTag->strJudgeMethod.c_str()) != 0)
				continue;

			for (int i = 0; i < pAlarmTag->vecAlarming.size(); i++)
			{
				CAlarmingInfo *pAlarmingInfo = pAlarmTag->vecAlarming[i];
				char szProduceTime1[32] = { 0 };
				PKTimeHelper::HighResTime2String(szProduceTime1, sizeof(szProduceTime1), pAlarmingInfo->tmProduce.nSeconds, pAlarmingInfo->tmProduce.nMilSeconds);
				if (!strProduceTime.empty() && strProduceTime.compare(szProduceTime1) != 0)
					continue;

				if (pAlarmingInfo->bConfirmed)// ��ȷ��. 20170807�����ֹ��ڴ����Ѿ�ȷ���ˣ����ڴ����ݿⲢδȷ�ϵ����
				{
					g_logger.LogMessage(PK_LOGLEVEL_WARN, "ȷ�ϵ�ı���ʱ�������ڴ��иõ��Ѿ�ȷ�Ϲ���.�����ڼ䣬����ȷ��.��Ӧ��TAG��:%s,judgemethod:%s", strTagName.c_str(), strJudgeMethod.c_str());
					//continue;
				}

				pAlarmingInfo->strConfirmUserName = strUsername;
				//pAlarmTag->strHowToProcess = strHowToProcess; // ȷ��ʱ��˵��
				m_eaprocessor.OnEAConfirm(pAlarmTag, szProduceTime1, (char *)strUsername.c_str(), true, 0, 0);
				hasAlarmConfirmed = true;
			}
		}

		return 0;
	}

	/*
	// ģ��ƥ���tag��
	// ��֧�ְ���һ��*��,*XX��XX*����*Ϊ�����Ϊ������
	string strAlmTagPrefix =""; // *��ǰ�沿�֣�nms1.*aaa������nms1�µ����б�����nms1.
	string strAlmTagSuffix = ""; // *�ĺ��沿�֣�nms1.*aaa������nms1�µ����б�����aaa
	*/

 	string strPattern = strTagFilter;
    string_replace(strPattern, "*", "[0-9a-zA-Z\\.]*"); // ������ʽ��
    boost::regex pattern(strPattern);//,regex_constants::extended);
	map<string, CDataTag *>::iterator itMapTag = MAIN_TASK->m_mapName2Tag.begin();
	for (; itMapTag != MAIN_TASK->m_mapName2Tag.end(); itMapTag++)
	{
	 	CDataTag *pTag = itMapTag->second;
	 	string strTagName = pTag->strName;
        std::string::const_iterator start = strTagName.begin();
        std::string::const_iterator end   = strTagName.end();
        boost::smatch result;
        //boost::match_results<std::string::const_iterator> result;
        bool bValid = false;
        while (boost::regex_search(strTagName, result, pattern))
        {
            bValid = true;
            break;
        }
        //bool bValid = boost::regex_match(strTagName,result,pattern);
		if(!bValid)
			continue;

		bool hasAlarmConfirmed = false;
		for (int i = 0; i < pTag->vecAllAlarmTags.size(); i++)
		{
			CAlarmTag *pAlarmTag = pTag->vecAllAlarmTags[i];
			if (PKStringHelper::StriCmp(strJudgeMethod.c_str(), pAlarmTag->strJudgeMethod.c_str()) != 0 && strJudgeMethod.compare("*") != 0)
			{
				continue;
			}
				
			for (int i = 0; i < pAlarmTag->vecAlarming.size(); i++)
			{
				CAlarmingInfo *pAlarmingInfo = pAlarmTag->vecAlarming[i];
				char szProduceTime1[32] = { 0 };
				PKTimeHelper::HighResTime2String(szProduceTime1, sizeof(szProduceTime1), pAlarmingInfo->tmProduce.nSeconds, pAlarmingInfo->tmProduce.nMilSeconds);
				if (!strProduceTime.empty() && strProduceTime.compare(szProduceTime1) != 0)
					continue;

				m_eaprocessor.OnEAConfirmOne(pAlarmingInfo, strUsername.c_str(), true, 0, 0);
				hasAlarmConfirmed = true;
			}
		}
	}
	return nRet;
}

// �����Ʊ���ʱ
int CCtrlProcTask::SuppressAlarm(string strTagName, string strJudgeMethod, string strProuceTime, string  strSuppressTime, string strUserName)
{
	int nRet = 0;
	// ����tag�����ҵ�tag���Ӧ������
	map<string, CDataTag *>::iterator itMap = MAIN_TASK->m_mapName2Tag.find(strTagName);
	if (itMap == MAIN_TASK->m_mapName2Tag.end()){
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "OnSuppressAlarm(%s,%s,%s,%s)��cannot find taginfo by name",
			strTagName.c_str(), strJudgeMethod.c_str(), strSuppressTime.c_str(), strUserName.c_str());
		nRet = -3;
		return nRet;
	}

	CDataTag *pTag = itMap->second;
	// ����������б����㣬�����ڱ����ĵ���Ѿ��ָ��ĵ����ȷ��
	for (int i = 0; i < pTag->vecAllAlarmTags.size(); i++)
	{
		CAlarmTag *pAlarmTag = pTag->vecAllAlarmTags[i];

		// δȷ�ϵı���������б�����Ϊ*���ջ��ߺ͸ñ������
		if (strJudgeMethod.compare("*") != 0 && PKStringHelper::StriCmp(strJudgeMethod.c_str(), pAlarmTag->strJudgeMethod.c_str()) != 0)
			continue;

		m_eaprocessor.OnSuppressAlarm(pAlarmTag, (char *)strProuceTime.c_str(), strSuppressTime, strUserName);
	}
	return 0;
}