/**************************************************************
*  Filename:    MainTask.cpp
*  Copyright:   XinHong Software Co., Ltd.
*
*  Description: Maintask
*
*  @author:     xingxing
*  @version     2021/07/09 Initial Version
**************************************************************/
#include "ace/OS.h"
#include <ace/Default_Constants.h>
#include "math.h"
#ifdef _WIN32
#include "windows.h"
#else
#include "unistd.h"
#include "stdlib.h"
#endif
#include <iomanip>
#include "pklog/pklog.h"
#include "pkeviewdbapi/pkeviewdbapi.h"
#include "pkdbapi/pkdbapi.h"
#include "MainTask.h"
#include "mosquitto/mosquitto.h"
#include "eviewcomm/eviewcomm.h"
using namespace std;

extern CPKLog g_logger;
string g_strGateWayId = "";
string	g_strTopicRealData = "";
string	g_strTopicControl = "";
string  g_strTopicConfig = "";

#define TOPIC_REALDATA_PREFIX			"topic001"				// 实时数据发送通道;
#define TOPIC_CONFIG_PREFIX				"peak/config"			// 配置通道;
#define TOPIC_CONTROL_PREFIX			"electric_command"		// 控制通道;
#define NULLASSTRING(x) x == NULL ? "" : x
#define SLEEP_MSEC	500

const string strSqlEvent = "select device, code, name, action, classes, protect_type,source from t_device_event;";
const string strSqlDeviceCommon = "select id, name from t_device_list where name!= 'eventDev' and enable<>0 or enable is null;";
const string strSqlDeviceEvent = "select id, name from t_device_list where name == 'eventDev' and enable<>0 or enable is null;";
const string strSqlTag = "select name, show_type,output_type, calc_value, device_id, precision from t_device_tag where device_id>0;";
string getTime();

//************************************
// Method:    my_connect_callback
// FullName:  my_connect_callback
// Access:    public 
// Returns:   void
// Qualifier:/* 回复重连时调入，在这里将做恢复重连的事情
//1、发送全量数据
// Parameter: struct mosquitto * mosq
// Parameter: void * obj  CMqttImpl* 由 new一个mosquitto时传入
// Parameter: int rc
//************************************
void my_connect_callback(struct mosquitto *mosq, void *obj, int rc)
{
	//恢复重连，需要将之前的数据发送;
	CMainTask* mainTask = (CMainTask*)obj;
	mainTask->m_bconnected = true;
	mainTask->m_pMqttObj->m_bConnected = true;
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "connected to MQTT");

	int nMid = 0;
	int nRet = mainTask->m_pMqttObj->mqttClient_Sub((char *)g_strTopicControl.c_str(), &nMid);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "mqtt:订阅通道:接收控制命令主题:%s 失败, retcode:%d", g_strTopicControl.c_str(), nRet);
	}
	else
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "mqtt:订阅通道:接收控制命令主题:%s 成功", g_strTopicControl.c_str());
}

//************************************
//断开连接回调;
// Method:    my_disconnect_callback
// FullName:  my_disconnect_callback
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: struct mosquitto * mosq
// Parameter: void * obj  CMqttImpl* 由 new一个mosquitto时传入
// Parameter: int result
//************************************
void my_disconnect_callback(struct mosquitto *mosq, void *obj, int result)
{
	//断开连接，表示需要开始保存以后发送的数据;
	CMainTask* mainTask = (CMainTask*)obj;
	mainTask->m_bconnected = false;
	mainTask->m_pMqttObj->m_bConnected = false;
	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "MQTT Disconnected..");
}

//************************************
//发布回调 只有发布成功才会进入这里，断开连接之后不会进入这里;
// Method:    my_publish_callback
// FullName:  my_publish_callback
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: struct mosquitto * mosq
// Parameter: void * obj
// Parameter: int mid
//************************************
void my_publish_callback(struct mosquitto *mosq, void *obj, int mid)
{
	CMainTask* mainTask = (CMainTask*)obj;
	mainTask->m_mapUpdatingValues.clear();
}

