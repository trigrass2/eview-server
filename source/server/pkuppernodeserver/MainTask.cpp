/**************************************************************
 *  Filename:    CtrlProcessTask.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 控制命令处理类.
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

// 得到PKMemDB的端口号PKMemDB的密码
int GetPKMqttPort(unsigned short &nListenPort)
{
	nListenPort = DEFAULT_MQTT_PORT;
 
	string strConfigPath = PKComm::GetConfigPath();
	strConfigPath = strConfigPath + PK_OS_DIR_SEPARATOR + "pkmqtt.conf";
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
	}

	if (nListenPort <= 0)
		nListenPort = DEFAULT_MQTT_PORT;
	return 0;
}

/**
 *  构造函数.
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
 *  析构函数.
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
 *  虚函数，继承自ACE_Task.
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
		g_logger.LogMessage(PK_LOGLEVEL_ERROR,"初始化RMAPI失败!");
	}

	//如果有需要继续计数的状态点，如，需要先读出已经保存的点，再清空所有点
	InitMemDBTags(); // 初始生成系统tag点，赋予初值

	nStatus = LoadConfig(); // 必须放在InitMemDBTags之后，否则db1会被覆盖掉
	if (nStatus != PK_SUCCESS)
	{
		//记录日志，读取配置文件失败
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

	// 检查连接
	CheckConnectToMqttServer();

	// 下面这个线程只能开启这一次吧？？？？不能放在连接里被开多次？
	m_pMqttObj->mqttClient_StartLoopWithNewThread();

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "MQTT Server IP:%s, 端口:%d.", m_strMqttListenIp.c_str(), m_nMqttListenPort);
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
	//先读出所有的VARIABLE_TYPE_ACCUMULATE_TIME VARIABLE_TYPE_ACCUMULATE_COUNT VARIABLE_TYPE_MEM 变量值，再清空
	
	nRet = m_redisRW0.flushdb();
	
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "初始化清除DB0");

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

	// 初始化的时候将tag点和报警点的相关配置信息写入redis数据库
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
 		jsonTagConfig["labelid"] = pDataTag->m_strDataLen;   // 要存labelname的 
		jsonTagConfig["nodecode"] = pDataTag->m_pNode->m_strCode;
		jsonTagConfig["nodename"] = pDataTag->m_pNode->m_strName;

		string strConfigData = m_jsonWriter.write(jsonTagConfig); // .toStyledString();
		m_redisRW2.set(strTagConfigName.c_str(), strConfigData.c_str());

		char szTime[PK_HOSTNAMESTRING_MAXLEN] = { '\0' };
		PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));
		// 放入时间戳
		unsigned int nPackTimeStampSec = 0;
		unsigned int nPackTimeStampMSec = 0;
		nPackTimeStampSec = PKTimeHelper::GetHighResTime(&nPackTimeStampMSec);

		Json::Value jsonTagData;
		if (pDataTag->m_strInitValue.empty()) // 如果没有初始值，那么取 ***  否则取初始值
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

// 本任务停止时
void CMainTask::OnStop()
{
	// 停止其他线程 
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

// 从配置文件加载和重新加载所有的配置theTaskGroupList
int CMainTask::LoadConfig()
{
	int nRet = PK_SUCCESS;
	// 初始化数据库连接
	CPKDbApi pkdbApi;
	nRet = pkdbApi.InitFromConfigFile("db.conf", "database");
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Init Database Failed:%d!(初始化数据库失败)", nRet);
		pkdbApi.UnInitialize();
		return -2;
	}

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "success to init database, begin read config including(node,tag)");

	// 查询所有的下级节点（如网关）信息
	nRet = LoadInitNodeConfig(pkdbApi);
	if (nRet != 0)
		return nRet;

	// 查询所有的变量
	nRet = LoadInitTagConfig(pkdbApi);
	if (nRet != 0)
		return nRet;

	return nRet;
}

int CMainTask::LoadInitNodeConfig(CPKDbApi &pkdbApi)
{
	vector< vector<string> > vecRows;
	char szSql[2048] = { 0 };

	// 读取所有的非本地节点
	sprintf(szSql, "select id,code,name,description from t_node_lower_list where enable is NULL or enable<>0 or enable=''" );
	string strError;
	int nRet = pkdbApi.SQLExecute(szSql, vecRows, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "读取节点失败, 返回值:%d， SQL:%s, error:%s", nRet, szSql, strError.c_str());
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

		g_logger.LogMessage(PK_LOGLEVEL_INFO, "增加了一个eview节点,id:%s, name:%s, code:%s, desc:%s. name将作为所有tag变量的前缀", 
			strId.c_str(), strName.c_str(), strCode.c_str(), strDesc.c_str());
		row.clear();
	}
	
	if(vecRows.size() == 0)
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "找到 0 个下级节点, SQL:%s", szSql);

	vecRows.clear(); 

	return 0;
}

int CMainTask::LoadInitTagConfig(CPKDbApi &pkdbApi)
{
	vector< vector<string> > vecRows;
	char szSql[2048] = { 0 };

	// 不使用类和对象模式，使用传统方式
	sprintf(szSql, "select TAG.name as name,TAG.address,TAG.datatype,TAG.datalen,TAG.description,TAG.node_id from t_device_tag TAG \
	where (TAG.node_id is NOT NULL AND TAG.node_id<>'' AND TAG.node_id<>0) AND (TAG.node_id in (select id from t_node_lower_list where enable is NULL or enable=1 or enable=''))");
	string strError;
	int nRet = pkdbApi.SQLExecute(szSql, vecRows, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "查询所有设备变量失败(设备模式）, 返回值:%d, SQL:%s, error:%s", nRet, szSql, strError.c_str());
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
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "变量(name:%s)的node_id:%d, 未找到该node_id的节点信息, SQL:%s", pTag->m_strName.c_str(), strNodeId.c_str(), szSql);
				continue;
			}

			pTag->m_pNode = itNode->second;
			// 为tag点增加上前缀！node1.tag1
			if (!pTag->m_pNode->m_strName.empty())
				strTagName = pTag->m_pNode->m_strName + "." + strTagName;

			pTag->m_strName = strTagName;
			m_mapName2Tag[pTag->m_strName] = pTag;
			pTag->m_pNode->m_vecTags.push_back(pTag);
			row.clear();
		}
		vecRows.clear();
	}

	// 读取类继承到对象的属性.对象属性仅仅读取一级变量（来自设备的变量值）
	vecRows.clear();
	sprintf(szSql, "select OBJ.name as obj_name,CLSPROP.name as prop_name,CLSPROP.address,CLSPROP.datatype,CLSPROP.datalen,CLSPROP.description,CLS.node_id \
		from t_class_prop CLSPROP, t_class_list CLS, t_object_list OBJ \
	where CLSPROP.class_id = CLS.id and CLS.id = OBJ.class_id and (OBJ.enable is NULL or OBJ.enable<>0 or OBJ.enable='') \
	and (CLS.node_id is NOT NULL and CLS.node_id<>'' AND CLS.node_id<>0) AND(CLS.node_id in (select id from t_node_lower_list where enable is NULL or enable=1 or enable=''))");
	nRet = pkdbApi.SQLExecute(szSql, vecRows, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "查询所有对象的类属性点（对象模式）失败, 返回值:%d SQL:%s, error:%s", nRet, szSql, strError.c_str());
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
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "变量(name:%s)已存在, SQL:%s", pTag->m_strName.c_str(), szSql);
				continue;
			}

			map<string, CNodeInfo *>::iterator itNode = m_mapId2Node.find(strNodeId);
			if (itNode == m_mapId2Node.end())
			{
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "变量(name:%s)的node_id:%d, 未找到该node_id的节点信息, SQL:%s", pTag->m_strName.c_str(), strNodeId.c_str(), szSql);
				continue;
			}

			pTag->m_pNode = itNode->second;
			m_mapName2Tag[pTag->m_strName] = pTag;
			pTag->m_pNode->m_vecTags.push_back(pTag);
			row.clear();
		}
		vecRows.clear();
	}
	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "加载配置, 有效下级节点共:%d个, 变量及属性:%d个", m_mapId2Node.size(), m_mapName2Tag.size());

	for (map<string, CNodeInfo *>::iterator itNode = m_mapId2Node.begin(); itNode != m_mapId2Node.end(); itNode++)
	{
		CNodeInfo * pLowerNode = itNode->second;
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "下级节点,id:%s,code:%s,name:%s, 变量及属性:%d个", 
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


// 和MQTT服务建立了连接
void my_connect_callback(struct mosquitto *mosq, void *obj, int rc)
{
	CMainTask *pMainTask = (CMainTask *)obj;
	pMainTask->m_pMqttObj->m_bInit = true;
	pMainTask->m_pMqttObj->m_bConnected = true;

	g_logger.LogMessage(PK_LOGLEVEL_INFO, "MQTT Server connected!");

	string strTopicRealData = TOPIC_REALDATA_PREFIX;// 实时数据发送通道, peak/realdata/{gatewayID}
	strTopicRealData = strTopicRealData + "/+"; // 所有的实时数据通道。
	string strTopicConfig = TOPIC_CONFIG_PREFIX; // 配置通道, peak/config/{gatewayID}
	strTopicConfig = strTopicConfig + "/+"; // 所有的实时数据通道

	// 成功连接的时候订阅
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

// 和MQTT服务断开了连接
void my_disconnect_callback(struct mosquitto *mosq, void *obj, int result)
{
	// 只是某个设备断开连接了 g_pMqttObj->m_bConnected = false;
	CMainTask *pMainTask = (CMainTask *)obj;
	pMainTask->m_pMqttObj->m_bConnected = false; // 和MQTT服务的连接断开
	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "MQTT Server Disconnected!");
}

//发布回调 只有发布成功才会进入这里，断开连接之后不会进入这里
//所以，每发布一个消息队列，都保存到发送缓存区中，如果发送成功，在这里将他删除；
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

// Qualifier: 接收来自通道的消息，暂时放在此处，测试修改后 改为抛消息到一个专门线程写redis
//消息类型：
/*
(config通道，可在当前线程判断处理，最好抛线程)
1、tag版本信息：
2、tag配置
(data通道，抛到writetag线程处理)
3、tag值
*/
void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
	CMainTask *pMainTask = (CMainTask *)obj;
	//如果是config通道，
	pMainTask->m_pMqttObj->m_bConnected = true;
	string strTopic = msg->topic;
	if (!msg->payload || msg->payloadlen == 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "INVALID! 接收到了一个下级节点的实时数据信息, topic:%s, 长度为:%d", strTopic.c_str(), msg->payloadlen);
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
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "接收到了一个下级节点的实时数据信息, topic:%s, 长度:%d bytes, 部分内容:%s", strTopic.c_str(), nTotalMsgLen, strPartMsg.c_str());

	int nPos = strTopic.find(TOPIC_REALDATA_PREFIX);
	if (nPos != string::npos) // 找到了实时数据的开头, 则认为是实时数据
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
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "接收到了一个来自网关的实时数据信息, topic:%s, 但不是json格式, 内容:%s", strTopic.c_str(), strMsg.c_str());
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
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "接收到网关的实时数据, topic:%s, 网关id:%s, 未找到配置的下级节点, 请先配置下级节点t_node_lower_list! %s", strTopic.c_str(), strNodeCode.c_str(), strMsg.c_str());
			return;
		}
		CNodeInfo *pNode = itNode->second;
		pNode->m_tvLastData = ACE_OS::gettimeofday();

		Json::Value jsonTagsData = jsonNode["data"];
		if (!jsonTagsData.isArray())
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "接收到网关的实时数据, topic:%s, data不是数组, 非法! 内容:%s", strTopic.c_str(), strMsg.c_str());
			return;
		}
		if (!jsonTagsData.isArray())
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "接收到网关的实时数据, topic:%s, 网关id:%s, data不是数组, 请检查! %s", strTopic.c_str(), strNodeCode.c_str(), strMsg.c_str());
			return;
		}

		// {"data":[{"n":"test1","ml":7,"q":0,"t":"2018-08-18 18:41:29.603","v":"3952"},....]
		int nTotalTagNum = jsonTagsData.size();
		int nInvalidTagNum = 0;

		DATAPROC_TASK->m_mutexTagDataCache.acquire(); // 对数据m_listTagDataCached加锁
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
			if (!pNode->m_strName.empty()) // 看看是否增加节点名称！
				tagDataJson.strName = pNode->m_strName + "." + tagDataJson.strName;

			jsonOneTag.removeMember("n"); // 删除名称
			tagDataJson.strVTQ = DATAPROC_TASK->m_jsonWriter.write(jsonOneTag); // 性能更高
 			DATAPROC_TASK->m_vecTagDataCached.push_back(tagDataJson);
		}
		DATAPROC_TASK->m_mutexTagDataCache.release();

		if (nInvalidTagNum == 0)
			g_logger.LogMessage(PK_LOGLEVEL_INFO, "解析MQTT数据, 放入待处理队列, id:%s, 个数:%d, 无效个数:%d, 时间戳:%s", strNodeCode.c_str(), nTotalTagNum, nInvalidTagNum, strUpdateTime.c_str());
		else
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "解析MQTT数据, 有非法数据, 放入待处理队列, id:%s, 总个数:%d, 无效个数:%d, 已发送有效个数:%d 成功, 时间戳:%s", strNodeCode.c_str(), nTotalTagNum, nInvalidTagNum, nTotalTagNum - nInvalidTagNum, strUpdateTime.c_str());
		return;
	}

	//如果是配置通道，则要么是返回的MD5 要么是返回的所有配置信息
	nPos = strTopic.find(TOPIC_CONFIG_PREFIX);
	if (nPos != string::npos) // 找到了配置Topic的开头, 则认为是配置
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "接收到了一个来自配置信息, topic:%s, 暂时忽略", strTopic.c_str());
		return;
	}
}
