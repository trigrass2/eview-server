/**************************************************************
 *  Filename:    CtrlProcessTask.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: �����������.
 *
 *  @author:   shijunpu
**************************************************************/

#include <ace/Default_Constants.h>
#include "ace/Thread_Mutex.h"
#include "ace/Guard_T.h"
#include "float.h"
#include "math.h"
#include "MainTask.h"
#include "errcode/ErrCode_iCV_DA.hxx"
#include "pkcomm/pkcomm.h"
#include "sqlite/CppSQLite3.h"
#include "eviewcomm/eviewcomm.h"
#include "json/json.h"
#include "common/RMAPI.h"
#include "common/eviewdefine.h"
#include "NodeCommonDef.h"
#include "pklog/pklog.h"
#include "PKSuperNodeServer.h"
#include "CtrlProcTask.h"
#include "DataProcTask.h"
#include "RedisFieldDefine.h"
#include "common/PKMiscHelper.h"
#include "mqttImpl.h"

#include <string>
#include <vector>
using namespace std;

#define NULLASSTRING(x) x==NULL?"":x

extern CPKLog g_logger;
extern CPKSuperNodeServer *		g_pPKNodeServer;
extern CProcessQueue *			g_pQueData2NodeSrv;

#define  CHECK_NULL_RETURN_ERR(X)  { if(!X) return EC_ICV_DA_CONFIG_FILE_STRUCTURE_ERROR; }
#define	DEFAULT_SLEEP_TIME			100	

#include <fstream>  
#include <streambuf>  
#include <iostream>  

void my_connect_callback(struct mosquitto *mosq, void *obj, int rc);
void my_disconnect_callback(struct mosquitto *mosq, void *obj, int result);
void my_publish_callback(struct mosquitto *mosq, void *obj, int mid);
void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg);

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

// �õ�PKMemDB�Ķ˿ں�PKMemDB������
int GetPKMqttPort(unsigned short &nListenPort)
{
	nListenPort = DEFAULT_MQTT_PORT;
 
	string strConfigPath = PKComm::GetConfigPath();
	strConfigPath = strConfigPath + PK_OS_DIR_SEPARATOR + "pkmqtt.conf";
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
	}

	if (nListenPort <= 0)
		nListenPort = DEFAULT_MQTT_PORT;
	return 0;
}