//************************************
// Method:    my_message_callback
// FullName:  my_message_callback
// Access:    public 
// Returns:   void
// Qualifier: 订阅消息回调，接收来自云端的控制消息;
// Parameter: struct mosquitto * mosq
// Parameter: void * obj
// Parameter: const struct mosquitto_message * msg MSGID:MSG
//************************************
void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
	CMainTask* mainTask = (CMainTask*)obj;
	string strMessage = (char *)msg->payload;
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "recv a message of topic:%s, content:%s", msg->topic, strMessage.c_str());

	//如果是接收到云端发过来的控制通道的消息;
	if (g_strTopicControl.compare(msg->topic) != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "recv a message of topic:%s, content:%s, not control topic:%s!", msg->topic, strMessage.c_str(), g_strTopicControl.c_str());
		return;
	}
	Json::Reader jsonReader;
	Json::Value jsonMsg;
	if (!jsonReader.parse(strMessage, jsonMsg, false))
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "recv a message of topic:%s, content:%s, not json format!!", msg->topic, strMessage.c_str());
		return;
	}
	Json::Value jsonAction = jsonMsg["action"];
	string strAction = "";
	if (!jsonAction.isNull())
		strAction = jsonAction.asString();

	if (strAction.compare("control") == 0)
	{
		mainTask->SendControlMsg2LocalNodeServer(strMessage, jsonMsg);
	}
	else if (strAction.compare("allrealdata") == 0) //请求全量信息, 未验证
	{
		mainTask->UpdateAllTags2Topic();
	}
	else
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "recv a message of topic:%s, content:%s, action:%s not support!!", msg->topic, strMessage.c_str(), strAction.c_str());
		return;
	}
}

CMainTask::CMainTask()
{
	m_bStop = false;
	m_pMqttObj = NULL;
	m_dbEview.Init();
	g_strGateWayId = "";
	m_hPkData = NULL;
	m_pkAllTagDatas = NULL;
}

CMainTask::~CMainTask()
{
	m_bStop = true;
}

int CMainTask::svc()
{
	int nRet = OnStart();
	bool bFirst = true;
	ACE_Time_Value tv;
	tv.set_msec(SLEEP_MSEC);
	time_t tmLastAllData = 0; // 上次发送全量数据的时间;
	while (!m_bStop)
	{
		vector<string> vecTagsValue;
		size_t nTagNum = m_vecTagsInfo.size();

		int nRet = pkMGet(m_hPkData, m_pkAllTagDatas, nTagNum);
		for (int i = 0; i < nTagNum; i++)
		{
			if (nRet == 0)
				vecTagsValue.push_back(m_pkAllTagDatas[i].szData);
			else
				vecTagsValue.push_back(TAG_QUALITY_UNKNOWN_REASON_STRING);
		}

		if (bFirst)	//初始设置数据;
		{
			for (int i = 0; i < nTagNum; i++)
			{
				m_mapTagsName2UpdateTime[m_vecTagsInfo[i].strTagName] = m_pkAllTagDatas[i].szData;
			}
			bFirst = false;
		}

		time_t tmNow;
		time(&tmNow);
		long nTimeSpanSec = labs(tmNow - tmLastAllData);
		//first time
		if (nTimeSpanSec >= 10) // 每10秒发送1次全量数据
		{
			nRet = PublishRealTagData2Topic(m_vecTagsInfo, vecTagsValue, false);
			if (nRet == 0)
			{
				tmLastAllData = tmNow;
			}
		}
		else //数据发生改变发送;
		{
			vector<TAGINFO> vecUpdateInfo;
			vector<string> vecUpdateValues;
			bool bStateChanged = false;
			bool bCodeChanged = false;

			for (int i = 0; i < m_vecTagsInfo.size(); i++)	//将每个符合要求的数据上传到Mqtt服务订阅端;
			{
				if (m_vecTagsInfo[i].strTagName == "eventCode")  //先将数据放入待上传容器，如果两个数值均未改变，则取消上传;
				{
					vecUpdateInfo.push_back(m_vecTagsInfo[i]);
					vecUpdateValues.push_back(m_pkAllTagDatas[i].szData);
				}

				if (m_vecTagsInfo[i].strTagName == "eventState")
				{
					vecUpdateInfo.push_back(m_vecTagsInfo[i]);
					vecUpdateValues.push_back(m_pkAllTagDatas[i].szData);
				}

				map<string, string>::iterator it = m_mapTagsName2UpdateTime.find(m_vecTagsInfo[i].strTagName);
				if (it != m_mapTagsName2UpdateTime.end())
				{//取数据进行比较,不能使用Json串直接进行比较;
					Json::Value rootPre;
					Json::Value rootCur;
					Json::Reader jsonReader;
					if (jsonReader.parse(vecTagsValue[i], rootPre, false) && jsonReader.parse(it->second, rootCur, false))
					{
						if (rootPre["v"] != rootCur["v"])
						{
							if (m_vecTagsInfo[i].strTagName == "eventCode")
							{
								bCodeChanged = true;
							}
							else if (m_vecTagsInfo[i].strTagName == "eventState")
							{
								bStateChanged = true;
							}
						}
					}
				}
			}
			if (!(bStateChanged || bCodeChanged)) //两者中任意一个发生变化,则同时更新两个数据;
			{
				vecUpdateInfo.clear();
				vecUpdateValues.clear();
			}

			if (vecUpdateInfo.size() > 0)
			{
				nRet = PublishRealTagData2Topic(vecUpdateInfo, vecUpdateValues, true);
				if (nRet == 0)
				{
					g_logger.LogMessage(PK_LOGLEVEL_INFO, "gateway:%s, send increment data success", g_strGateWayId.c_str());
				}
				else
				{
					g_logger.LogMessage(PK_LOGLEVEL_ERROR, "gateway:%s, send increment data failed", g_strGateWayId.c_str());
				}
				vecUpdateInfo.clear();
				vecUpdateValues.clear();
			}
		}
		ACE_OS::sleep(tv);
	}
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "MainTask Exit!");
	OnStop();
	return PK_SUCCESS;
}

