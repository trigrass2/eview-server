/**************************************************************
*  Filename:    MainTask.cpp
*  Copyright:   XinHong Software Co., Ltd.
*
*  Description: Maintask
*
*  @author:     xingxing
*  @version     2021/07/08 Initial Version
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
#include <iomanip>
#include <sstream>
using namespace std;

extern CPKLog g_logger;
string g_strGateWayId = "";
string	g_strTopicRealData = "";
string	g_strTopicControl = "";
string  g_strTopicConfig = "";

#define TOPIC_REALDATA_PREFIX			"topic001"				// ʵʱ���ݷ���ͨ��;
#define TOPIC_CONFIG_PREFIX				"peak/config"			// ����ͨ��;
#define TOPIC_CONTROL_PREFIX			"electric_command"		// ����ͨ��;
#define NULLASSTRING(x) x == NULL ? "" : x
#define SLEEP_MSEC	500

const string strSqlEvent = "select device, code, name, action, classes, protect_type,source from t_device_event;";
const string strSqlDeviceCommon = "select id, name from t_device_list where name!= 'eventDev' and enable<>0 or enable is null;";
const string strSqlDeviceEvent = "select id, name from t_device_list where driver_id =20000 and enable<>0 or enable is null;";
const string strSqlTag = "select name, show_type,output_type, calc_value, device_id, precision from t_device_tag where device_id>0;";
string getTime();
string getRound(float src, int bits);

//************************************
// Method:    my_connect_callback
// FullName:  my_connect_callback
// Access:    public 
// Returns:   void
// Qualifier:/* �ظ�����ʱ���룬�����ｫ���ָ�����������
//1������ȫ������
// Parameter: struct mosquitto * mosq
// Parameter: void * obj  CMqttImpl* �� newһ��mosquittoʱ����
// Parameter: int rc
//************************************
void my_connect_callback(struct mosquitto *mosq, void *obj, int rc)
{
	//�ָ���������Ҫ��֮ǰ�����ݷ���;
	CMainTask* mainTask = (CMainTask*)obj;
	mainTask->m_bconnected = true;
	mainTask->m_pMqttObj->m_bConnected = true;
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "connected to MQTT");

	int nMid = 0;
	int nRet = mainTask->m_pMqttObj->mqttClient_Sub((char *)g_strTopicControl.c_str(), &nMid);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "mqtt:����ͨ��:���տ�����������:%s ʧ��, retcode:%d", g_strTopicControl.c_str(), nRet);
	}
	else
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "mqtt:����ͨ��:���տ�����������:%s �ɹ�", g_strTopicControl.c_str());
}

//************************************
//�Ͽ����ӻص�;
// Method:    my_disconnect_callback
// FullName:  my_disconnect_callback
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: struct mosquitto * mosq
// Parameter: void * obj  CMqttImpl* �� newһ��mosquittoʱ����
// Parameter: int result
//************************************
void my_disconnect_callback(struct mosquitto *mosq, void *obj, int result)
{
	//�Ͽ����ӣ���ʾ��Ҫ��ʼ�����Ժ��͵�����;
	CMainTask* mainTask = (CMainTask*)obj;
	mainTask->m_bconnected = false;
	mainTask->m_pMqttObj->m_bConnected = false;
	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "MQTT Disconnected..");
}

//************************************
//�����ص� ֻ�з����ɹ��Ż��������Ͽ�����֮�󲻻��������;
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
// Qualifier: ������Ϣ�ص������������ƶ˵Ŀ�����Ϣ;
// Parameter: struct mosquitto * mosq
// Parameter: void * obj
// Parameter: const struct mosquitto_message * msg MSGID:MSG
//************************************
void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
	CMainTask* mainTask = (CMainTask*)obj;
	string strMessage = (char *)msg->payload;
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "recv a message of topic:%s, content:%s", msg->topic, strMessage.c_str());

	//����ǽ��յ��ƶ˷������Ŀ���ͨ������Ϣ;
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
	else if (strAction.compare("allrealdata") == 0) //����ȫ����Ϣ, δ��֤
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
	time_t tmLastAllData = 0; // �ϴη���ȫ�����ݵ�ʱ��;
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

		if (bFirst)	//��ʼ��������;
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
		if (nTimeSpanSec >= 10) // ÿ10�뷢��1��ȫ������;
		{
			nRet = PublishRealTagData2Topic(m_vecTagsInfo, vecTagsValue, false);
			if (nRet == 0)
				tmLastAllData = tmNow;
		}
		else //���ݷ����ı䷢�ͣ����յ��¼���Ϣ;
		{
			vector<TAGINFO> vecEventTag; //�洢�¼���صĵ�λ��Ϣ;
			vector<string> vecEventTagValues;//�洢�¼���ص�λ��ֵ;
			vector<TAGINFO> vUpdateTag;	//�洢���ݷ����仯�ĵ�λ��Ϣ;
			vector<string> vUpdateValue;  //�洢�����仯��λ��ֵ;
			map<string, int> mState;
			map<string, int> mCode;

			for (int i = 0; i < m_vecTagsInfo.size(); i++)//���¼���λ��״̬�ı����ɸѡ����;
			{
				if ((m_vecTagsInfo[i].strTagName.find("eventCode") != m_vecTagsInfo[i].strTagName.npos) || (m_vecTagsInfo[i].strTagName.find("eventState") != m_vecTagsInfo[i].strTagName.npos))
				{
					//���¼����������Ԥ�ȴ����vector��;
					vecEventTag.push_back(m_vecTagsInfo[i]);
					vecEventTagValues.push_back(m_pkAllTagDatas[i].szData);

					map<string, string>::iterator it = m_mapTagsName2UpdateTime.find(m_vecTagsInfo[i].strTagName);
					if (it != m_mapTagsName2UpdateTime.end())
					{
						Json::Value rootPre;
						Json::Value rootCur;
						Json::Reader jsonReader;
						if (jsonReader.parse(vecTagsValue[i], rootPre, false) && jsonReader.parse(it->second, rootCur, false))
						{
							if ((m_vecTagsInfo[i].strTagName.find("eventCode") != m_vecTagsInfo[i].strTagName.npos))
							{
								if (rootPre["v"] != rootCur["v"])//code ��ֵ�ı�;
									mCode[m_vecTagsInfo[i].strTagName] = 1;
								else
									mCode[m_vecTagsInfo[i].strTagName] = 0;
							}
							else
							{
								if (rootPre["v"] != rootCur["v"])//state ��ֵ�ı�;
									mState[m_vecTagsInfo[i].strTagName] = 1;
								else
									mState[m_vecTagsInfo[i].strTagName] = 0;
							}
						}
					}
				}
			}

			//����������,��map�е�״̬���Ӧ;
			if (mCode.size() != mState.size() || vecEventTag.size() != vecEventTagValues.size())
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "״̬�����豸��λ��ƥ�䣬�����λ����");
			else
			{
				map<string, int>::iterator iterState = mState.begin();
				map<string, int>::iterator iterCode = mCode.begin();
				int nCurLocation = 0;
				//��ȡʵ��״̬�����ı�ĵ�λ��Ϣ;
				for (; iterState != mState.end(), iterCode != mCode.end(); iterState++, iterCode++)
				{
					if (iterCode->second != 0 || iterState->second != 0)
					{
						vUpdateTag.push_back(vecEventTag[nCurLocation]);
						vUpdateValue.push_back(vecEventTagValues[nCurLocation]);
						nCurLocation++;
						vUpdateTag.push_back(vecEventTag[nCurLocation]);
						vUpdateValue.push_back(vecEventTagValues[nCurLocation]);
						nCurLocation++;
					}
					else
						nCurLocation += 2;
				}
			}

			if (vUpdateTag.size() > 0)
			{
				nRet = PublishRealTagData2Topic(vUpdateTag, vUpdateValue, true);
				if (nRet == 0)
				{
					g_logger.LogMessage(PK_LOGLEVEL_INFO, "gateway:%s, send increment data success", g_strGateWayId.c_str());
				}
				else
				{
					g_logger.LogMessage(PK_LOGLEVEL_ERROR, "gateway:%s, send increment data failed", g_strGateWayId.c_str());
				}
			}
			//�����������;
			vUpdateTag.clear();
			vUpdateValue.clear();
			vecEventTag.clear();
			vecEventTagValues.clear();
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
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "config info is NULL, wait for config to upload first. ������ϢΪ�գ��ȴ��ϴ�������Ϣ֮�����ϴ���ֵ��Ϣ");
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

	//��ȡtag����Ϣ;
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

	// �豸״̬���ݻ�ȡ;
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

	//��ȡ�¼���Ϣ;
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

	//�豸��Ϣ(��������);
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

	//�豸��Ϣ(�¼�����);
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
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "Ҫת�����ܵ���Ϊ:%d", m_vecTagsInfo.size());
	m_pkAllTagDatas = new PKDATA[nTagNum]();
	for (int i = 0; i < m_vecTagsInfo.size(); i++)
	{
		PKStringHelper::Safe_StrNCpy(m_pkAllTagDatas[i].szObjectPropName, m_vecTagsInfo[i].strTagName.c_str(), sizeof(m_pkAllTagDatas[i].szObjectPropName));
		strcpy(m_pkAllTagDatas[i].szFieldName, "");
	}

	// ��ʼ���ڴ����ݿ�;
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
		g_logger.LogMessage(PK_LOGLEVEL_CRITICAL, "!!!���ش���!!!���������ñ� t_forward_list�� �в�ѯ�쳣��sql��%s, error:%s", szSql, strError.c_str());
		return -1;
	}

	//��ʾû�в�ѯ��
	if (vecRows.size() <= 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_CRITICAL, "!!!���ش���!!!��û�����������ñ� t_forward_list�� �в�ѯ�����ݣ�sql��%s", szSql);
		return -1;
	}
	else if (vecRows.size() > 1)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "!����!�����������ñ� t_forward_list�� �в�ѯ���������ݣ�sql��%s,ֻȡ��һ��", szSql);
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
	int result = ACE_OS::getmacaddress(&macaddress); // ��windows���ǵ�һ�����ڣ�����������������mac��ַ;
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
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "TAG����������ֵ������ƥ�䣬�޷�����Json������������á�");
		return -1;
	}
	//��һ��������Ϣ;
	Json::Value deviceInfo;
	Json::Value root;
	Json::FastWriter writer;
	//�ڶ���������Ϣ;
	Json::Value curDevice;
	Json::Value curEventData;

	root["msg_id"] = "e29d3101-a02e-495b-a2d3-40ebeab2104a";	 //��ϢΨһ��ţ�GUID
	root["tenant_id"] = 17;										 //��˾���;
	root["prj_id"] = 1;											 //��Ŀ���;
	root["server_id"] = "XH-SRV-01";							 //��Ե���������;

	//�¼���Ϣ;
	if (bEvent)
	{
		for (int i = 0; i < vecTagsName.size(); i += 2)
		{
			curDevice["gather_time"] = getTime();//�ɼ�����ʱ��;
			curDevice["state"] = 0;
			//��ȡ�豸����;
			int curDeviceId;
			for (int j = 0; j < m_deviceEvent.size(); j++)
			{
				if (vecTagsName[i].deviceID == m_deviceEvent[j].deviceID)
				{
					curDevice["device_path"] = m_deviceEvent[j].strName;//�豸·��;
					curDevice["device_id"] = m_deviceEvent[j].strName;	//�豸����;
					curDeviceId = m_deviceEvent[j].deviceID;
				}
			}
			//��ȡ�¼�״̬�ʹ���;
			int nCodeNum, nState;
			getCodeAndState(vecTagsName[i], vecTagsName[i + 1], vecTagsValue[i], vecTagsValue[i + 1], nCodeNum, nState);
			//��ȡ�¼������Ϣ;
			getEventJsonValue(curDevice, curDeviceId, nCodeNum, nState);
			deviceInfo.append(curDevice);
			//��յ�ǰ�豸;
			curDevice.clear();
		}

	}
	else //������Ϣ;
	{
		getCommonJsonValue(vecTagsValue, vecTagsName, deviceInfo);
	}
	//����豸�ڵ�,���ƶ��ϴ�����;
	root["upload_time"] = getTime();	//��Ϣ�ϴ�ʱ��;
	root["device_info"] = deviceInfo;
	string strSengMsg = writer.write(root);

	//���;
	deviceInfo.clear();
	root.clear();

	//��¼��Ϣ,���浽��־;
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
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "���յ��Ŀ�������:%s, ������name��value, �Ƿ�!", strCtrlMsg.c_str());
		return -1;
	}

	string strTagName = jsonMsg["name"].asString();
	string strTagValue = jsonMsg["value"].asString();

	int nRet = pkControl(m_hPkData, strTagName.c_str(), "v", strTagValue.c_str());
	if (nRet == 0)
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "���յ���������:%s, ���·�����(v)�ɹ�!", strCtrlMsg.c_str());
	else
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "���յ���������:%s, �·�����(v)ʧ��,ret:%d!", strCtrlMsg.c_str(), nRet);
	return 0;
}

int CMainTask::getCodeAndState(TAGINFO &TagsCode, TAGINFO &TagsState, string &vecTagsValueCode, string &vecTagsValueState, int &code, int &state)
{
	Json::Value jsonTagCode;
	Json::Value jsonTagState;
	Json::Reader jsonReader;

	string &strTagValueCode = vecTagsValueCode;
	string &strTagValueState = vecTagsValueState;

	//��������;
	m_mapTagsName2UpdateTime[TagsCode.strTagName] = strTagValueCode;
	m_mapTagsName2UpdateTime[TagsState.strTagName] = strTagValueState;

	jsonReader.parse(strTagValueCode, jsonTagCode, false);
	jsonReader.parse(strTagValueState, jsonTagState, false);

	if ((jsonTagCode["v"].asString() == "*" || jsonTagCode["v"].asString() == "**" || jsonTagCode["v"].asString() == "***") ||
		(jsonTagState["v"].asString() == "*" || jsonTagState["v"].asString() == "**" || jsonTagState["v"].asString() == "***"))
	{
		code = 0;
		state = 0;
	}

	code = atoi(jsonTagCode["v"].asString().c_str());
	state = atoi(jsonTagState["v"].asString().c_str());
	return 0;
}

int CMainTask::getEventJsonValue(Json::Value &curDevice, int deviceID, int code, int state)
{
	Json::Value curEventData;
	EVENTINFO eventInfo;
	eventInfo.deviceID = deviceID;
	eventInfo.code = code;

	curEventData["evt_time"] = getTime();//�˴�ʱ��Ϊ��ʼ��װ���ݰ���ʱ��;
	list<EVENTINFO>::iterator it = find(m_eventInfo.begin(), m_eventInfo.end(), eventInfo);
	if (it != m_eventInfo.end())
	{
		if (it->action == "��¼")	//�����¼������ͣ�Ȼ����֯�����������ݰ�;
		{
			if (state == 0)
			{
				curEventData["evt_name"] = it->name + "�ָ�";
			}
			else
			{
				curEventData["evt_name"] = it->name + "����";
			}
		}
		else
		{
			if (state == 1)
			{
				curEventData["evt_name"] = it->name;
			}
			else
			{
				curEventData["evt_name"] = it->name + "�ָ�";
			}
		}
		curEventData["evt_class"] = it->classes;
		curEventData["evt_action"] = it->action;
		curEventData["protect_type"] = it->protecttype;
		curEventData["evt_source"] = it->source;
		curEventData["evt_info"] = state;
	}
	else
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "δ�ҵ���Ӧ���¼�Code�����������Ƿ���ȷ");
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
	Json::Value curDevice;//�豸������Ϣ;
	Json::Value curMeterData;
	Json::Value curEptStatData;
	Json::Value curEnvMeasureData;
	for (int i = 0; i < m_deviceCommon.size(); i++)
	{
		//������¼��豸,��������������;
		if ((m_deviceCommon[i].strName.find("eventDevice") != m_deviceCommon[i].strName.npos))
		{
			continue;
		}

		curDevice.clear();
		curMeterData.clear();
		curEptStatData.clear();
		curEnvMeasureData.clear();

		curDevice["gather_time"] = getTime();					//��ʼ��װ���ݰ���ʱ�䣬Ҳ���Ǵ��ڴ����ݿ��ȡ���ݵ�ʱ��;
		curDevice["device_path"] = m_deviceCommon[i].strName;	//�豸·������ʽ ����1���/����2���;
		curDevice["device_id"] = m_deviceCommon[i].strName;		//�豸����;
		for (int j = 0; j < vecTagsName.size(); j++)
		{
			string strDeviceStatus = "device." + m_deviceCommon[i].strName + ".connstatus"; //��ȡ�豸״̬;
			if (m_deviceCommon[i].deviceID == vecTagsName[j].deviceID)
			{//��ǰ�豸����Ϣ;
				string &strTagName = vecTagsName[j].strTagName;
				string &strTagValue = vecTagsValue[j];
				m_mapTagsName2UpdateTime[strTagName] = strTagValue;							//�����ϴ�ʱ��������;

				if ((vecTagsName[j].strTagName.find("eventCode") != vecTagsName[j].strTagName.npos) || (vecTagsName[j].strTagName.find("eventState") != vecTagsName[j].strTagName.npos)) //�¼�����ͳһ�ϴ�����Ҫ��������;
					continue;

				string strVal; //�洢��ȡ������ֵ;
				if (jsonReader.parse(strTagValue, jsonTagVTQ, false))
				{
					if (jsonTagVTQ["v"].asString() == "*" || jsonTagVTQ["v"].asString() == "**" || jsonTagVTQ["v"].asString() == "***")	//���δ��ȡ�����ݣ��������һ��ѭ��;
						continue;
					strVal = jsonTagVTQ["v"].asString();
				}

				//��ȡ�豸״̬������;
				if (strDeviceStatus == strTagName)
				{
					if (atoi(strVal.c_str()) == 0)
						curDevice["state"] = 1;
					else
						curDevice["state"] = 0;
					continue;
				}

				//��ȡ��Ҫ��ʾ������;
				size_t nLoc = strTagName.find_first_of('_');
				strTagName = strTagName.substr(nLoc + 1, -1);
				if (vecTagsName[j].outputType == "int")
				{
					long nOut;
					if (vecTagsName[j].calcValue == 0)
					{
						nOut = atoi(strVal.c_str());
					}
					else
					{
						nOut = atoi(strVal.c_str()) * vecTagsName[j].calcValue;
					}

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
					float fVal;
					if (vecTagsName[j].calcValue == 0)
					{
						fVal = atof(strVal.c_str());
					}
					else
					{
						fVal = atof(strVal.c_str()) * vecTagsName[j].calcValue;
					}
					string strPrecision = getRound(fVal, vecTagsName[j].precision);

					//long fResult = fVal * pow(10, vecTagsName[j].precision);
					//float fRet = fResult*1.0 / pow(10, vecTagsName[j].precision);
					if (vecTagsName[j].showType == "electric_meter")
					{
						curMeterData[strTagName] = strPrecision.c_str();
					}
					else if (vecTagsName[j].showType == "ept_stat")
					{
						curEptStatData[strTagName] = strPrecision.c_str();
					}
					else if (vecTagsName[j].showType == "env_measure")
					{
						curEnvMeasureData[strTagName] = strPrecision.c_str();
					}
				}
			}
		}
		//���ͳ�ƽ���ǿ���������ӵ�Json��;
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
	char szBaseTime[32] = { 0 };
	unsigned int nMSec = 0;
	int nSec = PKTimeHelper::GetHighResTime(&nMSec);
	PKTimeHelper::HighResTime2String(szBaseTime, sizeof(szBaseTime), nSec, nMSec);
	return szBaseTime;
}

string getRound(float src, int bits)
{
	char szValue[32] = { 0 };
	if (bits == 1)
	{
		sprintf(szValue, "%.1f", src);
	}
	else if (bits == 2)
	{
		sprintf(szValue, "%.2f", src);
	}
	else if (bits == 3)
	{
		sprintf(szValue, "%.3f", src);
	}
	else if (bits == 4)
	{
		sprintf(szValue, "%.4f", src);
	}
	string strRtn = szValue;
	return strRtn;
}