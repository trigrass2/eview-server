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
#include "PKSuperNodeServer.h"
#include "DataProcTask.h"
#include "boost/regex.hpp"
#include "MainTask.h" 

extern CPKSuperNodeServer* g_pPKNodeServer;
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
		this->RelayControlCmd2Node(strMessage.c_str(), root);
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
int CCtrlProcTask::LogControl2Event(const char *szUserName, CNodeTag *pDataTag, const char *szTagVal, const char *szTime)
{
	Json::Value	jsonCtrl;
	
	string strValueLabel = szTagVal; // GetControlLabel(pDataTag->m_strName.c_str(), atoi(szTagVal));
	if (strValueLabel.length() == 0) // ���û������ ����ֵ��Ӧ����ʾ���ݣ�label����ȡȱʡ����
	{
		strValueLabel =  szTagVal;
	}
	string strContent = pDataTag->m_strName;
	strContent = strContent + "," + strValueLabel;

	jsonCtrl[EVENT_TYPE_MSGID] = EVENT_TYPE_CONTROL;
	jsonCtrl["username"] = szUserName;
	jsonCtrl["name"] = pDataTag->m_strName.c_str();
	jsonCtrl["value"] = szTagVal;

	jsonCtrl["content"] = strContent;
	jsonCtrl["rectime"] = szTime;
	//jsonCtrl["subsys"] = pDataTag->strSubsysName.c_str();
	string strLogCtrl = m_jsonWriter.write(jsonCtrl); // ���ܸ���

	m_redisPub.publish(CHANNELNAME_EVENT, strLogCtrl.c_str());
	m_redisPub.commit();
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "����������־, tagname:%s, value:%s, username:%s to log", pDataTag->m_strName.c_str(), szTagVal, szUserName);
	return 0;
}

// Json������Ϊ��[{"name":tag1,"value":"100"},{"name":tag2,"value":"100"}]  {\"name\":\"tag1\",\"value\":\"100\"}
int CCtrlProcTask::RelayControlCmd2Node(const char *szCtrlCmd, Json::Value &root)
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
		g_pPKNodeServer->UpdateServerStatus(-12,"control format invalid");
		return -12;
	}

	for(vector<Json::Value *>::iterator itCmd = vectorCmds.begin(); itCmd != vectorCmds.end(); itCmd ++)
	{
		Json::Value &jsonCmd = **itCmd;

		string strActionType = "control";
		Json::Value jsonActionType = jsonCmd["action"]; // ���ֿ��ƻ���ȷ�ϱ���
		// �������������������Ϊ�ǿ���
		if(!jsonActionType.isNull())
			Json2String(jsonActionType, strActionType);	
		if(strActionType.empty())
			strActionType = "control";

		// ���ݵ�ַ�õ��豸��Ϣ�������ַ������ð��:����
		Json::Value tagName = jsonCmd["name"];
		string strTagNames = tagName.asString();

		// ����˵��ǽҪ����ǽ������ͷidд����cycle_TV_wall0��Ҳ�ǲ��������߼��ˣ��������޸ģ�ֻ������������⣡����֮��Ӧ���ǣ���Ϣ��������type�ֶΣ���������ǽ��
		// [{"name":"cycle_TV_wall0","value":{"cameras":[{"name":"testVideo","id":"1"}],"operatePriority":"","operateTimeout":""},"username":"admin"}]
		Json::Value tagValue = jsonCmd["value"];
		Json::Value jsonUserName = jsonCmd["username"];
		if(tagName.isNull() || tagValue.isNull()){
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "RelayControlCmd2Node��cannot find name and value attributes in data, ctrl:%s", szCtrlCmd);
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
		map<string, CNodeTag *>::iterator itMap = MAIN_TASK->m_mapName2Tag.find(strTagName);
		if (itMap == MAIN_TASK->m_mapName2Tag.end()){
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "RelayControlCmd2Node��cannot find taginfo by name:%s, ctrl:%s", strTagName.c_str(), szCtrlCmd);
			nRet = -3;
			continue;
		}

		CNodeTag *pDataTag = itMap->second;
		if(pDataTag == NULL){
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "RelayControlCmd2Node, find taginfo by name:%s, pServerTag==null, ctrl:%s", strTagName.c_str(), szCtrlCmd);
			nRet = -4;
			continue;
		} 

		CNodeInfo *pNode = pDataTag->m_pNode;
		char szTime[PK_HOSTNAMESTRING_MAXLEN] = {'\0'};
		PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));
		Json::Value	jsonCtrl;
		jsonCtrl["action"] = strActionType;
		jsonCtrl["name"] = strTagName.c_str();
		jsonCtrl["value"] = strTagValue.c_str();
		jsonCtrl["username"] = strUserName.c_str();
		string strNewCtrl = m_jsonWriter.write(jsonCtrl); // ���ܸ���

		// uppernode/control/ABCDABCDABCD
		string strTopicControl = TOPIC_CONTROL_PREFIX;;// ʵʱ���ݷ���ͨ��, peak/realdata/{gatewayID}
		strTopicControl = strTopicControl + "/" + pNode->m_strCode; // ���е�ʵʱ����ͨ����

		int nMid = 0;
		int nRet = MAIN_TASK->m_pMqttObj->mqttClient_Pub(strTopicControl.c_str(), &nMid, strNewCtrl.length(), strNewCtrl.c_str());
		// ʵ���豸����������Ҫ���͸��豸ִ������ 
		if(nRet != 0){
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "�յ���������, ת�����ڵ�: ctrl to :%s shm failed, tagname:%s, mqttClient_Pub:%d ,ctrl:%s", 
				pNode->m_strCode.c_str(), strTagName.c_str(), nRet, szCtrlCmd);
			nRet = -6;
			continue;
		}

		//LogControl2Event(strUserName.c_str(), pDataTag, strTagValue.c_str(), szTime);
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "�յ���������, tagname:%s, value:%s, ת�����ڵ�:%s successfully!", strTagName.c_str(), strTagValue.c_str(), pNode->m_strCode.c_str());
	}
	return nRet;
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