int CMainTask::OnStart()
{
	int nStatus = PK_SUCCESS;
	int nRet = 0;

	nRet = LoadConfig();
	if (nRet < 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Read Gateway file failed,exit...");
		return -1;
	}

	m_pMqttObj = new CMqttImpl((char*)m_gateWayConf.m_strMqqtIP.c_str(), atoi(m_gateWayConf.m_strPort.c_str()), (char*)g_strGateWayId.c_str());
	nRet = m_pMqttObj->mqttClient_Init(this);

	nRet = m_pMqttObj->mqttClient_Connect(3000, my_connect_callback, my_disconnect_callback, my_publish_callback, my_message_callback);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "mqtt:mqttClient_Connect failed, retcode:%d", nRet);
	}
	else
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "mqtt:mqttClient_Connect success");

	nRet = m_pMqttObj->mqttClient_StartLoopWithNewThread();
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "mqtt:subscribe task start failed, retcode:%d", nRet);
	}
	else
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "mqtt:start subscribe task OK");
	return nRet;
}

int CMainTask::OnStop()
{
	pkExit(m_hPkData);
	m_pMqttObj->mqttCLient_UnInit();
	if (m_pMqttObj != NULL)
	{
		delete m_pMqttObj;
		m_pMqttObj = NULL;
	}
	return 0;
}

int CMainTask::Start()
{
	return activate(THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED);
}

void CMainTask::Stop()
{
	m_bStop = true;
	int nWaitResult = wait();
}

int CMainTask::UpdateAllTags2Topic()
{
	vector<string> vecTagsValue;
	if (m_vecTagsInfo.size() <= 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "config info is NULL, wait for config to upload first. 配置信息为空，等待上传配置信息之后再上传点值信息");
		return -1;
	}

	size_t nTagNum = m_vecTagsInfo.size();
	int nRet = pkMGet(m_hPkData, m_pkAllTagDatas, nTagNum);

	for (int i = 0; i < nTagNum; i++)
	{
		if (nRet == 0)
			vecTagsValue.push_back(m_pkAllTagDatas[i].szData);
		else
			vecTagsValue.push_back(TAG_QUALITY_UNKNOWN_REASON_STRING);
	}
	nRet = PublishRealTagData2Topic(m_vecTagsInfo, vecTagsValue, false);
	return nRet;
}

