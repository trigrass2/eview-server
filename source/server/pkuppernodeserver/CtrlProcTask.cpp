/**************************************************************
 *  Filename:    CtrlProcessTask.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 控制命令处理类.
 *
 *  @author:    shijunpu
 *  @version    05/28/2008  确认和删除报警也放在控制里面进行执行。这里也是二道贩子，将其转发给数据处理就完事
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

// 如果json是object那么asString会异常
// 如果json是string，toStyledString会多出双引号，故而需要写个函数实现
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
		strJson = CONTROLPROCESS_TASK->m_jsonWriter.write(json); // 性能更高
		// strJson = json.toStyledString();
	}

	return 0;
}

/**
 *  构造函数.
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
 *  析构函数.
 *
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
CCtrlProcTask::~CCtrlProcTask()
{
	
}

/**
 *  虚函数，继承自ACE_Task.
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
		// 更新驱动的状态
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

		//g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "pknodeserver 收到命令,内容:%s", strMessage.c_str());

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

		// 根据tag名称，找到对应的驱动对应的订阅通道，发给该驱动即结束 【确认报警和控制都在这里处理了】
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

// 本任务停止时
void CCtrlProcTask::OnStop()
{
	m_redisSub.finalize();
	m_redisPub.finalize();
}

// 发送数据给事件服务进行记录
int CCtrlProcTask::LogControl2Event(const char *szUserName, CNodeTag *pDataTag, const char *szTagVal, const char *szTime)
{
	Json::Value	jsonCtrl;
	
	string strValueLabel = szTagVal; // GetControlLabel(pDataTag->m_strName.c_str(), atoi(szTagVal));
	if (strValueLabel.length() == 0) // 如果没有设置 控制值对应的显示内容（label），取缺省内容
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
	string strLogCtrl = m_jsonWriter.write(jsonCtrl); // 性能更高

	m_redisPub.publish(CHANNELNAME_EVENT, strLogCtrl.c_str());
	m_redisPub.commit();
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "变量控制日志, tagname:%s, value:%s, username:%s to log", pDataTag->m_strName.c_str(), szTagVal, szUserName);
	return 0;
}

// Json中数据为：[{"name":tag1,"value":"100"},{"name":tag2,"value":"100"}]  {\"name\":\"tag1\",\"value\":\"100\"}
int CCtrlProcTask::RelayControlCmd2Node(const char *szCtrlCmd, Json::Value &root)
{
	// g_logger.LogMessage(PK_LOGLEVEL_INFO,"pknodeserver收到命令：%s", szCtrlCmd);
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
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "ctrls(%s),format error，应为json array or object：{\"type\":\"control\",\"data\":{\"name\":\"tag1\",\"value\":\"100\"}",szCtrlCmd);
		g_pPKNodeServer->UpdateServerStatus(-12,"control format invalid");
		return -12;
	}

	for(vector<Json::Value *>::iterator itCmd = vectorCmds.begin(); itCmd != vectorCmds.end(); itCmd ++)
	{
		Json::Value &jsonCmd = **itCmd;

		string strActionType = "control";
		Json::Value jsonActionType = jsonCmd["action"]; // 区分控制还是确认报警
		// 报警单独处理，其余的认为是控制
		if(!jsonActionType.isNull())
			Json2String(jsonActionType, strActionType);	
		if(strActionType.empty())
			strActionType = "control";

		// 根据地址得到设备信息和其余地址串。以冒号:隔开
		Json::Value tagName = jsonCmd["name"];
		string strTagNames = tagName.asString();

		// 刑星说上墙要将上墙的摄像头id写到点cycle_TV_wall0，也记不清具体的逻辑了，先暂作修改，只针对南宁的问题！！！之后应该是，消息里面增加type字段，类型是上墙。
		// [{"name":"cycle_TV_wall0","value":{"cameras":[{"name":"testVideo","id":"1"}],"operatePriority":"","operateTimeout":""},"username":"admin"}]
		Json::Value tagValue = jsonCmd["value"];
		Json::Value jsonUserName = jsonCmd["username"];
		if(tagName.isNull() || tagValue.isNull()){
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "RelayControlCmd2Node，cannot find name and value attributes in data, ctrl:%s", szCtrlCmd);
			nRet = -2;
			continue;
		}

		string strTagName,strTagValue;
		Json2String(tagName, strTagName);	
		Json2String(tagValue, strTagValue);	
		string strUserName = "系统";
		if(!jsonUserName.isNull())
			Json2String(jsonUserName, strUserName);	

		// 根据tag名称找到tag点对应的驱动
		map<string, CNodeTag *>::iterator itMap = MAIN_TASK->m_mapName2Tag.find(strTagName);
		if (itMap == MAIN_TASK->m_mapName2Tag.end()){
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "RelayControlCmd2Node，cannot find taginfo by name:%s, ctrl:%s", strTagName.c_str(), szCtrlCmd);
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
		string strNewCtrl = m_jsonWriter.write(jsonCtrl); // 性能更高

		// uppernode/control/ABCDABCDABCD
		string strTopicControl = TOPIC_CONTROL_PREFIX;;// 实时数据发送通道, peak/realdata/{gatewayID}
		strTopicControl = strTopicControl + "/" + pNode->m_strCode; // 所有的实时数据通道。

		int nMid = 0;
		int nRet = MAIN_TASK->m_pMqttObj->mqttClient_Pub(strTopicControl.c_str(), &nMid, strNewCtrl.length(), strNewCtrl.c_str());
		// 实际设备的驱动，需要发送给设备执行命令 
		if(nRet != 0){
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "收到控制命令, 转发给节点: ctrl to :%s shm failed, tagname:%s, mqttClient_Pub:%d ,ctrl:%s", 
				pNode->m_strCode.c_str(), strTagName.c_str(), nRet, szCtrlCmd);
			nRet = -6;
			continue;
		}

		//LogControl2Event(strUserName.c_str(), pDataTag, strTagValue.c_str(), szTime);
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "收到控制命令, tagname:%s, value:%s, 转发给节点:%s successfully!", strTagName.c_str(), strTagValue.c_str(), pNode->m_strCode.c_str());
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