/**
 *  ���캯��.
 *
 *  @param  -[in,out]  CDriver*  pDriver: [comment]
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
CMainTask::CMainTask()
{
	m_bStop = false;
	m_bStopped = false;
	m_nTagNum = 0;
	m_pMqttObj = NULL;
	m_nMqttListenPort = DEFAULT_MQTT_PORT;
	m_strMqttListenIp = "127.0.0.1";
}

/**
 *  ��������.
 *
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
CMainTask::~CMainTask()
{
	map<string, CNodeTag *>::iterator itMapName2Tag = m_mapName2Tag.begin();
	for(; itMapName2Tag != m_mapName2Tag.end(); itMapName2Tag ++){
		CNodeTag *pTag = itMapName2Tag->second;
		if(pTag != NULL)
		{	
			delete pTag;
			pTag = NULL;
		}
	}
	m_mapName2Tag.clear();
}

/**
 *  �麯�����̳���ACE_Task.
 *
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
int CMainTask::svc()
{	
	ACE_High_Res_Timer::global_scale_factor();

	int nRet = OnStart();
	while(!m_bStop)
	{
		PKComm::Sleep(DEFAULT_SLEEP_TIME * 2); 
	}

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE,"nodeserver MainTask exit normally!");
	OnStop();

	m_bStopped = true;
	return nRet;	
}

int CMainTask::OnStart()
{
	int nStatus = PK_SUCCESS;

	nStatus = RMAPI_Init();
	if (nStatus != PK_SUCCESS)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR,"��ʼ��RMAPIʧ��!");
	}

	//�������Ҫ����������״̬�㣬�磬��Ҫ�ȶ����Ѿ�����ĵ㣬��������е�
	InitMemDBTags(); // ��ʼ����ϵͳtag�㣬�����ֵ

	nStatus = LoadConfig(); // �������InitMemDBTags֮�󣬷���db1�ᱻ���ǵ�
	if (nStatus != PK_SUCCESS)
	{
		//��¼��־����ȡ�����ļ�ʧ��
		g_logger.LogMessage(PK_LOGLEVEL_ERROR,"please check database error tip!ret:%d!", nStatus);
		g_pPKNodeServer->UpdateServerStatus(-100,"configfile error");
		return -1;
	}
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE,"load config over, tag count:%d!", m_nTagNum);
	 
	StartMQTT();

	CONTROLPROCESS_TASK->Start();
	DATAPROC_TASK->Start(); 
	return PK_SUCCESS;
}

int CMainTask::StartMQTT()
{
	GetPKMqttPort(m_nMqttListenPort);
	m_pMqttObj = new CMqttImpl(m_strMqttListenIp.c_str(), m_nMqttListenPort, "eview-uppernode");
	int nRet = m_pMqttObj->mqttClient_Init(this);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "mqttClient_Init return failed,%d", nRet);
	}

	// �������
	CheckConnectToMqttServer();

	// ��������߳�ֻ�ܿ�����һ�ΰɣ����������ܷ��������ﱻ����Σ�
	m_pMqttObj->mqttClient_StartLoopWithNewThread();

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "MQTT Server IP:%s, �˿�:%d.", m_strMqttListenIp.c_str(), m_nMqttListenPort);
	return 0;
}


int CMainTask::CheckConnectToMqttServer()
{
	if (m_pMqttObj->m_bConnected)
		return 0;

	int nRet = m_pMqttObj->mqttClient_Connect(3000, my_connect_callback, my_disconnect_callback, my_publish_callback, my_message_callback);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "mqttClient_Connect return failed,%d", nRet);
		return nRet;
	}

	return nRet;
}

int CMainTask::InitMemDBTags()
{
	int nRet = 0;
	int nPKMemDBListenPort = PKMEMDB_LISTEN_PORT;
	string strPKMemDBPassword = PKMEMDB_ACCESS_PASSWORD;
	GetPKMemDBPort(nPKMemDBListenPort, strPKMemDBPassword);

	nRet = m_redisRW0.initialize(PK_LOCALHOST_IP, nPKMemDBListenPort, strPKMemDBPassword.c_str(), 0);
	if(nRet != PK_SUCCESS)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR,"DB0.Initialize failed,ret:%d",nRet);
	}
	//�ȶ������е�VARIABLE_TYPE_ACCUMULATE_TIME VARIABLE_TYPE_ACCUMULATE_COUNT VARIABLE_TYPE_MEM ����ֵ�������
	
	nRet = m_redisRW0.flushdb();
	
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "��ʼ�����DB0");

	nRet = m_redisRW1.initialize(PK_LOCALHOST_IP, nPKMemDBListenPort, strPKMemDBPassword.c_str(), 1);
	if(nRet != PK_SUCCESS)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR,"DB1(current alarmings) Initialize failed,ret:%d",nRet);
	}
	nRet = m_redisRW1.flushdb();
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "init to delete all current alarmings records in DB1(current alarmings)");

	nRet = m_redisRW2.initialize(PK_LOCALHOST_IP, nPKMemDBListenPort, strPKMemDBPassword.c_str(), 2);
	if (nRet != PK_SUCCESS)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "DB2(tag config) Initialize failed,ret:%d", nRet);
	}

	return 0;
}

int CMainTask::WriteTagConfigAndInitValToMemDB()
{
	int nRet = 0;

	// ��ʼ����ʱ��tag��ͱ���������������Ϣд��redis���ݿ�
	int nTmp = 0;
	map<string, CNodeTag *>::iterator itMapName2Tag = m_mapName2Tag.begin();
	for (itMapName2Tag; itMapName2Tag != m_mapName2Tag.end(); itMapName2Tag++)
	{
		CNodeTag *pDataTag = itMapName2Tag->second;
		nTmp++;
		//printf("%d\n", nTmp);
		Json::Value jsonTagConfig;
		string strTagConfigName = pDataTag->m_strName;
		jsonTagConfig["name"] = strTagConfigName;
		jsonTagConfig[JSONFIELD_DESC] = pDataTag->m_strDesc;
		jsonTagConfig["datatype"] = pDataTag->m_strDataType;
 		jsonTagConfig["labelid"] = pDataTag->m_strDataLen;   // Ҫ��labelname�� 
		jsonTagConfig["nodecode"] = pDataTag->m_pNode->m_strCode;
		jsonTagConfig["nodename"] = pDataTag->m_pNode->m_strName;

		string strConfigData = m_jsonWriter.write(jsonTagConfig); // .toStyledString();
		m_redisRW2.set(strTagConfigName.c_str(), strConfigData.c_str());

		char szTime[PK_HOSTNAMESTRING_MAXLEN] = { '\0' };
		PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));
		// ����ʱ���
		unsigned int nPackTimeStampSec = 0;
		unsigned int nPackTimeStampMSec = 0;
		nPackTimeStampSec = PKTimeHelper::GetHighResTime(&nPackTimeStampMSec);

		Json::Value jsonTagData;
		if (pDataTag->m_strInitValue.empty()) // ���û�г�ʼֵ����ôȡ ***  ����ȡ��ʼֵ
		{
			jsonTagData[JSONFIELD_VALUE] = TAG_QUALITY_INIT_STATE_STRING; //pDataTag->initVal.strVal;
			jsonTagData[JSONFIELD_QUALITY] = TAG_QUALITY_INIT_STATE;
		}
		else
		{
			jsonTagData[JSONFIELD_VALUE] = pDataTag->m_strInitValue;
			jsonTagData[JSONFIELD_QUALITY] = TAG_QUALITY_GOOD;
		}
		jsonTagData[JSONFIELD_TIME] = szTime;
		jsonTagData[JSONFIELD_MAXALARM_LEVEL] = 0;
		jsonTagData[JSONFIELD_ALM_UNACKCOUNT] = 0;
		jsonTagData[JSONFIELD_ALMINGCOUNT] = 0;
		string strTagData = m_jsonWriter.write(jsonTagData); // .toStyledString();
		m_redisRW0.set(pDataTag->m_strName.c_str(), strTagData.c_str());
	}

	m_redisRW0.commit();
	m_redisRW2.commit();

	return 0;
}

// ������ֹͣʱ
void CMainTask::OnStop()
{
	// ֹͣ�����߳� 
	CONTROLPROCESS_TASK->Stop();
	DATAPROC_TASK->Stop(); 

	m_redisRW0.finalize();
	m_redisRW1.finalize();
	m_redisRW2.finalize();
}
 
int CMainTask::Start()
{
	return activate(THR_NEW_LWP | THR_JOINABLE |THR_INHERIT_SCHED);
}

void CMainTask::Stop()
{
	m_bStop = true;
	int nWaitResult = wait();

	ACE_Time_Value tvSleep;
	tvSleep.msec(100);
	while(!m_bStopped)
		ACE_OS::sleep(tvSleep);
}

void CMainTask::ClearMemDBTags()
{

}

// �������ļ����غ����¼������е�����theTaskGroupList
int CMainTask::LoadConfig()
{
	int nRet = PK_SUCCESS;
	// ��ʼ�����ݿ�����
	CPKDbApi pkdbApi;
	nRet = pkdbApi.InitFromConfigFile("db.conf", "database");
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Init Database Failed:%d!(��ʼ�����ݿ�ʧ��)", nRet);
		pkdbApi.UnInitialize();
		return -2;
	}

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "success to init database, begin read config including(node,tag)");

	// ��ѯ���е��¼��ڵ㣨�����أ���Ϣ
	nRet = LoadInitNodeConfig(pkdbApi);
	if (nRet != 0)
		return nRet;

	// ��ѯ���еı���
	nRet = LoadInitTagConfig(pkdbApi);
	if (nRet != 0)
		return nRet;

	return nRet;
}

int CMainTask::LoadInitNodeConfig(CPKDbApi &pkdbApi)
{
	vector< vector<string> > vecRows;
	char szSql[2048] = { 0 };

	// ��ȡ���еķǱ��ؽڵ�
	sprintf(szSql, "select id,code,name,description from t_node_lower_list where enable is NULL or enable<>0 or enable=''" );
	string strError;
	int nRet = pkdbApi.SQLExecute(szSql, vecRows, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "��ȡ�ڵ�ʧ��, ����ֵ:%d�� SQL:%s, error:%s", nRet, szSql, strError.c_str());
		return nRet;
	} 

	for (int iNode = 0; iNode < vecRows.size(); iNode++)
	{
		vector<string> &row = vecRows[iNode];

		int iCol = 0;

		string strId = NULLASSTRING(row[iCol].c_str()); 
		iCol++;
		string strCode = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDesc = NULLASSTRING(row[iCol].c_str());
		iCol++;
		
		CNodeInfo *pNode = new CNodeInfo();
		pNode->m_strId = strId;
		pNode->m_strName = strName;
		pNode->m_strCode = strCode;
		pNode->m_strDesc = strDesc;
		m_mapId2Node[pNode->m_strId] = pNode;  // id-->node
		m_mapCode2Node[pNode->m_strCode] = pNode; // code-->node

		g_logger.LogMessage(PK_LOGLEVEL_INFO, "������һ��eview�ڵ�,id:%s, name:%s, code:%s, desc:%s. name����Ϊ����tag������ǰ׺", 
			strId.c_str(), strName.c_str(), strCode.c_str(), strDesc.c_str());
		row.clear();
	}
	
	if(vecRows.size() == 0)
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "�ҵ� 0 ���¼��ڵ�, SQL:%s", szSql);

	vecRows.clear(); 

	return 0;
}

int CMainTask::LoadInitTagConfig(CPKDbApi &pkdbApi)
{
	vector< vector<string> > vecRows;
	char szSql[2048] = { 0 };

	// ��ʹ����Ͷ���ģʽ��ʹ�ô�ͳ��ʽ
	sprintf(szSql, "select TAG.name as name,TAG.address,TAG.datatype,TAG.datalen,TAG.description,TAG.node_id from t_device_tag TAG \
	where (TAG.node_id is NOT NULL AND TAG.node_id<>'' AND TAG.node_id<>0) AND (TAG.node_id in (select id from t_node_lower_list where enable is NULL or enable=1 or enable=''))");
	string strError;
	int nRet = pkdbApi.SQLExecute(szSql, vecRows, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "��ѯ�����豸����ʧ��(�豸ģʽ��, ����ֵ:%d, SQL:%s, error:%s", nRet, szSql, strError.c_str());
	}
	else
	{
		for (int iTag = 0; iTag < vecRows.size(); iTag++)
		{
			vector<string> &row = vecRows[iTag];

			int iCol = 0;
			CNodeTag *pTag = new CNodeTag();
			string strTagName = NULLASSTRING(row[iCol].c_str());
			iCol++;
			pTag->m_strAddress = NULLASSTRING(row[iCol].c_str());
			iCol++;
			pTag->m_strDataType = NULLASSTRING(row[iCol].c_str());
			iCol++;
			pTag->m_strDataLen = NULLASSTRING(row[iCol].c_str());
			iCol++;
			pTag->m_strDesc = NULLASSTRING(row[iCol].c_str());
			iCol++;
			string strNodeId = NULLASSTRING(row[iCol].c_str());

			map<string, CNodeInfo *>::iterator itNode = m_mapId2Node.find(strNodeId);
			if(itNode == m_mapId2Node.end())
			{
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "����(name:%s)��node_id:%d, δ�ҵ���node_id�Ľڵ���Ϣ, SQL:%s", pTag->m_strName.c_str(), strNodeId.c_str(), szSql);
				continue;
			}

			pTag->m_pNode = itNode->second;
			// Ϊtag��������ǰ׺��node1.tag1
			if (!pTag->m_pNode->m_strName.empty())
				strTagName = pTag->m_pNode->m_strName + "." + strTagName;

			pTag->m_strName = strTagName;
			m_mapName2Tag[pTag->m_strName] = pTag;
			pTag->m_pNode->m_vecTags.push_back(pTag);
			row.clear();
		}
		vecRows.clear();
	}

	// ��ȡ��̳е����������.�������Խ�����ȡһ�������������豸�ı���ֵ��
	vecRows.clear();
	sprintf(szSql, "select OBJ.name as obj_name,CLSPROP.name as prop_name,CLSPROP.address,CLSPROP.datatype,CLSPROP.datalen,CLSPROP.description,CLS.node_id \
		from t_class_prop CLSPROP, t_class_list CLS, t_object_list OBJ \
	where CLSPROP.class_id = CLS.id and CLS.id = OBJ.class_id and (OBJ.enable is NULL or OBJ.enable<>0 or OBJ.enable='') \
	and (CLS.node_id is NOT NULL and CLS.node_id<>'' AND CLS.node_id<>0) AND(CLS.node_id in (select id from t_node_lower_list where enable is NULL or enable=1 or enable=''))");
	nRet = pkdbApi.SQLExecute(szSql, vecRows, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "��ѯ���ж���������Ե㣨����ģʽ��ʧ��, ����ֵ:%d SQL:%s, error:%s", nRet, szSql, strError.c_str());
	}
	else
	{
		for (int iTag = 0; iTag < vecRows.size(); iTag++)
		{
			vector<string> &row = vecRows[iTag];

			int iCol = 0;
			CNodeTag *pTag = new CNodeTag();
			string strObjName = NULLASSTRING(row[iCol].c_str());
			iCol++;
			string strPropName = NULLASSTRING(row[iCol].c_str());
			iCol++;
			pTag->m_strAddress = NULLASSTRING(row[iCol].c_str());
			iCol++;
			pTag->m_strDataType = NULLASSTRING(row[iCol].c_str());
			iCol++;
			pTag->m_strDataLen = NULLASSTRING(row[iCol].c_str());
			iCol++;
			pTag->m_strDesc = NULLASSTRING(row[iCol].c_str());
			iCol++;
			string strNodeId = NULLASSTRING(row[iCol].c_str());
			pTag->m_strName = strObjName + "." + strPropName;
			if (m_mapName2Tag.find(pTag->m_strName) != m_mapName2Tag.end())
			{
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "����(name:%s)�Ѵ���, SQL:%s", pTag->m_strName.c_str(), szSql);
				continue;
			}

			map<string, CNodeInfo *>::iterator itNode = m_mapId2Node.find(strNodeId);
			if (itNode == m_mapId2Node.end())
			{
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "����(name:%s)��node_id:%d, δ�ҵ���node_id�Ľڵ���Ϣ, SQL:%s", pTag->m_strName.c_str(), strNodeId.c_str(), szSql);
				continue;
			}

			pTag->m_pNode = itNode->second;
			m_mapName2Tag[pTag->m_strName] = pTag;
			pTag->m_pNode->m_vecTags.push_back(pTag);
			row.clear();
		}
		vecRows.clear();
	}
	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "��������, ��Ч�¼��ڵ㹲:%d��, ����������:%d��", m_mapId2Node.size(), m_mapName2Tag.size());

	for (map<string, CNodeInfo *>::iterator itNode = m_mapId2Node.begin(); itNode != m_mapId2Node.end(); itNode++)
	{
		CNodeInfo * pLowerNode = itNode->second;
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "�¼��ڵ�,id:%s,code:%s,name:%s, ����������:%d��", 
			pLowerNode->m_strId.c_str(), pLowerNode->m_strCode.c_str(), pLowerNode->m_strName.c_str(), pLowerNode->m_vecTags.size());
	}
	
	return PK_SUCCESS;
}

string CMainTask::Json2String(Json::Value &jsonVal)
{
	if (jsonVal.isNull())
		return "";

	string strRet = "";
	if (jsonVal.isString())
	{
		strRet = jsonVal.asString();
		return strRet;
	}

	if (jsonVal.isInt())
	{
		int nVal = jsonVal.asInt();
		char szTmp[32];
		sprintf(szTmp, "%d", nVal);
		strRet = szTmp;
		return strRet;
	}

	if (jsonVal.isDouble())
	{
		double dbVal = jsonVal.asDouble();
		char szTmp[32];
		sprintf(szTmp, "%f", dbVal);
		strRet = szTmp;
		return strRet;
	}

	if (jsonVal.isBool())
	{
		bool bVal = jsonVal.asBool();
		char szTmp[32];
		sprintf(szTmp, "%d", bVal ? 1 : 0);
		strRet = szTmp;
		return strRet;
	}

	return "";
}


// ��MQTT������������
void my_connect_callback(struct mosquitto *mosq, void *obj, int rc)
{
	CMainTask *pMainTask = (CMainTask *)obj;
	pMainTask->m_pMqttObj->m_bInit = true;
	pMainTask->m_pMqttObj->m_bConnected = true;

	g_logger.LogMessage(PK_LOGLEVEL_INFO, "MQTT Server connected!");

	string strTopicRealData = TOPIC_REALDATA_PREFIX;// ʵʱ���ݷ���ͨ��, peak/realdata/{gatewayID}
	strTopicRealData = strTopicRealData + "/+"; // ���е�ʵʱ����ͨ����
	string strTopicConfig = TOPIC_CONFIG_PREFIX; // ����ͨ��, peak/config/{gatewayID}
	strTopicConfig = strTopicConfig + "/+"; // ���е�ʵʱ����ͨ��

	// �ɹ����ӵ�ʱ����
	int mid;
	int nRet = pMainTask->m_pMqttObj->mqttClient_Sub(strTopicRealData.c_str(), &mid);
	if (nRet == 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "mqttClient_Sub(topic:%s) success", strTopicRealData.c_str());
	}
	else
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "mqttClient_Sub(topic:%s) failed, ret: %d", strTopicRealData.c_str(), nRet);
	}

	nRet = pMainTask->m_pMqttObj->mqttClient_Sub(strTopicConfig.c_str(), &mid);
	if (nRet == 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "mqttClient_Sub(topic:%s) success", strTopicConfig.c_str());
	}
	else
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "mqttClient_Sub(topic:%s) failed, ret: %d", strTopicConfig.c_str(), nRet);
	}
}

// ��MQTT����Ͽ�������
void my_disconnect_callback(struct mosquitto *mosq, void *obj, int result)
{
	// ֻ��ĳ���豸�Ͽ������� g_pMqttObj->m_bConnected = false;
	CMainTask *pMainTask = (CMainTask *)obj;
	pMainTask->m_pMqttObj->m_bConnected = false; // ��MQTT��������ӶϿ�
	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "MQTT Server Disconnected!");
}

//�����ص� ֻ�з����ɹ��Ż��������Ͽ�����֮�󲻻��������
//���ԣ�ÿ����һ����Ϣ���У������浽���ͻ������У�������ͳɹ��������ｫ��ɾ����
void my_publish_callback(struct mosquitto *mosq, void *obj, int mid)
{
}

#ifdef _WIN32
__inline void UTF8_2_Unicode(WCHAR* dst, const char * src)
{
	char* uchar = (char *)dst;

	uchar[1] = ((src[0] & 0x0F) << 4) + ((src[1] >> 2) & 0x0F);
	uchar[0] = ((src[1] & 0x03) << 6) + (src[2] & 0x3F);
}

__inline void Unicode_2_GB2312(char* dst, WCHAR uData)
{
	WideCharToMultiByte(CP_ACP, NULL, &uData, 1, dst, sizeof(unsigned short), NULL, NULL);
}

int Utf8ToAnsi(char * buf, int buf_len, const char * src, int src_len)
{
	if (src == NULL)
	{
		buf[0] = '\0';
		return 0;
	}

	if (0 == src_len)
		src_len = strlen(src);

	if (src_len == 0)
	{
		buf[0] = '\0';
		return 0;
	}

	int j = 0;
	for (int i = 0; i < src_len;)
	{
		if (j >= buf_len - 1)
			break;

		if (src[i] >= 0)
		{
			buf[j++] = src[i++];
		}
		else
		{
			WCHAR w_c = 0;
			UTF8_2_Unicode(&w_c, src + i);

			char tmp[4] = "";
			Unicode_2_GB2312(tmp, w_c);

			buf[j + 0] = tmp[0];
			buf[j + 1] = tmp[1];

			i += 3;
			j += 2;
		}
	}

	buf[j] = '\0';

	return j;
}

string Utf8ToGbk(const char *src_str)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, src_str, -1, NULL, 0);
	wchar_t* wszGBK = new wchar_t[len + 1];
	memset(wszGBK, 0, len * 2 + 2);
	MultiByteToWideChar(CP_UTF8, 0, src_str, -1, wszGBK, len);
	len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
	char* szGBK = new char[len + 1];
	memset(szGBK, 0, len + 1);
	WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);
	string strTemp(szGBK);
	if (wszGBK) delete[] wszGBK;
	if (szGBK) delete[] szGBK;
	return strTemp;
}
#else
string Utf8ToGbk(const char *src_str)
{
    string strTemp = src_str;
    return strTemp;
}

#endif

// Qualifier: ��������ͨ������Ϣ����ʱ���ڴ˴��������޸ĺ� ��Ϊ����Ϣ��һ��ר���߳�дredis
//��Ϣ���ͣ�
/*
(configͨ�������ڵ�ǰ�߳��жϴ���������߳�)
1��tag�汾��Ϣ��
2��tag����
(dataͨ�����׵�writetag�̴߳���)
3��tagֵ
*/
void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
	CMainTask *pMainTask = (CMainTask *)obj;
	//�����configͨ����
	pMainTask->m_pMqttObj->m_bConnected = true;
	string strTopic = msg->topic;
	if (!msg->payload || msg->payloadlen == 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "INVALID! ���յ���һ���¼��ڵ��ʵʱ������Ϣ, topic:%s, ����Ϊ:%d", strTopic.c_str(), msg->payloadlen);
		return;
	}

	char *szMsg = (char *)msg->payload;
	string strMsg = Utf8ToGbk(szMsg);;// PKCodingConv::Utf8ToAnsi(szMsg, msg->payloadlen);
	string strPartMsg;
	int nTotalMsgLen = strMsg.length();
	if (strMsg.length() > 200)
		strPartMsg = strMsg.substr(0, 200);
	else
		strPartMsg = strMsg;
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "���յ���һ���¼��ڵ��ʵʱ������Ϣ, topic:%s, ����:%d bytes, ��������:%s", strTopic.c_str(), nTotalMsgLen, strPartMsg.c_str());

	int nPos = strTopic.find(TOPIC_REALDATA_PREFIX);
	if (nPos != string::npos) // �ҵ���ʵʱ���ݵĿ�ͷ, ����Ϊ��ʵʱ����
	{
		Json::Value jsonNode;
		Json::Reader jsonReader;

		/*
		string strUtf8 = PKCodingConv::Utf8ToAnsi(szMsg, msg->payloadlen);
		wstring wstrMsg;
		wstrMsg.reserve(msg->payloadlen);
		for (int i = 0; i < msg->payloadlen; i++)
		{
			WCHAR w_c = 0;
			UTF8_2_Unicode(&w_c, szMsg + i);
			wstrMsg += w_c;
		}
		string strTmp2 = Utf8ToGbk(szMsg);
		string strTmp = PKCodingConv::Utf8ToAnsi(szMsg, msg->payloadlen);
		string strMsgAnsi;
		PKCodingConv::UnicodeToAnsi(wstrMsg, strMsgAnsi);
		*/
		if (!jsonReader.parse(strMsg.c_str(), jsonNode, false))
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "���յ���һ���������ص�ʵʱ������Ϣ, topic:%s, ������json��ʽ, ����:%s", strTopic.c_str(), strMsg.c_str());
			return;
		}

		Json::Value jsonGWID = jsonNode["id"];
		string strNodeCode = jsonGWID.asString();

		string strUpdateTime = "";
		Json::Value jsonUpdateTime = jsonNode["t"];
		if (!jsonUpdateTime.isNull())
			strUpdateTime = pMainTask->Json2String(jsonUpdateTime);

		map<string, CNodeInfo *>::iterator itNode = pMainTask->m_mapCode2Node.find(strNodeCode);
 		if (itNode == pMainTask->m_mapCode2Node.end())
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "���յ����ص�ʵʱ����, topic:%s, ����id:%s, δ�ҵ����õ��¼��ڵ�, ���������¼��ڵ�t_node_lower_list! %s", strTopic.c_str(), strNodeCode.c_str(), strMsg.c_str());
			return;
		}
		CNodeInfo *pNode = itNode->second;
		pNode->m_tvLastData = ACE_OS::gettimeofday();

		Json::Value jsonTagsData = jsonNode["data"];
		if (!jsonTagsData.isArray())
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "���յ����ص�ʵʱ����, topic:%s, data��������, �Ƿ�! ����:%s", strTopic.c_str(), strMsg.c_str());
			return;
		}
		if (!jsonTagsData.isArray())
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "���յ����ص�ʵʱ����, topic:%s, ����id:%s, data��������, ����! %s", strTopic.c_str(), strNodeCode.c_str(), strMsg.c_str());
			return;
		}

		// {"data":[{"n":"test1","ml":7,"q":0,"t":"2018-08-18 18:41:29.603","v":"3952"},....]
		int nTotalTagNum = jsonTagsData.size();
		int nInvalidTagNum = 0;

		DATAPROC_TASK->m_mutexTagDataCache.acquire(); // ������m_listTagDataCached����
		for (int i = 0; i < nTotalTagNum; i++)
		{
			Json::Value jsonOneTag = jsonTagsData[i];
			Json::Value jsonTagName = jsonOneTag["n"];
			if (jsonTagName.isNull())
			{
				nInvalidTagNum++;
				continue;
			}

			TAGDATA_JSONSTR tagDataJson;
			tagDataJson.strName = pMainTask->Json2String(jsonTagName);
			if (!pNode->m_strName.empty()) // �����Ƿ����ӽڵ����ƣ�
				tagDataJson.strName = pNode->m_strName + "." + tagDataJson.strName;

			jsonOneTag.removeMember("n"); // ɾ������
			tagDataJson.strVTQ = DATAPROC_TASK->m_jsonWriter.write(jsonOneTag); // ���ܸ���
 			DATAPROC_TASK->m_vecTagDataCached.push_back(tagDataJson);
		}
		DATAPROC_TASK->m_mutexTagDataCache.release();

		if (nInvalidTagNum == 0)
			g_logger.LogMessage(PK_LOGLEVEL_INFO, "����MQTT����, ������������, id:%s, ����:%d, ��Ч����:%d, ʱ���:%s", strNodeCode.c_str(), nTotalTagNum, nInvalidTagNum, strUpdateTime.c_str());
		else
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "����MQTT����, �зǷ�����, ������������, id:%s, �ܸ���:%d, ��Ч����:%d, �ѷ�����Ч����:%d �ɹ�, ʱ���:%s", strNodeCode.c_str(), nTotalTagNum, nInvalidTagNum, nTotalTagNum - nInvalidTagNum, strUpdateTime.c_str());
		return;
	}

	//���������ͨ������Ҫô�Ƿ��ص�MD5 Ҫô�Ƿ��ص�����������Ϣ
	nPos = strTopic.find(TOPIC_CONFIG_PREFIX);
	if (nPos != string::npos) // �ҵ�������Topic�Ŀ�ͷ, ����Ϊ������
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "���յ���һ������������Ϣ, topic:%s, ��ʱ����", strTopic.c_str());
		return;
	}
}