int CMainTask::ReadAllTags()
{
	m_vecTagsInfo.clear();
	vector<vector<string> > vecRows;
	int nRet = 0;

	//获取tag点信息;
	TAGINFO tagInfo;
	string strError;
	nRet = m_dbEview.SQLExecute(strSqlTag.c_str(), vecRows, &strError);
	if (nRet == 0)
	{
		for (int i = 0; i < vecRows.size(); i++)
		{
			tagInfo.strTagName = vecRows[i][0];
			tagInfo.showType = vecRows[i][1];
			tagInfo.outputType = vecRows[i][2];
			tagInfo.calcValue = atof(vecRows[i][3].c_str());
			tagInfo.deviceID = atoi(vecRows[i][4].c_str());
			tagInfo.precision = atoi(vecRows[i][5].c_str());
			m_vecTagsInfo.push_back(tagInfo);
			vecRows[i].clear();
		}
		vecRows.clear();
	}

	// 设备状态数据获取;
	TAGINFO devStatus;
	nRet = m_dbEview.SQLExecute("select name, id from t_device_list where (enable is null or enable <> 0 )", vecRows, &strError);
	if (nRet == 0)
	{
		for (int i = 0; i < vecRows.size(); i++)
		{
			string strDeviceName = vecRows[i][0];
			string strTagName = "device." + strDeviceName + ".connstatus";
			devStatus.strTagName = strTagName;
			devStatus.deviceID = atoi(vecRows[i][1].c_str());
			m_vecTagsInfo.push_back(devStatus);
			vecRows[i].clear();
		}
		vecRows.clear();
	}

	//获取事件信息;
	EVENTINFO evnetCount;
	nRet = m_dbEview.SQLExecute(strSqlEvent.c_str(), vecRows, &strError);
	if (nRet == 0)
	{
		for (int i = 0; i < vecRows.size(); i++)
		{
			evnetCount.deviceID = atoi(vecRows[i][0].c_str());
			evnetCount.code = atoi(vecRows[i][1].c_str());
			evnetCount.name = vecRows[i][2].c_str();
			evnetCount.action = vecRows[i][3].c_str();
			evnetCount.classes = vecRows[i][4].c_str();
			evnetCount.protecttype = vecRows[i][5].c_str();
			evnetCount.source = vecRows[i][6].c_str();
			m_eventInfo.push_back(evnetCount);
			vecRows[i].clear();
		}
		vecRows.clear();
	}

	//设备信息(正常数据);
	DEVICEINFO deviceInfo;
	nRet = m_dbEview.SQLExecute(strSqlDeviceCommon.c_str(), vecRows, &strError);
	if (nRet == 0)
	{
		for (int i = 0; i < vecRows.size(); i++)
		{
			deviceInfo.deviceID = atoi(vecRows[i][0].c_str());
			deviceInfo.strName = vecRows[i][1].c_str();
			m_deviceCommon.push_back(deviceInfo);
			vecRows[i].clear();
		}
		vecRows.clear();
	}

	//设备信息(事件数据);
	nRet = m_dbEview.SQLExecute(strSqlDeviceEvent.c_str(), vecRows, &strError);
	if (nRet == 0)
	{
		for (int i = 0; i < vecRows.size(); i++)
		{
			deviceInfo.deviceID = atoi(vecRows[i][0].c_str());
			deviceInfo.strName = vecRows[i][1].c_str();
			m_deviceEvent.push_back(deviceInfo);
			vecRows[i].clear();
		}
		vecRows.clear();
	}

	size_t nTagNum = m_vecTagsInfo.size();
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "要转发的总点数为:%d", m_vecTagsInfo.size());
	m_pkAllTagDatas = new PKDATA[nTagNum]();
	for (int i = 0; i < m_vecTagsInfo.size(); i++)
	{
		PKStringHelper::Safe_StrNCpy(m_pkAllTagDatas[i].szObjectPropName, m_vecTagsInfo[i].strTagName.c_str(), sizeof(m_pkAllTagDatas[i].szObjectPropName));
		strcpy(m_pkAllTagDatas[i].szFieldName, "");
	}

	// 初始化内存数据库;
	if (m_hPkData == NULL)
	{
		m_hPkData = pkInit("127.0.0.1", NULL, NULL);
		if (m_hPkData == NULL)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Init PKData failed");
			return -1;
		}
	}
	return 0;
}

int CMainTask::LoadConfig()
{
	char szSql[] = "select id,name,description,connparam,username,password,param1,param2,param3,param4 from t_forward_list where enable=1 and protocol='mqtt'";
	vector<vector<string> > vecRows;
	string strError;
	int nRet = m_dbEview.SQLExecute(szSql, vecRows, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_CRITICAL, "!!!严重错误!!!：网关配置表 t_forward_list表 中查询异常：sql：%s, error:%s", szSql, strError.c_str());
		return -1;
	}

	//表示没有查询到
	if (vecRows.size() <= 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_CRITICAL, "!!!严重错误!!!：没有在网关配置表 t_forward_list表 中查询到数据：sql：%s", szSql);
		return -1;
	}
	else if (vecRows.size() > 1)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "!错误!：在网关配置表 t_forward_list表 中查询到多条数据：sql：%s,只取第一行", szSql);
	}

	vector<string> vecRow = vecRows[0];
	int iCol = 0;
	string strForwardId = NULLASSTRING(vecRow[iCol].c_str());
	iCol++;
	m_gateWayConf.m_strForwardName = NULLASSTRING(vecRow[iCol].c_str());
	iCol++;
	m_gateWayConf.m_strDescription = NULLASSTRING(vecRow[iCol].c_str());
	iCol++;
	m_gateWayConf.m_strConnParam = NULLASSTRING(vecRow[iCol].c_str());
	iCol++;
	m_gateWayConf.m_strUserName = NULLASSTRING(vecRow[iCol].c_str());
	iCol++;
	m_gateWayConf.m_strPassword = NULLASSTRING(vecRow[iCol].c_str());
	iCol++;
	g_strTopicRealData = NULLASSTRING(vecRow[iCol].c_str());
	iCol++;
	g_strTopicControl = NULLASSTRING(vecRow[iCol].c_str());
	iCol++;
	g_strTopicConfig = NULLASSTRING(vecRow[iCol].c_str());
	iCol++;
	vecRow.clear();
	vecRows.clear();

	vector<string> vecConnParam = PKStringHelper::StriSplit(m_gateWayConf.m_strConnParam, ":"); // ip:port

	m_gateWayConf.m_strMqqtIP = vecConnParam[0];
	if (vecConnParam.size() > 1)
		m_gateWayConf.m_strPort = vecConnParam[1];
	if (m_gateWayConf.m_strPort.empty())
		m_gateWayConf.m_strPort = "1883";

	GetGateWayId();
	nRet = ReadAllTags();
	return nRet;
}

int CMainTask::GetGateWayId()
{
	ACE_OS::macaddr_node_t macaddress;
	int result = ACE_OS::getmacaddress(&macaddress); // 在windows下是第一个网口，可能是虚拟网卡的mac地址;
	if (result != -1)
	{
		char szMacAddr[32] = { 0 };
		sprintf(szMacAddr, "%02X%02X%02X%02X%02X%02X",
			macaddress.node[0],
			macaddress.node[1],
			macaddress.node[2],
			macaddress.node[3],
			macaddress.node[4],
			macaddress.node[5]);
		g_strGateWayId = szMacAddr;
		return 0;
	}
	else
	{
		g_strGateWayId = "";
		g_logger.LogMessage(PK_LOGLEVEL_CRITICAL, "GetGatewayId failed! Please Check!!!!!!");
		return -100;
	}
}

int CMainTask::PublishRealTagData2Topic(vector<TAGINFO> vecTagsName, vector<string> vecTagsValue, bool bEvent)
{
	if (vecTagsName.size() != vecTagsValue.size())
		return -1;
	//第一层数据信息;
	Json::Value jsonTagVTQ;
	Json::Value deviceInfo;
	Json::Value subArray;
	Json::Value root;
	Json::Reader jsonReader;
	Json::FastWriter writer;
	//第二层数据信息;
	Json::Value curDevice;
	Json::Value curEventData;

	root["msg_id"] = "e29d3101-a02e-495b-a2d3-40ebeab2104a";	 //消息唯一编号，GUID
	root["tenant_id"] = 17;										 //公司编号;
	root["prj_id"] = 1;											 //项目编号;
	root["server_id"] = "XH-SRV-01";							 //边缘服务器编号;

	//事件信息;
	if (bEvent)
	{
		curDevice["gather_time"] = getTime();					//采集数据时间;
		curDevice["device_path"] = m_deviceEvent[0].strName;	//设备路径;
		curDevice["device_id"] = m_deviceEvent[0].strName;	//设备名称;
		curDevice["state"] = 0;

		//获取事件状态和代码;
		int nCodeNum, nState;
		getCodeAndState(vecTagsName, vecTagsValue, nCodeNum, nState);
		//获取事件相关信息;
		getEventJsonValue(curDevice, nCodeNum, nState);
		deviceInfo.append(curDevice);
	}
	else //数据信息;
	{
		getCommonJsonValue(vecTagsValue, vecTagsName, deviceInfo);
	}
	//添加设备节点,向云端上传数据;
	root["upload_time"] = getTime();	//消息上传时间;
	root["device_info"] = deviceInfo;
	string strSengMsg = writer.write(root);

	//记录消息,保存到日志;
	int nRet = m_pMqttObj->mqttClient_Pub((char*)g_strTopicRealData.c_str(), NULL, strSengMsg.length(), strSengMsg.c_str());
	if (nRet == PK_SUCCESS)
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "succesful to publish realdata to mqtt(%s),tagnum:%d,bytes:%d. Context: %s", m_gateWayConf.m_strMqqtIP.c_str(), vecTagsName.size(), strSengMsg.length(), strSengMsg.c_str());
	else
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "fail to publish realdata to mqtt(%s),tagnum:%d,bytes:%d, ret:%d", m_gateWayConf.m_strMqqtIP.c_str(), vecTagsName.size(), strSengMsg.length(), nRet);
	return nRet;
}

int CMainTask::SendControlMsg2LocalNodeServer(string strCtrlMsg, Json::Value &jsonMsg)
{
	if (jsonMsg["name"].isNull() || jsonMsg["value"].isNull())
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "接收到的控制命令:%s, 不包含name和value, 非法!", strCtrlMsg.c_str());
		return -1;
	}

	string strTagName = jsonMsg["name"].asString();
	string strTagValue = jsonMsg["value"].asString();

	int nRet = pkControl(m_hPkData, strTagName.c_str(), "v", strTagValue.c_str());
	if (nRet == 0)
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "接收到控制命令:%s, 已下发控制(v)成功!", strCtrlMsg.c_str());
	else
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "接收到控制命令:%s, 下发控制(v)失败,ret:%d!", strCtrlMsg.c_str(), nRet);
	return 0;
}

int CMainTask::getCodeAndState(vector<TAGINFO> &vecTagsName, vector<string> &vecTagsValue, int &code, int &state)
{
	Json::Value jsonTagVTQ;
	Json::Reader jsonReader;

	for (int j = 0; j < vecTagsName.size(); j++)
	{
		string &strTagValue = vecTagsValue[j];
		m_mapTagsName2UpdateTime[vecTagsName[j].strTagName] = strTagValue;
		jsonReader.parse(strTagValue, jsonTagVTQ, false);
		if (jsonTagVTQ["v"].asString() == "*" || jsonTagVTQ["v"].asString() == "**" || jsonTagVTQ["v"].asString() == "***")	//如果未获取到数据，则进入下一个循环;
			continue;

		if (vecTagsName[j].strTagName == "eventCode")			//获取Code代码;
		{
			code = atoi(jsonTagVTQ["v"].asString().c_str());
		}
		else if (vecTagsName[j].strTagName == "eventState")   //获取事件状态;
		{
			state = atoi(jsonTagVTQ["v"].asString().c_str());
		}
	}
	return 0;
}

int CMainTask::getEventJsonValue(Json::Value &curDevice, int code, int state)
{
	Json::Value curEventData;
	EVENTINFO eventInfo;
	eventInfo.deviceID = m_deviceEvent[0].deviceID;
	eventInfo.code = code;

	curEventData["evt_time"] = getTime();//此处时间为开始组装数据包的时间;
	list<EVENTINFO>::iterator it = find(m_eventInfo.begin(), m_eventInfo.end(), eventInfo);
	if (it != m_eventInfo.end())
	{
		if (it->action == "记录")	//区分事件的类型，然后组织成完整的数据包;
		{
			if (state == 0)
			{
				curEventData["evt_name"] = it->name + "恢复";
			}
			else
			{
				curEventData["evt_name"] = it->name + "启用";
			}
		}
		else
		{
			curEventData["evt_name"] = it->name;
		}
		curEventData["evt_class"] = it->classes;
		curEventData["evt_action"] = it->action;
		curEventData["protect_type"] = it->protecttype;
		curEventData["evt_source"] = it->source;
		curEventData["evt_info"] = state;
	}
	else
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "未找到对应的事件Code，请检查配置是否正确");
	}

	if (!curEventData.isNull())
	{
		curDevice["events"] = curEventData;
	}
	return 0;
}

int CMainTask::getCommonJsonValue(vector<string> &vecTagsValue, vector<TAGINFO> &vecTagsName, Json::Value &deviceInfo)
{
	Json::Value jsonTagVTQ;
	Json::Reader jsonReader;
	Json::Value curDevice;	//设备基本信息;
	Json::Value curMeterData;
	Json::Value curEptStatData;
	Json::Value curEnvMeasureData;

	curDevice["gather_time"] = getTime();	//开始组装数据包的时间，也就是从内存数据库获取数据的时间;
	for (int i = 0; i < m_deviceCommon.size(); i++)
	{
		curDevice["device_path"] = m_deviceCommon[i].strName;									//设备路径，格式 网关1编号/网关2编号;
		curDevice["device_id"] = m_deviceCommon[i].strName;									//设备名称;
		string strDeviceStatus = "device." + m_deviceCommon[i].strName + ".connstatus"; 		//获取设备状态;
		for (int j = 0; j < vecTagsName.size(); j++)
		{
			if (m_deviceCommon[i].deviceID == vecTagsName[j].deviceID)
			{//当前设备的信息;
				string &strTagName = vecTagsName[j].strTagName;
				string &strTagValue = vecTagsValue[j];
				m_mapTagsName2UpdateTime[strTagName] = strTagValue;	//更新上传时间点的数据;

				if ((vecTagsName[j].strTagName == "eventCode") || (vecTagsName[j].strTagName == "eventState")) //事件不做统一上传，需要单独处理;
					continue;

				string strVal; //存储获取到的数值;
				if (jsonReader.parse(strTagValue, jsonTagVTQ, false))
				{
					if (jsonTagVTQ["v"].asString() == "*" || jsonTagVTQ["v"].asString() == "**" || jsonTagVTQ["v"].asString() == "***")	//如果未获取到数据，则进入下一个循环;
						continue;
					strVal = jsonTagVTQ["v"].asString();
				}

				//获取设备状态点数据;
				if (strDeviceStatus == strTagName)
				{
					if (atoi(strVal.c_str()) == 0)
						curDevice["state"] = 1;
					else
						curDevice["state"] = 0;
					continue;
				}

				//获取需要显示的名称;
				size_t nLoc = strTagName.find_first_of('_');
				strTagName = strTagName.substr(nLoc + 1, -1);
				if (vecTagsName[j].outputType == "int")
				{
					int nOut = atoi(strVal.c_str()) * vecTagsName[j].calcValue;
					if (vecTagsName[j].showType == "electric_meter")
					{
						curMeterData[strTagName] = nOut;
					}
					else if (vecTagsName[j].showType == "ept_stat")
					{
						curEptStatData[strTagName] = nOut;
					}
					else if (vecTagsName[j].showType == "env_measure")
					{
						curEnvMeasureData[strTagName] = nOut;
					}
				}
				else if ((vecTagsName[j].outputType == "float"))
				{
					double fOut = atoi(strVal.c_str()) * vecTagsName[j].calcValue;
					char szTEMP[32] = { 0 };
					sprintf(szTEMP, "%.2f", fOut);
					if (vecTagsName[j].showType == "electric_meter")
					{
						curMeterData[strTagName] = szTEMP;
					}
					else if (vecTagsName[j].showType == "ept_stat")
					{
						curEptStatData[strTagName] = szTEMP;
					}
					else if (vecTagsName[j].showType == "env_measure")
					{
						curEnvMeasureData[strTagName] = szTEMP;
					}
				}
			}
		}

		//如果统计结果非空则将数据添加到Json中;
		if (!curMeterData.isNull())
		{
			curDevice["electric_meter"] = curMeterData;
		}
		if (!curEptStatData.isNull())
		{
			curDevice["ept_stat"] = curEptStatData;
		}
		if (!curEnvMeasureData.isNull())
		{
			curDevice["env_measure"] = curEnvMeasureData;
		}
		deviceInfo.append(curDevice);
	}
	return 0;
}

string getTime()
{
	char szBaseTime[32] = {0};
	unsigned int nMSec = 0;
	int nSec = PKTimeHelper::GetHighResTime(&nMSec);
	PKTimeHelper::HighResTime2String(szBaseTime, sizeof(szBaseTime), nSec, nMSec);
	return szBaseTime;
}