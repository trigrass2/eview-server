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
#include "PKNodeServer.h"
#include "CtrlProcTask.h"
#include "DataProcTask.h"
#include "RedisFieldDefine.h"
#include "TagExprCalc.h"
#include "TagProcessTask.h"
#include "Alarm2DBTask.h"
#include "UserAuthTask.h"
#include "common/PKMiscHelper.h"
#include "common/eviewcomm_internal.h"
#include "shmqueue/ProcessQueue.h"

#include <string>
#include <vector>
using namespace std;

#define NULLASSTRING(x) x==NULL?"":x

extern CPKLog g_logger;
extern CPKNodeServer *		g_pPKNodeServer;
CProcessQueue *				g_pQueData2NodeSrv = NULL;

int CalcLHLimits(CDataTag *pTag);
#define  CHECK_NULL_RETURN_ERR(X)  { if(!X) return EC_ICV_DA_CONFIG_FILE_STRUCTURE_ERROR; }
#define	DEFAULT_SLEEP_TIME		100	

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

	m_pSimDevice = NULL;	// 模拟变量设备
	m_pMemDevice = NULL;	// 内存变量设备
	m_pSimDriver = NULL;
	m_pMemDriver = NULL;
	m_nVersion = VERSION_NODESERVER_INITIAL;
}

/**
 *  析构函数.
 *
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
CMainTask::~CMainTask()
{
	map<string, CDataTag *>::iterator itMapName2Tag = m_mapName2Tag.begin();
	for(; itMapName2Tag != m_mapName2Tag.end(); itMapName2Tag ++){
		CDataTag *pTag = itMapName2Tag->second;
		if(pTag != NULL)
		{
			pTag->vecAllAlarmTags.clear();
			pTag->vecCalcTags.clear();

			if (pTag->nTagType == VARIABLE_TYPE_DEVICE)
			{
				CDeviceTag *pCalcTag = (CDeviceTag *)pTag;
				delete pCalcTag;
			}
			else if (pTag->nTagType == VARIABLE_TYPE_CALC)
			{
				CCalcTag *pTagCalc = (CCalcTag *)pTag;
				delete pTagCalc;
			}
			else if (pTag->nTagType == VARIABLE_TYPE_SIMULATE)
			{
				CSimTag *pSimTag = (CSimTag *)pTag;
				delete pSimTag;
			}
			else if (pTag->nTagType == VARIABLE_TYPE_PREDEFINE)
			{
				CPredefTag *pPredefTag = (CPredefTag *)pTag;
				delete pPredefTag;
			}
			else if (pTag->nTagType == VARIABLE_TYPE_ACCUMULATE_TIME)
			{
				CTagAccumulateTime *pTagAcculturate = (CTagAccumulateTime *)pTag;
				delete pTagAcculturate;
			}
			if (pTag->nTagType == VARIABLE_TYPE_ACCUMULATE_COUNT)
			{
				CTagAccumulateCount *pTagAcculturate = (CTagAccumulateCount *)pTag;
				delete pTagAcculturate;
			}
			else
				delete pTag;
			pTag = NULL;
		}
	}
	m_mapName2Tag.clear();

	map<string, CAlarmTag *>::iterator itMapName2AlarmTAg = m_mapName2AlarmTag.begin();
	for(; itMapName2AlarmTAg != m_mapName2AlarmTag.end(); itMapName2AlarmTAg ++){
		CAlarmTag *pAlarmTag = itMapName2AlarmTAg->second;
		if(pAlarmTag != NULL)
		{
			delete pAlarmTag;
			pAlarmTag = NULL;
		}
	}
	m_mapName2AlarmTag.clear();

	map<string, CMyObject *>::iterator itObj = m_mapName2Object.begin();
	for (; itObj != m_mapName2Object.end(); itObj++){
		CMyObject *pObject = itObj->second;
		if (pObject != NULL)
		{
			delete pObject;
			pObject = NULL;
		}
	}
	m_mapName2Object.clear();

	map<string, CMyDriver *>::iterator itDriver = m_mapName2Driver.begin();
	for (; itDriver != m_mapName2Driver.end(); itDriver++){
		CMyDriver *pDriver = (CMyDriver *)itDriver->second;
		if (pDriver->m_nType == DRIVER_TYPE_PHYSICALDEVICE)
		{
			CMyDriverPhysicalDevice *pPhysicalDrv = (CMyDriverPhysicalDevice *)pDriver;
			CProcessQueue *pDrvShm = pPhysicalDrv->m_pDrvShm;
			if (pDrvShm != NULL)
			{
				delete pDrvShm;
				pDrvShm = NULL;
			}
		}
	}
	m_mapName2Driver.clear();
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
		ACE_Time_Value tvSleep;
		tvSleep.msec(DEFAULT_SLEEP_TIME); // 100ms
		ACE_OS::sleep(tvSleep);


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
		// g_pPKNodeServer->UpdateServerStatus(-100,"configfile error");
		return -1;
	}
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE,"load config over, tag count:%d!", m_nTagNum);

	CreateProcessQueue();

	USERAUTH_TASK->Start();
	ALARM2DB_TASK->Start();
	CONTROLPROCESS_TASK->Start();
	DATAPROC_TASK->Start();
	TAGPROC_TASK->Start();
	return PK_SUCCESS;
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
		g_logger.LogMessage(PK_LOGLEVEL_ERROR,"DB0.Initialize failed,ret:%d,%s:%d",nRet, PK_LOCALHOST_IP, nPKMemDBListenPort);
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

CPredefTag *CMainTask::AddPredefTag(string strTagName, int nTagDataType)
{
	CPredefTag *pPredefTag = new CPredefTag();
	pPredefTag->strName = strTagName;
	pPredefTag->nDataType = nTagDataType;
	if (nTagDataType == TAG_DT_INT32)
	{
		pPredefTag->nDataTypeClass = DATATYPE_CLASS_INTEGER;
		pPredefTag->strDataType = "int32";
		pPredefTag->nTagDataLen = 4;
	}
	else // TEXT
	{
		pPredefTag->nDataTypeClass = DATATYPE_CLASS_TEXT;
		pPredefTag->strDataType = "text";
		pPredefTag->nTagDataLen = 64;
	}
	m_mapName2PredefTag[pPredefTag->strName] = pPredefTag;
	m_mapName2Tag[pPredefTag->strName] = pPredefTag;
	return pPredefTag;
}

int CMainTask::AddPredefineTagToConfig()
{
	// TAG点数量和许可信息
	AddPredefTag(TAG_SYSTEM_TAG_COUNT, TAG_DT_INT32);
	AddPredefTag(TAG_SYSTEM_LICENSE_EXPIRETIME, TAG_DT_TEXT);

	AddPredefTag(TAG_SYSTEM_MEMORY, TAG_DT_INT32);
	AddPredefTag(TAG_SYSTEM_CPU, TAG_DT_INT32);
	AddPredefTag(TAG_SYSTEM_TIME, TAG_DT_INT32);

	AddPredefTag(TAGNAME_SYSTEM_ALARM, TAG_DT_INT32);

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "已经为%d个报警类型增加了同名对象!", m_mapName2AlarmType.size());
	for (map<string, CAlarmType *>::iterator it = m_mapName2AlarmType.begin(); it != m_mapName2AlarmType.end(); it++)
	{
		CAlarmType *pAlarmType = it->second;
		string strAlarmTypeTagName = "alarmtype.";
		strAlarmTypeTagName += it->first;
		AddPredefTag(strAlarmTypeTagName, TAG_DT_INT32);
	}
	
	for (map<string, CDataTag *>::iterator itTag = m_mapName2Tag.begin(); itTag != m_mapName2Tag.end(); itTag++)
	{
		CDataTag *pTag = itTag->second;
		
		// 为每个TAG增加配置了报警类型属性
		for (unsigned int i = 0; i < pTag->vecAllAlarmTags.size(); i++)
		{
			CAlarmTag *pAlarmTag = pTag->vecAllAlarmTags[i];
			string strAlarmTypeName = pAlarmTag->strAlarmTypeName;
			if (pTag->mapAlarmTypeName2AlarmInfo.find(strAlarmTypeName) != pTag->mapAlarmTypeName2AlarmInfo.end())
				continue;
			CAlarmInfo *pNewAlarmInfo = new CAlarmInfo();
			pTag->mapAlarmTypeName2AlarmInfo[strAlarmTypeName] = pNewAlarmInfo;
		}
		//for (map<string, CAlarmType *>::iterator it = m_mapName2AlarmType.begin(); it != m_mapName2AlarmType.end(); it++)
		//{
		//	string strAlarmTypeName = it->first;
		//	CAlarmType *pAlarmType = it->second;
		//	CAlarmType *pNewAlarmType = new CAlarmType();
		//	*pNewAlarmType = *pAlarmType;
		//	pTag->mapName2AlarmType[strAlarmTypeName] = pNewAlarmType;
		//}
	}

	return 0;
}

int CMainTask::WriteTagConfigAndInitValToMemDB()
{
	int nRet = 0;

	// 初始化的时候将tag点和报警点的相关配置信息写入redis数据库
	int nTmp = 0;
	map<string, CDataTag *>::iterator itMapName2Tag = m_mapName2Tag.begin();
	for (itMapName2Tag; itMapName2Tag != m_mapName2Tag.end(); itMapName2Tag++)
	{
		CDataTag *pDataTag = itMapName2Tag->second;
		nTmp++;
		//printf("%d\n", nTmp);
		Json::Value jsonTagConfig;
		string strTagConfigName = pDataTag->strName;
		jsonTagConfig["name"] = strTagConfigName;
		jsonTagConfig[JSONFIELD_DESC] = pDataTag->strDesc;
		jsonTagConfig["datatype"] = pDataTag->strDataType;
		jsonTagConfig["subsys"] = pDataTag->strSubsysName;
		jsonTagConfig["pollrate"] = pDataTag->nPollrateMS;
		jsonTagConfig["labelid"] = pDataTag->nLabelID;   // 要存labelname的
		jsonTagConfig["hisdata_interval"] = pDataTag->strHisDataInterval;
		jsonTagConfig["trenddata_interval"] = pDataTag->strTrendDataInterval;
		jsonTagConfig["enable_control"] = pDataTag->bEnableControl;
		jsonTagConfig["param"] = pDataTag->strParam;

		// 设备变量和模拟设备具有地址
		if (pDataTag->nTagType == VARIABLE_TYPE_DEVICE)
		{
			CDeviceTag *pDeviceTag = (CDeviceTag *)pDataTag;
			string strDeviceName = "";
			if (pDeviceTag->pDevice && pDeviceTag->pDevice->m_pDriver)
				jsonTagConfig["driver"] = pDeviceTag->pDevice->m_pDriver->m_strName;
			else
				jsonTagConfig["driver"] = "未配置";

			if (pDeviceTag->pDevice)
				jsonTagConfig["device"] = pDeviceTag->pDevice->m_strName;
			else
				jsonTagConfig["device"] = "未配置";

			jsonTagConfig["address"] = pDeviceTag->strAddress;
			jsonTagConfig["tagtype"] = "device";
		}
		else if (pDataTag->nTagType == VARIABLE_TYPE_SIMULATE)
		{
			CSimTag *pTag = (CSimTag *)pDataTag;
			jsonTagConfig["driver"] = pTag->pDevice->m_pDriver->m_strName;
			jsonTagConfig["device"] = pTag->pDevice->m_strName;
			jsonTagConfig["address"] = pTag->strAddress;
			jsonTagConfig["tagtype"] = "simulate";
		}
		else if (pDataTag->nTagType == VARIABLE_TYPE_MEMVAR)
		{
			CMemTag *pMemTag = (CMemTag *)pDataTag;
			jsonTagConfig["tagtype"] = "memory";
		}
		else if (pDataTag->nTagType == VARIABLE_TYPE_PREDEFINE)
		{
			CPredefTag *pTag = (CPredefTag *)pDataTag;
			jsonTagConfig["tagtype"] = "predefine";
		}
		else if (pDataTag->nTagType == VARIABLE_TYPE_CALC)
		{
			CCalcTag *pCalcTag = (CCalcTag *)pDataTag;
			jsonTagConfig["address"] = pCalcTag->strAddress;
			jsonTagConfig["tagtype"] = "calc";
		}
		else if (pDataTag->nTagType == VARIABLE_TYPE_ACCUMULATE_TIME)
		{
			CCalcTag *pCalcTag = (CCalcTag *)pDataTag;
			jsonTagConfig["address"] = pCalcTag->strAddress;
			jsonTagConfig["tagtype"] = "accutime";
		}
		else if (pDataTag->nTagType == VARIABLE_TYPE_ACCUMULATE_COUNT)
		{
			CCalcTag *pCalcTag = (CCalcTag *)pDataTag;
			jsonTagConfig["address"] = pCalcTag->strAddress;
			jsonTagConfig["tagtype"] = "accucount";
		}
		else
		{
			CDataTag *pTag = (CDataTag *)pDataTag;
			jsonTagConfig["tagtype"] = "unknown";
		}

		for (unsigned int i = 0; i < pDataTag->vecAllAlarmTags.size(); i++)
		{
			CAlarmTag *pAlarmTag = pDataTag->vecAllAlarmTags[i];
			char szAlarmName[256] = {0};
			PKStringHelper::Snprintf(szAlarmName, sizeof(szAlarmName), "alarm_%s[%d]", pAlarmTag->strJudgeMethod.c_str(), i + 1); // 增加报警序号，防止同样的报警类型重复
			string strAlarmName = szAlarmName;
			string strTmp = "alarm_" + pAlarmTag->strJudgeMethod + "_enable";
			jsonTagConfig[strAlarmName + ".enable"] = pAlarmTag->bEnable;

			string strThresholdDesc;
			pAlarmTag->GetThresholdAsString(strThresholdDesc);
			jsonTagConfig[strAlarmName + ".threshold"] = strThresholdDesc;// pAlarmTag->strThreshold;
		}

		string strConfigData = m_jsonWriter.write(jsonTagConfig); // .toStyledString();
		m_redisRW2.set(strTagConfigName.c_str(), strConfigData.c_str());

		char szTime[PK_HOSTNAMESTRING_MAXLEN] = { '\0' };
		PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));
		// 放入时间戳
		unsigned int nPackTimeStampSec = 0;
		unsigned int nPackTimeStampMSec = 0;
		nPackTimeStampSec = PKTimeHelper::GetHighResTime(&nPackTimeStampMSec);

		Json::Value jsonTagData;
		if (pDataTag->strInitValue.empty()) // 如果没有初始值，那么取 ***  否则取初始值
		{
			jsonTagData[JSONFIELD_VALUE] = TAG_QUALITY_INIT_STATE_STRING; //pDataTag->initVal.strVal;
			jsonTagData[JSONFIELD_QUALITY] = TAG_QUALITY_INIT_STATE;
		}
		else
		{
			jsonTagData[JSONFIELD_VALUE] = pDataTag->strInitValue;
			jsonTagData[JSONFIELD_QUALITY] = TAG_QUALITY_GOOD;
		}
		jsonTagData[JSONFIELD_TIME] = szTime;
		jsonTagData[JSONFIELD_MAXALARM_LEVEL] = 0;
		jsonTagData[JSONFIELD_ALM_UNACKCOUNT] = 0;
		jsonTagData[JSONFIELD_ALMINGCOUNT] = 0;
		string strTagData = m_jsonWriter.write(jsonTagData); // .toStyledString();
		m_redisRW0.set(pDataTag->strName.c_str(), strTagData.c_str());
	}

	// 将对象的初始化信息发送到pkcserver进行初始化
	Json::Value jsonListObjects;
	map<string, CMyObject *>::iterator itObject = m_mapName2Object.begin();
	for (itObject; itObject != m_mapName2Object.end(); itObject++)
	{
		Json::Value jsonObject;
		CMyObject *pMyObject = itObject->second;

		pMyObject->UpdateToConfigMemDB(m_jsonWriter);
		pMyObject->UpdateToDataMemDB(m_jsonWriter);
	}

	//
	DATAPROC_TASK->m_eaProcessor.WriteSystemAlarmInfo2MemDB();
	//m_redisRW0.commit();
	m_redisRW2.commit();

	return 0;
}

// 本任务停止时
void CMainTask::OnStop()
{
	// 停止其他线程
	TAGPROC_TASK->Stop();
	CONTROLPROCESS_TASK->Stop();
	DATAPROC_TASK->Stop();
	ALARM2DB_TASK->Stop();
	USERAUTH_TASK->Stop();

	m_redisRW0.finalize();
	m_redisRW1.finalize();
	m_redisRW2.finalize();

	if (g_pQueData2NodeSrv)
		delete g_pQueData2NodeSrv;
	g_pQueData2NodeSrv = NULL;

}

int CMainTask::LoadDriverConfig(CPKEviewDbApi &PKEviewDbApi)
{
	int nRet = PK_SUCCESS;
		
	// 查询指定的驱动列表
	long lQueryId = -1;
	char szSql[2048] = { 0 };

	// 驱动信息,禁用的驱动不再读取
	char szTmp[4096] = { 0 };
	char szSqlPrefix[2048] = { 0 };
	sprintf(szSql, "SELECT distinct DRV.id as id, DRV.name as name, DRV.modulename as modulename,DRV.description as description, DRV.platform as platform, DRV.enable\
				FROM t_device_driver DRV,t_device_list DEV WHERE (DEV.enable is null or DEV.enable<>0) and DEV.driver_id=DRV.id and (DRV.enable is null or DRV.enable<>0)");
	if (m_nVersion >= VERSION_NODESERVER_SUPPORT_MULTINODE) // >= 1
		strcat(szSql, " AND (DRV.node_id is NULL or DRV.node_id=0)");
 
	// id为0的是user_vars, 是不需要定义的
	// 读取非禁用和非模拟的驱动
	vector<vector<string> > vecRow;
	//sprintf(szSql, "%s and ((A.simulate is null or A.simulate=0) and (B.simulate is null or B.simulate = 0))", szSqlPrefix);
	string strError;
	nRet = PKEviewDbApi.SQLExecute(szSql, vecRow, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "[LoadDriverConfig] Query Database failed:%s!(查询数据库失败), error:%s, 程序将无法继续执行，请处理...", szSql, strError.c_str());
		return -2;
	}

	// 非禁用和非模拟的设备信息
	for (int iTab = 0; iTab < vecRow.size(); iTab++)
	{
		vector<string> &row = vecRow[iTab];
		int iCol = 0;
		string strDriverId = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDriverName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDriverModuleName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDriverDesc = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDriverSupportPlatform = NULLASSTRING(row[iCol].c_str());
		iCol++;

		map<string, CMyDriver *>::iterator itDrv = m_mapName2Driver.find(strDriverName);
		if (itDrv != m_mapName2Driver.end())
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "driver name 重复了:%s!", strDriverName.c_str());
			continue;
		}

		CMyDriverPhysicalDevice *pDriver = new CMyDriverPhysicalDevice();
		pDriver->m_nId = ::atoi(strDriverId.c_str());
		pDriver->m_strDesc = strDriverDesc;
		pDriver->m_strModuleName = strDriverModuleName;
		pDriver->m_strName = strDriverName;
		m_mapName2Driver[strDriverName] = pDriver;
	}

	// 增加模拟驱动simdrv。在这里增加保证仅仅增加1次
	m_pSimDriver = new CMyDriverSimulate();
	m_mapName2Driver[m_pSimDriver->m_strName] = m_pSimDriver;

	// 增加模拟驱动simdrv。在这里增加保证仅仅增加1次
	m_pMemDriver = new CMyDriverMemory();
	m_mapName2Driver[m_pMemDriver->m_strName] = m_pMemDriver;

	m_nTagNum += m_mapName2Driver.size();
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "available driver count:%d!", m_mapName2Driver.size());

	return 0;
}

// 返回map<string, string> mapDeviceName2DeviceId
int CMainTask::LoadTagConfig(CPKEviewDbApi &PKEviewDbApi)
{
	int nRet = PK_SUCCESS;

	// 查询tag点
	long lQueryId = -1;
	char szSql[2048] = { 0 };
	vector<vector<string> > vecRow;

	int nTagNum = 0;
	sprintf(szSql, "SELECT TAG.id AS tagid,\
				  TAG.name AS tagname,TAG.cname AS cname,TAG.description AS description,TAG.address AS address,TAG.device_id AS deviceid,DEV.name AS devicename,DRV.name AS drivername,TAG.datatype AS datatype,\
				TAG.datalen AS datalen,LAB.id AS labelid,TAG.subsys_id AS subsysid,SYS.name AS subsysname,\
				TAG.initvalue AS initvalue,TAG.enable_control AS enablecontrol,DEV.simulate AS devsim,	TAG.pollrateMS,TAG.byteorder AS byteorder,\
				TAG.enable_range,TAG.rawmin,TAG.rawmax,TAG.outmin,TAG.outmax \
				FROM t_device_tag TAG\
				LEFT JOIN t_subsys_list SYS ON TAG.subsys_id = SYS.id\
				LEFT JOIN t_tag_value_dict LAB ON TAG.label_id = LAB.id\
				LEFT JOIN t_device_list DEV on TAG.device_id = DEV.id\
				LEFT JOIN t_device_driver DRV on DEV.driver_id = DRV.id\
				where(DEV.enable IS NULL OR DEV.enable <> 0 or DEV.enable='') AND (DRV.enable is NULL or DRV.enable <>0 or DRV.enable='')");
	if (m_nVersion >= VERSION_NODESERVER_SUPPORT_MULTINODE)
		strcat(szSql, " AND(TAG.node_id is NULL or TAG.node_id='' or TAG.node_id = 0) AND(DRV.node_id is NULL or DRV.node_id='' or DRV.node_id = 0)");

	string strError;
	nRet = PKEviewDbApi.SQLExecute(szSql, vecRow, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "fail to query database:%s, return:%d, error:%s", szSql, nRet, strError.c_str());
		return -2;
	}

	for (int iTab = 0; iTab < vecRow.size(); iTab++)
	{
		vector<string> &row = vecRow[iTab];
		int iCol = 0;
		string strTagId = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strTagName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strTagCName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDeccription = NULLASSTRING(row[iCol].c_str());
		iCol++;

		string strAddress = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDeviceID = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDeviceName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDriverName = NULLASSTRING(row[iCol].c_str());
		iCol++;

		string strTagDataType = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strTagDataLen = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strLableID = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strSubsysId = NULLASSTRING(row[iCol].c_str());
		iCol++;

		string strSubsysName = NULLASSTRING(row[iCol].c_str());
		iCol++;

		string strInitValue = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strEnableControl = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDevSimulate = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strPollrateMS = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strByteOrder = NULLASSTRING(row[iCol].c_str());
		iCol++;

		string strEnableRange = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strRawMin = NULLASSTRING(row[iCol].c_str());
		iCol++;		
		string strRawMax = NULLASSTRING(row[iCol].c_str());
		iCol++;		
		string strOutMin = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strOutMax = NULLASSTRING(row[iCol].c_str());
		iCol++;
		
		int nDeviceId = atoi(strDeviceID.c_str());
		// 判断tag是不是模拟的
		bool bSimulateTag = false;
		if (strDevSimulate.length() > 0)
			bSimulateTag = true;

		CDataTag *pDataTag = NULL;
		//根据表达式计算
		if (nDeviceId == VARIABLE_TYPE_CALC)	// 根据device_id进行判断，如果是二次变量点
		{
			pDataTag = new CCalcTag();
			pDataTag->strName = strTagName;
			pDataTag->nTagType = VARIABLE_TYPE_CALC;
			pDataTag->pDevice = m_pMemDevice;	// 属于内存变量
			m_mapName2CalcTag[pDataTag->strName] = (CCalcTag*)pDataTag; // 加入表达式
			((CCalcTag *)pDataTag)->strAddress = strAddress;
		}
		//累计变量满足条件的时间
		else if (nDeviceId == VARIABLE_TYPE_ACCUMULATE_TIME)	// 根据device_id进行判断，如果是累计时间变量点
		{
			pDataTag = new CTagAccumulateTime();
			pDataTag->strName = strTagName;
			pDataTag->nTagType = VARIABLE_TYPE_ACCUMULATE_TIME;
			pDataTag->pDevice = m_pMemDevice;	// 属于内存变量
			((CTagAccumulateTime *)pDataTag)->strAddress = strAddress;
			m_mapName2CalcTag[pDataTag->strName] = (CCalcTag*)pDataTag; // 加入表达式
			m_mapName2AccumlateTime[pDataTag->strName] = (CTagAccumulateTime*)pDataTag; // 加入表达式
		}
		//累计变量满足条件的次数
		else if (nDeviceId == VARIABLE_TYPE_ACCUMULATE_COUNT)	// 根据device_id进行判断，如果是累计次数变量点
		{
			pDataTag = new CTagAccumulateCount();
			pDataTag->strName = strTagName;
			pDataTag->nTagType = VARIABLE_TYPE_ACCUMULATE_COUNT;
			pDataTag->pDevice = m_pMemDevice;	// 属于内存变量
			((CTagAccumulateCount *)pDataTag)->strAddress = strAddress;
			m_mapName2CalcTag[pDataTag->strName] = (CCalcTag*)pDataTag; // 加入表达式
			m_mapName2AccumlateCount[pDataTag->strName] = (CTagAccumulateCount*)pDataTag; // 加入表达式
		}
		//累计变量满足条件的次数
		else if (nDeviceId == VARIABLE_TYPE_MEMVAR || PKStringHelper::StriCmp(strDriverName.c_str(), DRIVER_NAME_MEMORY) == 0)	// 根据device_id进行判断，如果是累计次数变量点
		{
			pDataTag = new CMemTag();
			pDataTag->strName = strTagName;
			pDataTag->nTagType = VARIABLE_TYPE_MEMVAR;
			pDataTag->pDevice = m_pMemDevice;	// 属于内存变量
			m_mapName2MemTag[pDataTag->strName] = (CMemTag *)pDataTag;
		}
		//累计变量满足条件的次数
		else if (nDeviceId == VARIABLE_TYPE_SIMULATE || bSimulateTag)	// 根据device_id进行判断，如果是累计次数变量点
		{
			pDataTag = new CSimTag();		
			pDataTag->strName = strTagName;
			pDataTag->nTagType = VARIABLE_TYPE_SIMULATE;
			pDataTag->pDevice = m_pSimDevice;	// 属于模拟变量
			((CSimTag *)pDataTag)->strAddress = strAddress;	
			((CSimTag *)pDataTag)->m_strDeviceSimValue = strDevSimulate;
			m_mapName2SimTag[pDataTag->strName] = (CSimTag *)pDataTag;
		}
		else	// 如果是一次设备变量点
		{
			map<string, CMyDevice*>::iterator itDev = m_mapName2Device.find(strDeviceName);
			if (itDev == m_mapName2Device.end())
			{
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "变量:%s, 设备名称:%s, 未找到对应的设备, 系统共加载设备个数:%d", strTagName.c_str(), strDeviceName.c_str(), m_mapName2Device.size());
				continue;
			}
			CMyDevice *pDevice = itDev->second;

			CDeviceTag *pDeviceTag = new CDeviceTag();
			pDataTag = pDeviceTag;
			pDataTag->nTagType = VARIABLE_TYPE_DEVICE;
			pDataTag->strName = strTagName;
			pDeviceTag->strAddress = strAddress;
			pDeviceTag->pDevice = pDevice;
			m_mapName2DeviceTag[pDataTag->strName] = pDeviceTag;

			// 将tag点增加到设备map中，以便于在驱动断开时，可以设置所有变量质量为BAD
			pDevice->m_vecDeviceTag.push_back(pDeviceTag);
		}

		pDataTag->nSubsysID = atoi(strSubsysId.c_str());
		pDataTag->nLabelID = atoi(strLableID.c_str());
		pDataTag->strDataType = strTagDataType;
		pDataTag->strDesc = strDeccription;
		pDataTag->strSubsysName = strSubsysName;
		pDataTag->strInitValue = strInitValue;
		pDataTag->nPollrateMS = ::atoi(strPollrateMS.c_str());

		int nDataType;
		int nDataTypeLenBits;
		PKMiscHelper::GetTagDataTypeAndLen(pDataTag->strDataType.c_str(), strTagDataLen.c_str(), &nDataType, &nDataTypeLenBits);
		if (nDataType == TAG_DT_UNKNOWN)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "tag:%s datatype:%s not supported, assume int32!!", pDataTag->strName.c_str(), pDataTag->strDataType.c_str());
			nDataType = TAG_DT_INT32;
			nDataTypeLenBits = 32; // 32位4个字节
		}
		pDataTag->nTagDataLen = (nDataTypeLenBits + 7)/8; // 字节个数
		pDataTag->nDataType = nDataType;
		pDataTag->nDataTypeClass = CDataTag::GetDataTypeClass(nDataType);
		pDataTag->CalcRangeParam(strEnableRange, strRawMin, strRawMax, strOutMin, strOutMax);

		m_mapName2Tag[pDataTag->strName] = pDataTag;

		nTagNum++;
	}
	m_nTagNum += nTagNum;
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "readed tag count:%d!(读取到tag点的个数，设备模式变量)", nTagNum);

	// 再加载历史数据配置。放这里是防止老的数据库没有此属性也能继续向下进行
	LoadTag_HisDataConfig(PKEviewDbApi);

	return 0;
}


// 返回map<string, string> mapDeviceName2DeviceId
int CMainTask::LoadTag_HisDataConfig(CPKEviewDbApi &PKEviewDbApi)
{
	int nRet = PK_SUCCESS;

	// 查询tag点
	long lQueryId = -1;
	char szSql[2048] = { 0 };
	vector<vector<string> > vecRow;

	int nTagNum = 0;
	sprintf(szSql, "SELECT TAG.id AS tagid,TAG.name AS tagname,TAG.hisdata_interval AS hisdata_interval,TAG.trenddata_interval AS trenddata_interval\
		FROM t_device_tag TAG\
		LEFT JOIN t_subsys_list SYS ON TAG.subsys_id = SYS.id\
		LEFT JOIN t_tag_value_dict LAB ON TAG.label_id = LAB.id\
		LEFT JOIN t_device_list DEV on TAG.device_id = DEV.id\
		LEFT JOIN t_device_driver DRV on DEV.driver_id = DRV.id\
	where(DEV.enable IS NULL OR DEV.enable <> 0)");
	string strError;
	nRet = PKEviewDbApi.SQLExecute(szSql, vecRow, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "查询设备模式下每个tag的历史数据和趋势数据属性失败, 但应该不影响使用:%s, return:%d, error:%s", szSql, nRet, strError.c_str());
		return -2;
	}

	for (int iTab = 0; iTab < vecRow.size(); iTab++)
	{
		vector<string> &row = vecRow[iTab];
		int iCol = 0;
		string strTagId = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strTagName = NULLASSTRING(row[iCol].c_str());
		iCol++;

		string strHisDataInterval = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strTrendDataInterval = NULLASSTRING(row[iCol].c_str());
		iCol++;

		map<string, CDataTag *>::iterator itTag = m_mapName2Tag.find(strTagName);
		if (itTag != m_mapName2Tag.end())
		{
			CDataTag *pTag = itTag->second;
			pTag->strHisDataInterval = strHisDataInterval;
			pTag->strTrendDataInterval = strTrendDataInterval;
		}
	}

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "readed tag hisdata count:%d!(读取到tag点历史信息的个数，设备模式)", nTagNum);

	return 0;
}

// 返回map<string, string> mapDeviceName2DeviceId
int CMainTask::LoadDeviceConfig(CPKEviewDbApi &PKEviewDbApi)
{
	int nRet = PK_SUCCESS;

	// 查询tag点
	long lQueryId = -1;
	char szSql[2048] = { 0 };
	vector<vector<string> > vecRow;

	int nTagNum = 0;
	sprintf(szSql, "select DEV.id as device_id,DEV.name as device_name,DEV.simulate,DEV.enable as enable,DEV.description as description,\
		DEV.driver_id as driver_id, DRV.name as driver_name,\
		conntype, connparam, conntimeout, taskno, param1, param2, param3, param4\
		from t_device_list DEV LEFT JOIN t_device_driver DRV on DEV.driver_id = DRV.id  where (DEV.enable is null or DEV.enable<>0)");
	if (m_nVersion >= VERSION_NODESERVER_SUPPORT_MULTINODE)
		strcat(szSql, " AND (DRV.node_id is NULL or DRV.node_id='' or DRV.node_id=0)");

	string strError; 
	nRet = PKEviewDbApi.SQLExecute(szSql, vecRow, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "(LoadDeviceConfig) fail to query database:%s, return:%d, error:%s, 程序将无法继续执行，请处理...", szSql, nRet, strError.c_str());
		return -2;
	}

	m_mapName2Device.clear();
	for (int iTab = 0; iTab < vecRow.size(); iTab++)
	{
		vector<string> &row = vecRow[iTab];
		int iCol = 0;
		string strDeviceID = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDeviceName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strSimulate = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDisable = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDescription = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDriverId = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDriverName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strConnType = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strConnParam = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strConnTimeout = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strTaskNo = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strParam1 = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strParam2 = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strParam3 = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strParam4 = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strProjectId = "";

		map<string, CMyDevice *>::iterator itDev = m_mapName2Device.find(strDeviceName);
		if (itDev != m_mapName2Device.end()) // 不存在该设备，增加之
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "(LoadDeviceConfig) device:%s 重复了,当前已有%d个设备!", strDeviceName.c_str(), m_mapName2Device.size());
			continue;
		}
		if (strDriverName.empty())
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "(LoadDeviceConfig) device:%s 设备名未配置驱动. 模拟变量和内存变量不需要配置驱动!!", strDeviceName.c_str());
			continue;
		}

		CMyDriver *pDriver = NULL;
		map<string, CMyDriver *>::iterator itDrv = m_mapName2Driver.find(strDriverName);
		if (itDrv == m_mapName2Driver.end())
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "(LoadDeviceConfig) device:%s 未找到对应的驱动,请检查设备配置!", strDeviceName.c_str());
			continue;
		}
		pDriver = itDrv->second;

		CMyPysicalDevice *pDevice = new CMyPysicalDevice();
		pDriver->m_mapDevice[strDeviceName] = pDevice;
		pDevice->m_pDriver = pDriver;
		pDevice->m_nId = atoi(strDeviceID.c_str());
		pDevice->m_strName = strDeviceName;
		pDevice->m_strDesc = strDescription;
		pDevice->m_strSimulate = strSimulate.c_str();
		pDevice->m_bDisable = ::atoi(strDisable.c_str());
		pDevice->m_nDirverId = ::atoi(strDriverId.c_str());

		// 下面参数在本程序中不使用
		pDevice->m_strConnType = strConnType;
		pDevice->m_strConnParam = strConnParam;
		pDevice->m_nConnTimeout = ::atoi(strConnTimeout.c_str());
		pDevice->m_strTaskNo = strTaskNo;
		pDevice->m_strParam1 = strParam1;
		pDevice->m_strParam2 = strParam2;
		pDevice->m_strParam3 = strParam3;
		pDevice->m_strParam4 = strParam4;
		pDevice->m_strProjectId = strProjectId;
		m_mapName2Device[strDeviceName] = pDevice;
	}

	// 增加模拟设备和内存变量设备

	m_pSimDevice = new CMySimulateDevice();
	m_pSimDevice->m_pDriver = m_pMemDriver;
	m_mapName2Device[m_pSimDevice->m_strName] = m_pSimDevice;

	m_pMemDevice = new CMyMemoryDevice();
	m_pMemDevice->m_pDriver = m_pMemDriver;
	m_mapName2Device[m_pMemDevice->m_strName] = m_pMemDevice;

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "device channel count:%d!", vecRow.size());

	return 0;
}

// 查询数据库Label信息
int CMainTask::LoadLabelConfig(CPKEviewDbApi &PKEviewDbApi)
{
	int nRet = PK_SUCCESS;

	// 查询tag点
	long lQueryId = -1;
	char szSql[2048] = { 0 };
	vector<vector<string> > vecRow;
	// 查询控制标签
	sprintf(szSql, "SELECT id, param, description FROM t_tag_value_dict");
	if (m_nVersion >= VERSION_NODESERVER_SUPPORT_MULTINODE)
		strcat(szSql, " WHERE (node_id is NULL or node_id=0)");

	string strError;
	nRet = PKEviewDbApi.SQLExecute(szSql, vecRow, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "query database failed:%s, ret:%d!(查询数据库失败), error:%s", szSql, nRet, strError.c_str());
		return -2;
	}

	for (int iTab = 0; iTab < vecRow.size(); iTab++)
	{
		vector<string> &row = vecRow[iTab];
		int iCol = 0;
		string strLabelID = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strParam = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDesc = NULLASSTRING(row[iCol].c_str());
		iCol++;

		map<int, CLabel *> mapLabels;
		vector<string> vecLabels = PKStringHelper::StriSplit(strParam, ",");
		for (int i = 0; i < vecLabels.size(); i++)
		{
			string strOneLabel = vecLabels[i];
			if (strOneLabel.empty())
				continue;

			vector<string> vecOneLabel = PKStringHelper::StriSplit(strOneLabel, ":");
			if (vecOneLabel.size() != 2)
			{
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Label config format error, t_tag_value_dict表 第%d行,Labelid:%s,配置格式错误(参数:%s)! invalid lable:%s!配置无效!应为: id:name,id2:name,...)", iTab, strLabelID.c_str(), strParam.c_str(), strOneLabel.c_str());
				continue;
			}
			CLabel *pLabel = new CLabel();
			pLabel->nControlVal = atoi(vecOneLabel[0].c_str());
			pLabel->strLabelName = vecOneLabel[1];
			pLabel->strDesc = strDesc;

			mapLabels[pLabel->nControlVal] = pLabel;
		}

		int nLabelID = atoi(strLabelID.c_str());

		m_mapId2Label[nLabelID] = mapLabels;
	}
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "readed tag value's label count:%d", m_mapId2Label.size());
	return 0;
}

// 返回map<string, string> mapDeviceName2DeviceId
// enableconnect & connstatus
int CMainTask::AddDeviceInternalTag()
{
	int nRet = PK_SUCCESS;

	// 如果设备未配置连接状态点，则增加连接点
	for (map<string, CMyDevice *>::iterator itDev = m_mapName2Device.begin(); itDev != m_mapName2Device.end(); itDev++)
	{
		string strDevName = itDev->first;
		CMyDevice *pDevice = itDev->second;
		if (pDevice->m_bDisable) // 已经禁用了
			continue;

		CDeviceTag *pDeviceTag = NULL;
		CDeviceTag *pDeviceTag2 = NULL;
		if (pDevice->m_strSimulate.compare("1") == 0)
		{
			pDeviceTag = new CSimTag(); // connstatus tag
			pDeviceTag2 = new CSimTag(); // enableconnect tag
		}
		else
		{
			pDeviceTag = new CDeviceTag();  // connstatus tag
			pDeviceTag2 = new CDeviceTag(); // enableconnect tag
		}

		// connstatus
		pDeviceTag->strAddress = "connstatus";
		pDeviceTag->strName = "device." + pDevice->m_strName + "." + pDeviceTag->strAddress; // device.{devicename}.connstatus
		pDeviceTag->pDevice = pDevice;
		pDeviceTag->bEnableAlarm = false;
		pDeviceTag->bEnableControl = 0;
		pDeviceTag->pObject = NULL;
		pDeviceTag->pObjProp = NULL;
		pDeviceTag->nDataType = TAG_DT_INT16;
		pDeviceTag->strDataType = DT_NAME_INT16;
		pDeviceTag->nDataTypeClass = CDataTag::GetDataTypeClass(pDeviceTag->nDataType);
		m_mapName2Tag[pDeviceTag->strName] = pDeviceTag;
		m_mapName2DeviceTag[pDeviceTag->strName] = pDeviceTag;

		// enableconnect
		pDeviceTag2->strAddress = "enableconnect";
		pDeviceTag2->strName = "device." + pDevice->m_strName + "." + pDeviceTag2->strAddress; // device.{devicename}.enableconnect
		pDeviceTag2->pDevice = pDevice;
		pDeviceTag2->bEnableAlarm = false;
		pDeviceTag2->bEnableControl = 0;
		pDeviceTag2->pObject = NULL;
		pDeviceTag2->pObjProp = NULL;
		pDeviceTag2->nDataType = TAG_DT_INT16;
		pDeviceTag2->strDataType = DT_NAME_INT16;
		pDeviceTag2->nDataTypeClass = CDataTag::GetDataTypeClass(pDeviceTag2->nDataType);
		m_mapName2Tag[pDeviceTag2->strName] = pDeviceTag2;
		m_mapName2DeviceTag[pDeviceTag2->strName] = pDeviceTag2;
	}
	return 0;
}

// 查询数据库变量报警信息
int CMainTask::LoadTagAlarmConfig(CPKEviewDbApi &PKEviewDbApi)
{
	int nRet = PK_SUCCESS;

	// 查询tag点
	long lQueryId = -1;
	char szSql[2048] = { 0 };
	vector<vector<string> > vecRow;
	// 查询报警点

	int nAlarmTagNum = 0;
	sprintf(szSql, "SELECT ALM.id as id,TAG.name as tagname, TAG.cname as tagcname, TAG.datatype as datatype,ALM.cname as alarmcname,ALMTYPE.name as alarmtypename, judgemethod, DEV.enable, priority, threshold \
				   	FROM t_device_tagalarm ALM left join t_device_tag TAG on ALM.tag_id=TAG.id LEFT JOIN t_alarm_type ALMTYPE on ALM.alarmtype_id=ALMTYPE.id,t_device_list DEV,t_device_driver DRV \
					where TAG.device_id = DEV.id and DEV.driver_id = DRV.id \
					and DEV.driver_id = DRV.id and (DEV.enable is null or DEV.enable<>0 or DEV.enable='') and (DRV.enable is null or DRV.enable<>0 or DRV.enable='')");
	if (m_nVersion >= VERSION_NODESERVER_SUPPORT_MULTINODE)
		strcat(szSql, " AND (DRV.node_id is NULL or DRV.node_id=0 or DRV.node_id='')");
	string strError;
	nRet = PKEviewDbApi.SQLExecute(szSql, vecRow, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "fail to query database:%s,retcode:%d, error:%s, 程序将无法继续执行，请处理...", szSql, nRet, strError.c_str());
		return -2;
	}

	for (int iTab = 0; iTab < vecRow.size(); iTab++)
	{
		vector<string> &row = vecRow[iTab];
		int iCol = 0;
		string strAlarmID = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strTagName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strTagCName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDataType = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strAlarmCName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strAlarmTypeName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strJudgemethod = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strEnable = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strPriority = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strThreshold = NULLASSTRING(row[iCol].c_str());
		iCol++;

		map<string, CDataTag *>::iterator itMapTag = m_mapName2Tag.find(strTagName);
		if (itMapTag == m_mapName2Tag.end())
		{
			g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "alarm config(报警配置)(alarmid:%s,name:%s,cname:%s): cannot find tag(未找到对应的tag点)，invalid(非法)!",
				strAlarmID.c_str(), strTagName.c_str(), strAlarmCName.c_str());
			continue;
		}

		CDataTag *pTag = itMapTag->second;
		//string strTmp = strJudgemethod;
		//std::transform(strTmp.begin(), strTmp.end(), strTmp.begin(), ::tolower);
		string strAlarmTagName = strTagName + "." + strJudgemethod; // 报警名称需要拼接为： tagname.alm_LL.....
		if (m_mapName2AlarmTag.find(strAlarmTagName) != m_mapName2AlarmTag.end())
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "TAG:%s,ALARM:%s has exist in g_mapName2AlarmTag", pTag->strName.c_str(), strAlarmTagName.c_str());
		}
		else
		{
			if (pTag->nDataTypeClass == DATATYPE_CLASS_OTHER) // blob 或其他未定义数据类型
			{
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "[%s] alarm config error(报警点配置错误), datatype:%s is not number datatype, please check!(数据类型必须是数值类型,请检查）", strAlarmTagName.c_str(), strDataType.c_str());
				continue;
			}

			CAlarmTag *pAlarmTag = new CAlarmTag();
			pAlarmTag->pTag = pTag;
			pAlarmTag->bEnable = true; //  atoi(strEnable.c_str());
			pAlarmTag->nLevel = std::atoi(strPriority.c_str());
			pAlarmTag->strAlarmTypeName = strAlarmTypeName;

			pAlarmTag->strJudgeMethod = strJudgemethod;
			CAlarmTag::GetJudgeMethodTypeAndIndex(strJudgemethod.c_str(), pAlarmTag->nJudgeMethodType, pAlarmTag->strJudgeMethodIndex);
			pAlarmTag->SplitThresholds(strThreshold.c_str()); // 将threshold进行切分		

			pTag->vecAllAlarmTags.push_back(pAlarmTag);
			
			m_mapName2AlarmTag[pTag->strName + "." + strJudgemethod] = pAlarmTag;
			nAlarmTagNum++;
		}
	}
	// m_nTagNum += nAlarmTagNum;
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "readed alarm point count:%d!(读取到报警点的个数，设备模式)", nAlarmTagNum);
	return 0;
}

// 根据点名提取出对象，并建立对象、点之间的关系
// 按说设备模式才会需要提取出对象。对象模式应该是不需要的
int CMainTask::ExtractObjectFromTags()
{
	int nRet = PK_SUCCESS;

	// 提取对象
	map<string, CDataTag *>::iterator itMapTagName = m_mapName2Tag.begin();
	for (itMapTagName; itMapTagName != m_mapName2Tag.end(); itMapTagName++)
	{
		// 根据点名提取出对象，并建立对象、点之间的关系
		CDataTag *pTag = itMapTagName->second;
		string strObjectName = ExtractObjectName(itMapTagName->first); // 如果是一段式如：testtag1，那么传出的对象名是空的！此时对于模拟点和内存点，需要根据类型来设置对象
		if (strObjectName.compare("NULL") == 0)		// tag点名是一段式，不属于某个对象
		{
			continue;
		}

		map<string, CMyObject *>::iterator itMapObject = m_mapName2Object.find(strObjectName);  // 在对象列表中查找当前点的对象
		if (itMapObject == m_mapName2Object.end())   // 没有找到，说明该对象还未被加入过，将对象加入
		{
			CMyObject *pObject = new CMyObject();
			pObject->strName = strObjectName;
			pObject->m_pDevice = pTag->pDevice;
			pObject->vecTags.push_back(pTag);
			m_mapName2Object[strObjectName] = pObject;
			pTag->pObject = pObject;
		}
		else   // 找到了，说明该对象已经被加入过，将点加入到对象的点列表中
		{
			CMyObject *pObject = itMapObject->second;
			pObject->vecTags.push_back(itMapTagName->second);
			pTag->pObject = pObject;
		}
	}
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "提取出对象个数:%d个(tag个数共:%d个,含内部变量)", m_mapName2Object.size(), m_mapName2Tag.size());
	return nRet;
}

// 得到每一个报警点的报警上下限
int CMainTask::AutoCalcLimitsOfTagAlarm()
{
	// 得到每一个报警点的报警上下限
	map<string, CDataTag *>::iterator itMap = m_mapName2Tag.begin();
	for (itMap; itMap != m_mapName2Tag.end(); itMap++)
	{
		CDataTag *pTag = itMap->second;
		CalcLHLimits(pTag); // 计算每个点的上下限值！
	}
	return 0;
}

int CMainTask::LoadConfig()
{
	int nRet = PK_SUCCESS;

	// 初始化数据库连接
	CPKEviewDbApi PKEviewDbApi;
	nRet = PKEviewDbApi.Init();
	if(nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Init Database Failed:%d!(初始化数据库失败), 程序将无法继续执行，请处理...", nRet);
		PKEviewDbApi.Exit();
		return -2;
	}

	m_nVersion = PKEviewComm::GetVersion(PKEviewDbApi, "pknodeserver");

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "success to init database, begin read config including(driver,tag,alarms), version:%d", m_nVersion);

	// 查询所有报警类型信息
	nRet = LoadAlarmTypeConfig(PKEviewDbApi); // 设备/对象模式下的报警类型配置
	//if (nRet != 0)
	//	return nRet;

	// 查询指定的驱动列表
	nRet = LoadDriverConfig(PKEviewDbApi);
	//if (nRet != 0)
	//	return nRet;

	// 读取设备和变量的配置信息
	nRet = LoadDeviceConfig(PKEviewDbApi);
	//if (nRet != 0)
	//	return nRet;

	// 查询数据库Label信息
	nRet = LoadLabelConfig(PKEviewDbApi); // 读取对象/设备模式下的标签配置
	//if (nRet != 0)
	//	return nRet;

	// 设备模式下的变量配置
	nRet = LoadTagConfig(PKEviewDbApi);
	// if (nRet != 0)
	//	return nRet;
	AddDeviceInternalTag(); // 增加内部点，包括：enableconnect和connstatus点

	// 对象模式下的对象+属性配置
	if (true)
	{
		// 加载类的信息
		LoadClassConfig(PKEviewDbApi);

		LoadClassPropConfig(PKEviewDbApi);
		
		LoadClassAlarmConfig(PKEviewDbApi);

		// 加载对象的信息
		LoadObjectConfig(PKEviewDbApi); // 加载对象的信息,生成对象的属性和报警属性

		CreateChannelClassAndObj(); // 创建通道有关的类、属性和对象。如果已经手工配置则无需创建了

		// 加载对类属性的改变部分
		LoadObjectPropConfig(PKEviewDbApi); 

		// 加载对象自己修改的属性
		LoadObjectPropAlarmConfig(PKEviewDbApi); // 加载对象自己修改的属性

		// 生成TAG点信息
		GenerateTagsFromObjProps();
	}

	// 从tag点中找出二次变量点，判断是二级变量还是三级变量（我们现在只支持三级），分别存储到不同的map，便于以后计算使用
	SplitCalcTags();

	// 至此，所有的TAG点都已经生成完毕了!

	// 查询数据库变量报警信息，设备模式
	nRet = LoadTagAlarmConfig(PKEviewDbApi);
	//if (nRet != 0)
	//	return nRet;

	nRet = AddPredefineTagToConfig(); // 增加预定义的系统变量
	if (nRet != 0)
		return nRet;

	// 根据点名提取出对象，并建立对象、点之间的关系
	nRet = ExtractObjectFromTags();
	//if (nRet != 0)
	//	return nRet;

	// 得到每一个报警点的报警上下限
	nRet = AutoCalcLimitsOfTagAlarm();

	WriteTagConfigAndInitValToMemDB();

	// 清楚之间的报警，避免每次重启该表中会有越来越多的报警
	ClearPreviousRealAlarms(PKEviewDbApi);

	//加载历史报警。主键带有时间，不宜再加载了，否则可能会有很多报警
	// LoadPreviousRealAlarms(PKEviewDbApi);

	PKEviewDbApi.Exit();

	CalcAllTagDataSize();

	return nRet;
}

int CalcLHLimits_Real(CDataTag *pTag)
{
	double dbHHHLimit = INT_MAX;
	double dbHHLimit = INT_MAX;
	double dbHLimit = INT_MAX;
	double dbLLimit = INT_MIN;
	double dbLLLimit = INT_MIN;
	double dbLLLLimit = INT_MIN;

	bool bHaveHHHAlarm = false;
	bool bHaveHHAlarm = false;
	bool bHaveLLAlarm = false;
	bool bHaveLLLAlarm = false;
	// 第一轮，预处理
	for (int i = 0; i < pTag->vecAllAlarmTags.size(); i++)
	{
		CAlarmTag *pAlarmTag = pTag->vecAllAlarmTags[i];
		if (pAlarmTag->nJudgeMethodType == ALARM_JUDGE_HHH)
		{
			dbHHHLimit = pAlarmTag->dbThreshold;
			bHaveHHHAlarm = true;
		}
		else if (pAlarmTag->nJudgeMethodType == ALARM_JUDGE_HH)
		{
			dbHHLimit = pAlarmTag->dbThreshold;
			bHaveHHAlarm = true;
		}
		else if (pAlarmTag->nJudgeMethodType == ALARM_JUDGE_H)
		{
			dbHLimit = pAlarmTag->dbThreshold;
		}
		else if (pAlarmTag->nJudgeMethodType == ALARM_JUDGE_L)
		{
			dbLLimit = pAlarmTag->dbThreshold;
		}
		else if (pAlarmTag->nJudgeMethodType == ALARM_JUDGE_LL)
		{
			dbLLLimit = pAlarmTag->dbThreshold;
			bHaveLLAlarm = true;
		}
		else if (pAlarmTag->nJudgeMethodType == ALARM_JUDGE_LLL)
		{
			dbLLLLimit = pAlarmTag->dbThreshold;
			bHaveLLLAlarm = true;
		}
	}

	// 第二轮，计算报警上下限
	for (int i = 0; i < pTag->vecAllAlarmTags.size(); i++)
	{
		CAlarmTag *pAlarmTag = pTag->vecAllAlarmTags[i];
		if (pAlarmTag->nJudgeMethodType == ALARM_JUDGE_HHH)
		{
			pAlarmTag->dbThreshold = dbHHLimit;
			pAlarmTag->dbThreshold2 = INT_MAX;
		}
		else if (pAlarmTag->nJudgeMethodType == ALARM_JUDGE_HH)
		{
			pAlarmTag->dbThreshold = dbHHLimit;
			if (bHaveHHHAlarm)
				pAlarmTag->dbThreshold2 = dbHHHLimit;
			else
				pAlarmTag->dbThreshold2 = INT_MAX;
		}
		else if (pAlarmTag->nJudgeMethodType == ALARM_JUDGE_H)
		{
			pAlarmTag->dbThreshold = dbHLimit;
			if (bHaveHHAlarm)
				pAlarmTag->dbThreshold2 = dbHHLimit;
			else if (bHaveHHHAlarm)
				pAlarmTag->dbThreshold2 = dbHHHLimit;
			else
				pAlarmTag->dbThreshold2 = INT_MAX;
		}
		else if (pAlarmTag->nJudgeMethodType == ALARM_JUDGE_L)
		{
			pAlarmTag->dbThreshold2 = dbLLimit;
			if (bHaveLLAlarm)
				pAlarmTag->dbThreshold = dbLLLimit;
			else if (bHaveLLLAlarm)
				pAlarmTag->dbThreshold = dbLLLLimit;
			else
				pAlarmTag->dbThreshold = INT_MIN;
		}
		else if (pAlarmTag->nJudgeMethodType == ALARM_JUDGE_LL)
		{
			pAlarmTag->dbThreshold2 = dbLLLimit;
			if (bHaveLLLAlarm)
				pAlarmTag->dbThreshold = dbLLLLimit;
			else
				pAlarmTag->dbThreshold = INT_MIN;
		}
		else if (pAlarmTag->nJudgeMethodType == ALARM_JUDGE_LLL)
		{
			pAlarmTag->dbThreshold2 = dbLLLLimit;
			pAlarmTag->dbThreshold = INT_MIN;
		}
	}

	return 0;
}

int CalcLHLimits_Integer(CDataTag *pTag)
{
	ACE_INT64 nHHHLimit = INT_MAX;
	ACE_INT64 nHHLimit = INT_MAX;
	ACE_INT64 nHLimit = INT_MAX;
	ACE_INT64 nLLimit = INT_MIN;
	ACE_INT64 nLLLimit = INT_MIN;
	ACE_INT64 nLLLLimit = INT_MIN;

	bool bHaveHHHAlarm = false;
	bool bHaveHHAlarm = false;
	bool bHaveLLAlarm = false;
	bool bHaveLLLAlarm = false;
	// 第一轮，预处理
	for (int i = 0; i < pTag->vecAllAlarmTags.size(); i++)
	{
		CAlarmTag *pAlarmTag = pTag->vecAllAlarmTags[i];
		if (pAlarmTag->nJudgeMethodType == ALARM_JUDGE_HHH)
		{
			nHHHLimit = pAlarmTag->nThreshold;
			bHaveHHHAlarm = true;
		}
		else if (pAlarmTag->nJudgeMethodType == ALARM_JUDGE_HH)
		{
			nHHLimit = pAlarmTag->nThreshold;
			bHaveHHAlarm = true;
		}
		else if (pAlarmTag->nJudgeMethodType == ALARM_JUDGE_H)
		{
			nHLimit = pAlarmTag->nThreshold;
		}
		else if (pAlarmTag->nJudgeMethodType == ALARM_JUDGE_L)
		{
			nLLimit = pAlarmTag->nThreshold;
		}
		else if (pAlarmTag->nJudgeMethodType == ALARM_JUDGE_LL)
		{
			nLLLimit = pAlarmTag->nThreshold;
			bHaveLLAlarm = true;
		}
		else if (pAlarmTag->nJudgeMethodType == ALARM_JUDGE_LLL)
		{
			nLLLLimit = pAlarmTag->nThreshold;
			bHaveLLLAlarm = true;
		}
	}

	// 第二轮，计算报警上下限
	for (int i = 0; i < pTag->vecAllAlarmTags.size(); i++)
	{
		CAlarmTag *pAlarmTag = pTag->vecAllAlarmTags[i];
		if (pAlarmTag->nJudgeMethodType == ALARM_JUDGE_HHH)
		{
			pAlarmTag->nThreshold = nHHLimit;
			pAlarmTag->nThreshold2 = INT_MAX;
		}
		else if (pAlarmTag->nJudgeMethodType == ALARM_JUDGE_HH)
		{
			pAlarmTag->nThreshold = nHHLimit;
			if (bHaveHHHAlarm)
				pAlarmTag->nThreshold2 = nHHHLimit;
			else
				pAlarmTag->nThreshold2 = INT_MAX;
		}
		else if (pAlarmTag->nJudgeMethodType == ALARM_JUDGE_H)
		{
			pAlarmTag->nThreshold = nHLimit;
			if (bHaveHHAlarm)
				pAlarmTag->nThreshold2 = nHHLimit;
			else if (bHaveHHHAlarm)
				pAlarmTag->nThreshold2 = nHHHLimit;
			else
				pAlarmTag->nThreshold2 = INT_MAX;
		}
		else if (pAlarmTag->nJudgeMethodType == ALARM_JUDGE_L)
		{
			pAlarmTag->nThreshold2 = nLLimit;
			if (bHaveLLAlarm)
				pAlarmTag->nThreshold = nLLLimit;
			else if (bHaveLLLAlarm)
				pAlarmTag->nThreshold = nLLLLimit;
			else
				pAlarmTag->nThreshold = INT_MIN;
		}
		else if (pAlarmTag->nJudgeMethodType == ALARM_JUDGE_LL)
		{
			pAlarmTag->nThreshold2 = nLLLimit;
			if (bHaveLLLAlarm)
				pAlarmTag->nThreshold = nLLLLimit;
			else
				pAlarmTag->nThreshold = INT_MIN;
		}
		else if (pAlarmTag->nJudgeMethodType == ALARM_JUDGE_LLL)
		{
			pAlarmTag->nThreshold2 = nLLLLimit;
			pAlarmTag->nThreshold = INT_MIN;
		}
	}

	return 0;
}

// 计算报警点的上下限
int CalcLHLimits(CDataTag *pTag)
{
	if (pTag->nDataTypeClass == DATATYPE_CLASS_REAL)
		return CalcLHLimits_Real(pTag);
	else if (pTag->nDataTypeClass == DATATYPE_CLASS_INTEGER)
		return CalcLHLimits_Integer(pTag);
	else
		return -1;

}

void string_replace(string&s1, const string&s2, const string&s3);

// 从tag点中找出二次变量点，判断是二级变量还是三级变量（我们现在只支持三级），分别存储到不同的map，便于以后计算使用
int CMainTask::SplitCalcTags()
{
	map<string, CCalcTag *>::iterator itCalcTag = m_mapName2CalcTag.begin();
	for (itCalcTag; itCalcTag != m_mapName2CalcTag.end(); itCalcTag++)
	{
		CCalcTag *pCalcTag = itCalcTag->second;
		if (pCalcTag->pObject)
		{
			string strParam1Replace = pCalcTag->pObject->strParam1;
			string strParam2Replace = pCalcTag->pObject->strParam2;
			string strParam3Replace = pCalcTag->pObject->strParam3;
			string strParam4Replace = pCalcTag->pObject->strParam4;
			string strObjectReplace = pCalcTag->pObject->strName;
			string_replace(pCalcTag->strAddress, "self.param1", strParam1Replace.c_str()); // SELF表示对象名称，替换为实际对象.属性
			string_replace(pCalcTag->strAddress, "self.param2", strParam2Replace.c_str()); // SELF表示对象名称，替换为实际对象.属性
			string_replace(pCalcTag->strAddress, "self.param3", strParam3Replace.c_str()); // SELF表示对象名称，替换为实际对象.属性
			string_replace(pCalcTag->strAddress, "self.param4", strParam4Replace.c_str()); // SELF表示对象名称，替换为实际对象.属性
			string_replace(pCalcTag->strAddress, "self", strObjectReplace.c_str()); // SELF表示对象名称，替换为实际对象.属性
		}

		string strExpr = pCalcTag->strAddress;
		CTagExprCalc &exprCalc = pCalcTag->exprCalc;
		exprCalc.ParseExpr(strExpr.c_str());
		vector<string> vars;
		bool bRet = exprCalc.GetVars(exprCalc.GetRootNode(), vars);

		bool bLevel3CalcTag = false; // 是否是三级计算变量
		for (int i = 0; i < vars.size(); i ++) // 对于二级变量中的每一个tag点
		{
			string strTagName = vars[i]; // 可能是一级变量，也可能是二级变量或三级变量。如果是二级变量，SELF表示对象名称
			map<string, CDataTag*>::iterator itTag = m_mapName2Tag.find(strTagName);
			if (itTag == m_mapName2Tag.end())   // 如果没有找到，表达式配置有问题，提示并跳出
			{
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "second tag:%s 的expression(%s) error,tag: %s does not exist, cannot calc second-level tag's value!", (itCalcTag->first).c_str(), strExpr.c_str(), strTagName.c_str());
				continue;
			}

			CDataTag *pTag = itTag->second;
			exprCalc.m_mapName2Tag[itTag->first] = pTag;

			if (VARIABLE_TYPE_CALC == pTag->nTagType || VARIABLE_TYPE_ACCUMULATE_COUNT == pTag->nTagType || VARIABLE_TYPE_ACCUMULATE_TIME == pTag->nTagType)	// 找到了，判断是不是三级计算变量,如果二级变量包含的tag点还是二级变量，那么这个点是三级变量
			{
				bLevel3CalcTag = true;
			}
		}

		if (bLevel3CalcTag)
			m_mapName2CalcTagLevel3[itCalcTag->first] = pCalcTag;
		else
			m_mapName2CalcTagLevel2[itCalcTag->first] = pCalcTag;
		
	}

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "二级变量:%d个", m_mapName2CalcTagLevel2.size());
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "三级变量:%d个", m_mapName2CalcTagLevel3.size());
	return PK_SUCCESS;
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

int CMainTask::ClearPreviousRealAlarms(CPKEviewDbApi &PKEviewDbApi)
{
	int nRet = PK_SUCCESS;

	// 查询tag点
	long lQueryId = -1;
	char szSql[2048] = { 0 };
	vector<vector<string> > vecRow;
	// 查询控制标签
	int nLabelNum = 0;
	sprintf(szSql, "delete from t_alarm_real");
	string strError;
	nRet = PKEviewDbApi.SQLExecute(szSql, vecRow, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "清除之前的实时报警信息失败,请检查该表是否存在,query database failed:%s, ret:%d!(查询数据库失败), error:%s", szSql, nRet, strError.c_str());
		return -2;
	}
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "清除之前的实时报警信息成功!");
	return 0;
}

// 查询数据库Label信息
int CMainTask::LoadPreviousRealAlarms(CPKEviewDbApi &PKEviewDbApi)
{
	int nRet = PK_SUCCESS;

	// 查询tag点
	long lQueryId = -1;
	char szSql[2048] = { 0 };
	vector<vector<string> > vecRow;
	// 查询控制标签
	int nLabelNum = 0;
	if (m_nVersion < VERSION_NODESERVER_ALARM_HAVE_OBJDESCRIPTION)
	{
		sprintf(szSql, "select tagname,judgemethod,objectname,propname,producetime,isalarming,isconfirmed,\
					   confirmer,confirmtime,recoverytime,valonproduce,valonrecover,valonconfirm,threshold,repetitions,description,param1,param2,param3,param4 from t_alarm_real");
	}
	else
	{
		sprintf(szSql, "select tagname,judgemethod,objectname,objectdescription,propname,producetime,isalarming,isconfirmed,\
					   	confirmer,confirmtime,recoverytime,valonproduce,valonrecover,valonconfirm,threshold,repetitions,description,param1,param2,param3,param4 from t_alarm_real");

	}
	string strError; 
	nRet = PKEviewDbApi.SQLExecute(szSql, vecRow, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "加载之前报警信息,query database failed:%s, ret:%d!(查询数据库失败), error:%s", szSql, nRet, strError.c_str());
		return -2;
	}

	map<string, string> mapTagAlarmName; // 为了去除冗余的报警
	vector<string> vecSqlToDelete;
	for (int iTab = 0; iTab < vecRow.size(); iTab++)
	{
		vector<string> &row = vecRow[iTab];
		int iCol = 0;
		string strTagName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strJudgeMethod = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strObjectName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strObjectDesc = "";
		if (m_nVersion >= VERSION_NODESERVER_ALARM_HAVE_OBJDESCRIPTION)
		{
			strObjectDesc = NULLASSTRING(row[iCol].c_str());
			iCol++;
		}
		string strPropName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strProduceTime = NULLASSTRING(row[iCol].c_str());
		iCol++;
		bool bIsAlarming = ::atoi(NULLASSTRING(row[iCol].c_str()));
		iCol++;
		bool bIsConfirmed = ::atoi(NULLASSTRING(row[iCol].c_str()));
		iCol++;
		string strConfirmer = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strConfirmTime = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strRecoveryTime = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strValOnProduce = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strValOnRecover = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strValOnConfirm = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strTreshold = NULLASSTRING(row[iCol].c_str());
		iCol++;
		int nRepetitions = ::atoi(NULLASSTRING(row[iCol].c_str()));
		iCol++;
		string strDescription = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strParam1 = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strParam2 = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strParam3 = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strParam4 = NULLASSTRING(row[iCol].c_str());
		iCol++;

		// 删除不存在的tag点的报警记录
		string strAlarmName = strTagName + "." + strJudgeMethod; // +"." + strProduceTime; // obj1.prop1.HH.2018-03-2 15.23.22 作为主键
		map<string,string>::iterator itMapReduntCheck = mapTagAlarmName.find(strAlarmName);
		map<string, CAlarmTag *>::iterator itAlarmTag = m_mapName2AlarmTag.find(strAlarmName);
		if (itAlarmTag == m_mapName2AlarmTag.end() ||strProduceTime.compare("")== 0 ||
			(strRecoveryTime.compare("") != 0 && strConfirmTime.compare("") != 0)||
			itMapReduntCheck != mapTagAlarmName.end()
			) // 没有找到这个报警点,或者这个报警点前面已经查到了，这是第二次查到
		{
			// 在数据库表中删除这个报警
			char szSQL[2048] = { 0 };
			sprintf(szSQL, "delete from t_alarm_real where tagname='%s' and judgemethod='%s'", strTagName.c_str(), strJudgeMethod.c_str());
			vecSqlToDelete.push_back(szSQL);
			continue;
		}

		mapTagAlarmName[strAlarmName] = strAlarmName;
		CAlarmTag *pAlarmTag = itAlarmTag->second;

		CAlarmingInfo *pAlarmingInfo = new CAlarmingInfo();
		pAlarmTag->vecAlarming.insert(pAlarmTag->vecAlarming.begin(), pAlarmingInfo);
		pAlarmingInfo->pAlarmTag = pAlarmTag;		
		// 存在这个报警点的报警，下面将这些报警信息重新执行一次，以便初始报警信息就包含了这些报警
		unsigned int nProduceMSec = 0;
		unsigned int nProduceSec = PKTimeHelper::String2HighResTime(strProduceTime.c_str(), &nProduceMSec);
		pAlarmingInfo->strParam1 = strParam1;
		pAlarmingInfo->strParam2 = strParam2;
		pAlarmingInfo->strParam3 = strParam3;
		pAlarmingInfo->strParam4 = strParam4;

		pAlarmTag->pTag->curVal.AssignStrValueAndQuality(pAlarmTag->pTag->nDataTypeClass, strValOnProduce, 0);

		DATAPROC_TASK->m_eaProcessor.OnEAProduce(pAlarmingInfo, strTreshold.c_str(), false, nProduceSec, nProduceMSec, nRepetitions);
		if (strConfirmTime.compare("") != 0) // 如果已确认，那么进入已确认流程处理
		{
			unsigned int nConfirmMSec = 0;
			unsigned int nConfirmSec = PKTimeHelper::String2HighResTime(strConfirmTime.c_str(), &nConfirmMSec);
			pAlarmingInfo->strConfirmUserName = strConfirmer;
			pAlarmTag->pTag->curVal.AssignStrValueAndQuality(pAlarmTag->pTag->nDataTypeClass, strValOnConfirm, 0); // 确认时的值为当前tag的值，质量为GOOD
			DATAPROC_TASK->m_eaProcessor.OnEAConfirmOne(pAlarmingInfo, strConfirmer.c_str(), false, nConfirmSec, nConfirmMSec);
		}
		if (strRecoveryTime.compare("") != 0) // 如果已确认，那么进入已确认流程处理
		{
			unsigned int nRecoveryMSec = 0;
			unsigned int nRecoverySec = PKTimeHelper::String2HighResTime(strRecoveryTime.c_str(), &nRecoveryMSec);
			pAlarmTag->pTag->curVal.AssignStrValueAndQuality(pAlarmTag->pTag->nDataTypeClass, strValOnRecover, 0); // 恢复时的值为当前tag的值，质量为GOOD
			DATAPROC_TASK->m_eaProcessor.OnEARecover(pAlarmingInfo, false, nRecoverySec, nRecoveryMSec);
		}
	}//for (int iTab = 0; iTab < vecRow.size(); iTab++)

	mapTagAlarmName.clear();

	// 如果有垃圾报警，那么删除之
	// 遍历事件;
	int nSuccessDelNum = 0;
	int nFailDelNum = 0;
	for (int iSqlDel = 0; iSqlDel < vecSqlToDelete.size(); iSqlDel++)
	{
		string &strSQL = vecSqlToDelete[iSqlDel]; // 新报警是SQL语句。老报警是Update，如果失败则继续执行一个Insert
		string strError;
		int nRet = PKEviewDbApi.SQLExecute(strSQL.c_str(), &strError);
		if (nRet != 0)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "删除过时的实时报警t_real_alarm,执行SQL失败,错误码:%d, SQL:%s, error:%s", nRet, strSQL.c_str(), strError.c_str());
			nFailDelNum++;
			continue;
		}
		nSuccessDelNum++;
	}
	if (vecSqlToDelete.size() > 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "删除过时的实时报警t_real_alarm, 成功删除:%d个, 失败:%d个", nSuccessDelNum, nFailDelNum);
		vecSqlToDelete.clear();
	}	
	return 0;
}

// 如果tag点名是两段式，提取第一段为对象名并返回
string CMainTask::ExtractObjectName(string strTagName)
{
	// 判断点名中是否包含对象
	int nObjectPos = strTagName.find(".");

	string strObjectName = "NULL";
	if (nObjectPos != string::npos)
	{
		strObjectName = strTagName.substr(0, nObjectPos);
	}

	return strObjectName;
}

void CMainTask::ClearMemDBTags()
{

}

int CMainTask::LoadClassConfig(CPKEviewDbApi &PKEviewDbApi)
{
	int nRet = PK_SUCCESS;

	// 查询tag点
	long lQueryId = -1;
	char szSql[2048] = { 0 };
	vector<vector<string> > vecRow;

	int nTagNum = 0;
	sprintf(szSql, "select A.id as id,A.name as name,A.description as description,A.parent_id as parent_id,B.name as parent_name from t_class_list A left join t_class_list B on A.parent_id=B.id");
	if (m_nVersion >= VERSION_NODESERVER_SUPPORT_MULTINODE)
		strcat(szSql, " WHERE (A.node_id is NULL or A.node_id=0 or A.node_id='')  AND (A.enable is NULL or A.enable <>0 or A.enable='')");
	string strError;
	nRet = PKEviewDbApi.SQLExecute(szSql, vecRow, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "fail to read class info from database:%s, return:%d, error:%s", szSql, nRet, strError.c_str());
		return -2;
	}

	for (int iTab = 0; iTab < vecRow.size(); iTab++)
	{
		vector<string> &row = vecRow[iTab];
		int iCol = 0;
		string strId = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDesc = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strParentId = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strProjectId ="";
		string strParentName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		if (strName.empty())
			continue;

		map<string, CMyClass *>::iterator itMap = m_mapName2Class.find(strName);
		if (itMap == m_mapName2Class.end()) // 不存在该设备，增加之
		{
			int nId = atoi(strId.c_str());
			int nProjectId = atoi(strProjectId.c_str());
			int nParentId = atoi(strParentId.c_str());

			CMyClass *pMyClass = new CMyClass();
			pMyClass->m_nId = nId;
			pMyClass->m_nParentId = nParentId;
			pMyClass->m_nProjectId = nProjectId;
			pMyClass->m_strName = strName;
			pMyClass->m_strDesc = strDesc;
			pMyClass->strParentName = strParentName;
			pMyClass->m_mapClassProp.clear();
			m_mapName2Class[strName] = pMyClass;
		}
	}

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "read class count:%d!", m_mapName2Class.size());

	return 0;
}


int CMainTask::LoadClassPropConfig(CPKEviewDbApi &PKEviewDbApi)
{
	int nRet = PK_SUCCESS;

	// 查询tag点
	long lQueryId = -1;
	char szSql[2048] = { 0 };
	vector<vector<string> > vecRow;

	int nTotalPropNum = 0;
	sprintf(szSql, "select A.id as id,A.name as name,A.description as description,A.address as address, A.type_id as type_id,A.datatype as datatype,A.datalen as datalen,A.pollrate as pollrate,\
				   	initvalue,enable_range,rawmin,rawmax,outmin,outmax,byteorder,enable_control,hisdata_interval,trenddata_interval,A.param as param,\
					LBL.name as label_name,LBL.id as label_id, CLS.name as class_name, CLS.id as class_id FROM t_class_prop A \
					LEFT JOIN t_class_list CLS on A.class_id = CLS.id \
					LEFT JOIN t_tag_value_dict LBL on A.label_id = LBL.id");
	if (m_nVersion >= VERSION_NODESERVER_SUPPORT_MULTINODE)
		strcat(szSql, " WHERE (CLS.node_id is NULL or CLS.node_id=0 or CLS.node_id='') AND (CLS.enable is NULL or CLS.enable <>0 or CLS.enable='')");
	string strError;
	nRet = PKEviewDbApi.SQLExecute(szSql, vecRow, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "fail to query database:%s, return:%d, error:%s", szSql, nRet, strError.c_str());
		return -2;
	}

	for (int iTab = 0; iTab < vecRow.size(); iTab++)
	{
		vector<string> &row = vecRow[iTab];
		int iCol = 0;
		string strId = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strClassPropName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDeccription = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strAddress = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strTypeId = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDataType = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDataLen = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strPollRateMS = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strInitValue = NULLASSTRING(row[iCol].c_str());
		iCol++;

		string strEnableRange = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strRawMin = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strRawMax = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strOutMin = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strOutMax = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strByteOrder = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strEnableControl = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strHisDataInterval = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strTrendDataInterval = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strParam = NULLASSTRING(row[iCol].c_str());
		iCol++;

		string strLableName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strLabelId = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strClassName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strClassId = NULLASSTRING(row[iCol].c_str());
		iCol++;

		// 类名称和属性名称不能为空
		if (strClassName.empty()|| strClassPropName.empty())
			continue;

		// 根据名称找到类
		map<string, CMyClass *>::iterator itMapClass = m_mapName2Class.find(strClassName); // 这个类不存在，那么可能是类定义错误了，生成这个类！
		if (itMapClass == m_mapName2Class.end())
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "属性:%s 对应的类:%s,id:%s ", strClassPropName.c_str(), strClassName.c_str(), strClassId.c_str());
			continue;
		}
		CMyClass * pMyClass = itMapClass->second;

		// 检查属性是否已经存在，如果已经存在则不需要增加
		map<string, CMyClassProp *>::iterator itMapProp = pMyClass->m_mapClassProp.find(strClassPropName); 
		if (itMapProp != pMyClass->m_mapClassProp.end())
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "属性:%s 对应的类:%s 中已具有该属性", strClassPropName.c_str(), strClassName.c_str());
			continue;
		}

		// 声明一个属性，并加入
		CMyClassProp *pClassProp = new CMyClassProp();
		pClassProp->m_pClass = pMyClass;
		pClassProp->m_strName = strClassPropName;
		pClassProp->m_strDesc = strDeccription;
		pClassProp->m_strAddress = strAddress;
		pClassProp->m_strHisDataInterval = strHisDataInterval;
		pClassProp->m_strTrendDataInterval = strTrendDataInterval;
		pClassProp->m_bEnableControl = atoi(strEnableControl.c_str());
		pClassProp->m_strDataType = strDataType;

		pClassProp->m_strInitValue = strInitValue;
		pClassProp->m_strParam = strParam;
		pClassProp->m_strLabelName = strLableName;
		pClassProp->m_nLabelId = ::atoi(strLabelId.c_str());
		pClassProp->m_nPollRateMS = ::atoi(strPollRateMS.c_str());
		pClassProp->m_nPropTypeId = ::atoi(strTypeId.c_str());

		int nDataType;
		int nDataTypeLenBits;
		PKMiscHelper::GetTagDataTypeAndLen(strDataType.c_str(), strDataLen.c_str(), &nDataType, &nDataTypeLenBits);
		if (nDataType == TAG_DT_UNKNOWN)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Class：%s, Prop:%s datatype:%s not supported, assume int32!!", pMyClass->m_strName.c_str(), pClassProp->m_strName.c_str(), strDataType.c_str());
			nDataType = TAG_DT_INT32;
			nDataTypeLenBits = 32; // 32位4个字节
		}
		pClassProp->m_nDataLen = (nDataTypeLenBits + 7) / 8; // 字节个数
		pClassProp->m_nDataType = nDataType;
		pClassProp->m_nDataTypeClass = CDataTag::GetDataTypeClass(nDataType);

		pClassProp->CalcRangeParam(strEnableRange, strRawMin, strRawMax, strOutMin, strOutMax);

		pMyClass->m_mapClassProp[pClassProp->m_strName] = pClassProp;
		nTotalPropNum++;
	}

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "read classnum:%d, prop count:%d!", m_mapName2Class.size(), nTotalPropNum);

	return 0;
}

int CMainTask::CreateChannelClassAndObj()
{
	CMyClass *pChannelClass = NULL;
	map<string, CMyClass *>::iterator itClass = m_mapName2Class.find("通道");
	if (itClass == m_mapName2Class.end())
	{
		// 如果没有通道类，则增加一个特殊的类：通道
		pChannelClass = new CMyClass();
		pChannelClass->m_nId = -1;
		pChannelClass->m_nParentId = -1;
		pChannelClass->m_nProjectId = -1;
		pChannelClass->m_strName = "通道";
		pChannelClass->m_strDesc = "通道类的描述";
		pChannelClass->strParentName = "";
		pChannelClass->m_mapClassProp.clear();
		m_mapName2Class[pChannelClass->m_strName] = pChannelClass;
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "增加了一个内置类:通道!");
	}
	else
	{
		pChannelClass = itClass->second;
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "已经配置了  通道  这个类, 不需要自动增加!");
	}
	
	// 增加一个特殊的类：通道的属性：connstatus、enableconnect
	map<string, CMyClassProp *>::iterator itClsProp = pChannelClass->m_mapClassProp.find("connstatus");
	if (itClsProp == pChannelClass->m_mapClassProp.end())
	{
		// 声明一个属性，并加入
		CMyClassProp *pClassProp = new CMyClassProp();
		pClassProp->m_pClass = pChannelClass;
		pClassProp->m_strName = "connstatus";
		pClassProp->m_strDesc = "连接状态";
		pClassProp->m_strAddress = "connstatus";
		pClassProp->m_strHisDataInterval = "";
		pClassProp->m_strTrendDataInterval = "";
		pClassProp->m_bEnableControl = 0;
		pClassProp->m_strDataType = "int16";
		pClassProp->m_nDataType = TAG_DT_INT16;
		pClassProp->m_nDataLen = 2;
		pClassProp->m_strInitValue = "";
		pClassProp->m_strParam = "";
		pClassProp->m_bEnableRange = false;
		pClassProp->m_strLabelName = "";
		pClassProp->m_nLabelId = -1;
		pClassProp->m_nPollRateMS = 1000;
		pClassProp->m_nPropTypeId = VARIABLE_TYPE_DEVICE;
		pChannelClass->m_mapClassProp[pClassProp->m_strName] = pClassProp;
	}

	// 增加一个特殊的类：通道的属性：connstatus、enableconnect
	itClsProp = pChannelClass->m_mapClassProp.find("enableconnect");
	if (itClsProp == pChannelClass->m_mapClassProp.end())
	{
		// 声明一个属性，并加入
		CMyClassProp *pClassProp = new CMyClassProp();
		pClassProp->m_pClass = pChannelClass;
		pClassProp->m_strName = "enableconnect";
		pClassProp->m_strDesc = "允许连接";
		pClassProp->m_strAddress = "enableconnect";
		pClassProp->m_strHisDataInterval = "";
		pClassProp->m_strTrendDataInterval = "";
		pClassProp->m_bEnableControl = 1;
		pClassProp->m_strDataType = "int16";
		pClassProp->m_nDataType = TAG_DT_INT16;
		pClassProp->m_nDataLen = 2;
		pClassProp->m_strInitValue = "";
		pClassProp->m_strParam = "";
		pClassProp->m_bEnableRange = false;
		pClassProp->m_strLabelName = "";
		pClassProp->m_nLabelId = -1;
		pClassProp->m_nPollRateMS = 1000;
		pClassProp->m_nPropTypeId = VARIABLE_TYPE_DEVICE;
		pChannelClass->m_mapClassProp[pClassProp->m_strName] = pClassProp;
	}

	// 对每个通道实例化类
	for (map<string, CMyDevice *>::iterator itDev = m_mapName2Device.begin(); itDev != m_mapName2Device.end(); itDev++)
	{
		string strDevName = "device."; //device.设备名称作为对象名
		strDevName += itDev->first;
		CMyDevice *pDevice = itDev->second;

		map<string, CMyObject *>::iterator itObj = m_mapName2Object.find(strDevName);
		if (itObj != m_mapName2Object.end())
		{
			CMyObject *pMyObject = itObj->second;
			g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "已经为设备:%s 配置了同名的对象!", strDevName.c_str());
			GenerateObjectAndPropAndAlarmsFromClass(pMyObject, pChannelClass); // 已经实例化过了，再补充通道类的部分属性
			continue;
		}

		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "为通道:%s 增加了一个同名的对象!", strDevName.c_str());
		CMyObject *pMyObject = new CMyObject();
		pMyObject->nId = -1;
		pMyObject->strName = strDevName;
		pMyObject->strDescription = strDevName;
		pMyObject->strDriverName = pDevice->m_pDriver->m_strName;
		pMyObject->strSubsysName = "";
		pMyObject->nSubsysId = -1;
		pMyObject->strDeviceName = strDevName;
		pMyObject->nDeviceId = pDevice->m_nId;
		pMyObject->strParam1 = pDevice->m_strParam1;
		pMyObject->strParam2 = pDevice->m_strParam2;
		pMyObject->strParam3 = pDevice->m_strParam3;
		pMyObject->strParam4 = pDevice->m_strParam4;
		pMyObject->pClass = pChannelClass;
		pMyObject->m_pDevice = pDevice;
		m_mapName2Object[strDevName] = pMyObject;
		GenerateObjectAndPropAndAlarmsFromClass(pMyObject); // 以通道类作为主要类进行实例化
	}
	return 0;
}

int CMainTask::LoadObjectConfig(CPKEviewDbApi &PKEviewDbApi)
{
	int nRet = PK_SUCCESS;

	// 查询tag点
	long lQueryId = -1;
	char szSql[2048] = { 0 };
	vector<vector<string> > vecRow;

	// 查询设备列表
	int nTagNum = 0;
	sprintf(szSql, "SELECT A.id as id,A.name as name,A.description as description, SYS.id as sys_id,SYS.name as sys_name, CLS.name as class_name, \
		DRV.name as driver_name, DEV.name as device_name, DEV.id as device_id,\
		A.param1 as param1,A.param2 as param2,A.param3 as param3,A.param4 as param4\
		FROM t_object_list A\
		LEFT JOIN t_class_list CLS on A.class_id = CLS.id\
		LEFT JOIN t_subsys_list SYS on A.subsys_id = SYS.id\
		LEFT JOIN t_device_list DEV on A.device_id = DEV.id\
		LEFT JOIN t_device_driver DRV on DEV.driver_id = DRV.id\
		WHERE  A.class_id = CLS.id AND(DRV.enable is null or DRV.enable<>0 or DRV.enable='') and (DEV.enable is null or DEV.enable<>0 or DEV.enable='') and (A.enable is NULL or A.enable<>0 or A.enable='')");
	if (m_nVersion >= VERSION_NODESERVER_SUPPORT_MULTINODE)
		strcat(szSql, " AND (CLS.node_id IS NULL OR CLS.node_id = 0 OR CLS.node_id = '')");

	string strError;
	nRet = PKEviewDbApi.SQLExecute(szSql, vecRow, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "fail to read class info from database:%s, return:%d, error:%s", szSql, nRet, strError.c_str());
		return -2;
	}

	for (int iTab = 0; iTab < vecRow.size(); iTab++)
	{
		vector<string> &row = vecRow[iTab];
		int iCol = 0;
		string strId = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDesc = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strSubsysId = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strSubsysNme = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strClassName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDriverName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDeviceName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDeviceId = NULLASSTRING(row[iCol].c_str());
		iCol++;

		string strParam1 = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strParam2 = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strParam3 = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strParam4 = NULLASSTRING(row[iCol].c_str());
		iCol++;

		if (strName.empty())
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "对象名称:为空, 类：%s, 跳过@ ", strName.c_str(), strClassName.c_str());
			continue;
		}

		map<string, CMyObject *>::iterator itMap = m_mapName2Object.find(strName);
		if (itMap != m_mapName2Object.end()) // 不存在该设备，增加之
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "对象名称:%s 已存在，类：%s ", strName.c_str(), strClassName.c_str());
			continue;
		}

		// 根据名称找到类
		map<string, CMyClass *>::iterator itMapClass = m_mapName2Class.find(strClassName); // 这个类不存在，那么可能是类定义错误了，生成这个类！
		if (itMapClass == m_mapName2Class.end())
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "对象:%s 对应的类:%s 不存在", strName.c_str(), strClassName.c_str());
			continue;
		}
		CMyClass * pMyClass = itMapClass->second;

		CMyDevice * pMyDevice = NULL;
		map<string, CMyDevice *>::iterator itMapDev = m_mapName2Device.find(strDeviceName); // 这个设备不存在，那么可能是设备ID为负值，是几种特殊的情况（计算变量、累计时间变量、累计次数变量等）
		if (itMapDev == m_mapName2Device.end())
		{
			g_logger.LogMessage(PK_LOGLEVEL_INFO, "对象:%s 对应的设备名称:%s 未配置或配置错误，该对象将不直接关联到设备!", strName.c_str(), strDeviceName.c_str());
			// continue;
			pMyDevice = NULL;
		}
		else
			pMyDevice = itMapDev->second;

		CMyObject *pMyObject = new CMyObject();
		pMyObject->nId = ::atoi(strId.c_str());
		pMyObject->strName = strName;
		pMyObject->strDescription = strDesc;
		pMyObject->strDriverName = strDriverName;
		pMyObject->strSubsysName = strSubsysNme;
		pMyObject->nSubsysId = ::atoi(strSubsysId.c_str());
		pMyObject->strDeviceName = strDeviceName;
		pMyObject->nDeviceId = ::atoi(strDeviceId.c_str());
		pMyObject->strParam1 = strParam1;
		pMyObject->strParam2 = strParam2;
		pMyObject->strParam3 = strParam3;
		pMyObject->strParam4 = strParam4;
		pMyObject->pClass = pMyClass;
		pMyObject->m_pDevice = pMyDevice;
		m_mapName2Object[strName] = pMyObject;

		GenerateObjectAndPropAndAlarmsFromClass(pMyObject);

		// 为每个对象增加报警类型属性
		for (map<string, CAlarmType *>::iterator it = m_mapName2AlarmType.begin(); it != m_mapName2AlarmType.end(); it++)
		{
			string strAlarmTypeName = it->first;
			CAlarmInfo *pAlarmInfo = new CAlarmInfo();
			pMyObject->m_mapAlarmTypeName2Info[strAlarmTypeName] = pAlarmInfo;
		}
	}

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "读取到的对象个数(read object count):%d个!", m_mapName2Object.size());
	return PK_SUCCESS;
}

int CMainTask::LoadObjectPropConfig(CPKEviewDbApi &PKEviewDbApi)
{
	int nRet = PK_SUCCESS;

	// 查询tag点
	long lQueryId = -1;
	char szSql[2048] = { 0 };
	vector<vector<string> > vecRow;

	int nPropChangeCount = 0;

	if (m_nVersion < VERSION_NODESERVER_SUPPORT_MULTINODE)
		sprintf(szSql, "select OBJ.name as object_name,PROP.name as class_prop_name,A.object_prop_field_name as object_prop_field_name,A.object_prop_field_value as object_prop_field_value \
		from t_object_prop A LEFT JOIN t_object_list OBJ on A.object_id = OBJ.id \
		LEFT JOIN t_class_prop PROP on A.class_prop_id = PROP.id where (OBJ.enable is NULL or OBJ.enable<>0 or OBJ.enable='')");
	else
		sprintf(szSql, "select OBJ.name as object_name,PROP.name as class_prop_name,A.object_prop_field_name as object_prop_field_name,A.object_prop_field_value as object_prop_field_value\
			FROM t_object_prop A \
			LEFT JOIN t_object_list OBJ on A.object_id = OBJ.id\
			LEFT JOIN t_class_prop PROP on A.class_prop_id = PROP.id\
			LEFT JOIN t_class_list CLS ON OBJ.class_id = CLS.id\
			WHERE(CLS.node_id is NULL or CLS.node_id = 0 or CLS.node_id = '') AND (CLS.enable is NULL or CLS.enable <>0 or CLS.enable = '') and (OBJ.enable is NULL or OBJ.enable<>0 or OBJ.enable='')");

	string strError;
	nRet = PKEviewDbApi.SQLExecute(szSql, vecRow, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "fail to read class info from database:%s, return:%d, error:%s", szSql, nRet, strError.c_str());
		return -2;
	}

	for (int iTab = 0; iTab < vecRow.size(); iTab++)
	{
		vector<string> &row = vecRow[iTab];
		int iCol = 0;
		string strObjectName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strClassPropName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strObjectPropKey = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strObjectPropValue = NULLASSTRING(row[iCol].c_str());
		iCol++;

		if (strObjectName.empty() || strClassPropName.empty() || strObjectPropKey.empty())
			continue;

		map<string, CMyObject *>::iterator itMap = m_mapName2Object.find(strObjectName);
		if (itMap == m_mapName2Object.end()) // 不存在该设备，退出
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "LoadObjectPropChangeConfig, 对象名称为:%s 不存在，对象属性：%s, key：%s, value:%s ", strObjectName.c_str(), strClassPropName.c_str(), strObjectPropKey.c_str(), strObjectPropValue.c_str());
			continue;
		}
		CMyObject *pMyObject = itMap->second;

		map<string, CMyObjectProp *>::iterator itMapProp = pMyObject->mapProps.find(strClassPropName);
		if (itMapProp == pMyObject->mapProps.end()) // 不存在该设备属性，增加之
		{
			g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "对象名称为:%s 属性不存在，对象属性：%s, key：%s, value:%s ", strObjectName.c_str(), strClassPropName.c_str(), strObjectPropKey.c_str(), strObjectPropValue.c_str());
			continue;
		}
		
		CMyObjectProp * pObjectProp = itMapProp->second;
		nRet = ModifyObjectProp(pObjectProp, strObjectPropKey, strObjectPropValue);
		if (nRet == PK_SUCCESS)
			nPropChangeCount++;
	}

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "读取到的对象属性个数:%d, 有效个数:%d, 无效个数:%d", vecRow.size(), nPropChangeCount, vecRow.size() - nPropChangeCount);
	return PK_SUCCESS;
}

// 加载对象属性自己的报警配置
int CMainTask::LoadObjectPropAlarmConfig(CPKEviewDbApi &PKEviewDbApi)
{
	int nRet = PK_SUCCESS;

	// 查询tag点
	long lQueryId = -1;
	char szSql[2048] = { 0 };
	vector<vector<string> > vecRow;

	int nPropChangeCount = 0;
	if (m_nVersion < VERSION_NODESERVER_SUPPORT_MULTINODE)
		sprintf(szSql, "SELECT OBJ.name as object_name,PROP.name as class_prop_name,A.judgemethod as judgemethod,A.alarm_field_name as alarm_field_name,A.alarm_field_value as alarm_field_value \
			FROM t_object_prop_alarm A\
			LEFT JOIN t_object_list OBJ on A.object_id = OBJ.id \
			LEFT JOIN t_class_prop PROP on A.class_prop_id = PROP.id \
			WHERE (OBJ.enable is NULL or OBJ.enable<>0 or OBJ.enable='')");
	else
		sprintf(szSql, "SELECT OBJ.name as object_name,PROP.name as class_prop_name,A.judgemethod as judgemethod,A.alarm_field_name as alarm_field_name,A.alarm_field_value as alarm_field_value \
			FROM t_object_prop_alarm A\
			LEFT JOIN t_object_list OBJ on A.object_id = OBJ.id\
			LEFT JOIN t_class_prop PROP on A.class_prop_id = PROP.id\
			LEFT JOIN t_class_list CLS on CLS.id = PROP.class_id\
			WHERE (OBJ.enable is NULL or OBJ.enable<>0 or OBJ.enable='') AND (CLS.node_id is NULL or CLS.node_id = 0 or CLS.node_id = '') AND (CLS.enable is NULL or CLS.enable <>0 or CLS.enable = '')");

	string strError;
	nRet = PKEviewDbApi.SQLExecute(szSql, vecRow, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "fail to read class info from database:%s, return:%d, error:%s", szSql, nRet, strError.c_str());
		return -2;
	}

	int nAddObjProp = 0;
	int nModifyObjProp = 0;
	for (int iTab = 0; iTab < vecRow.size(); iTab++)
	{
		vector<string> &row = vecRow[iTab];
		int iCol = 0;
		string strObjectName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strClassPropName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strJudgeMethod = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strObjectPropFieldKey = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strObjectPropFieldValue = NULLASSTRING(row[iCol].c_str());
		iCol++;

		if (strObjectName.empty() || strClassPropName.empty() || strJudgeMethod.empty() || strObjectPropFieldKey.empty())
			continue;

		map<string, CMyObject *>::iterator itMap = m_mapName2Object.find(strObjectName);
		if (itMap == m_mapName2Object.end()) // 不存在该设备，增加之
		{
			g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "LoadObjectPropAlarmConfig, 对象名称为:%s 不存在，对象属性：%s, key：%s, value:%s ", strObjectName.c_str(), strClassPropName.c_str(), strObjectPropFieldKey.c_str(), strObjectPropFieldValue.c_str());
			continue;
		}
		CMyObject *pMyObject = itMap->second;

		map<string, CMyObjectProp *>::iterator itMapProp = pMyObject->mapProps.find(strClassPropName);
		if (itMapProp == pMyObject->mapProps.end()) // 不存在该设备，增加之
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "加载对象报警配置, 对象名称为:%s 的类属性属性：%s 未配置, key：%s, value:%s ", strObjectName.c_str(), strClassPropName.c_str(), strObjectPropFieldKey.c_str(), strObjectPropFieldValue.c_str());
			continue;
		}

		CMyObjectProp * pObjectProp = itMapProp->second;
		nRet = ModifyObjectPropAlarm(pObjectProp, strJudgeMethod, strObjectPropFieldKey, strObjectPropFieldValue, nAddObjProp, nModifyObjProp);
		if (nRet == PK_SUCCESS)
			nPropChangeCount++;
	}

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "读取到的对象报警个数:%d, 增加对象数:%d, 修改对象数:%d", vecRow.size(), nAddObjProp, nModifyObjProp);
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "read Object Prop Alarm count:%d!", nPropChangeCount);
	return PK_SUCCESS;
}


// 根据类的属性和报警配置，生成对象的属性和报警配置
int CMainTask::GenerateObjectAndPropAndAlarmsFromClass(CMyObject *pObject)
{
	CMyClass *pClass = pObject->pClass;
	if (!pClass)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "对象:%s 对应的类指针不存在", pObject->strName.c_str());
		return -1;
	}

	return GenerateObjectAndPropAndAlarmsFromClass(pObject, pClass);
}

// 根据非主要类的属性和报警配置，生成对象的属性和报警配置
// 用在增加connstatus和enableconnect上
int CMainTask::GenerateObjectAndPropAndAlarmsFromClass(CMyObject *pObject, CMyClass *pClass)
{
	map<string, CMyClassProp *> & mapClsProp = pClass->m_mapClassProp;
	for (map<string, CMyClassProp *>::iterator itObjProp = mapClsProp.begin(); itObjProp != mapClsProp.end(); itObjProp++)
	{
		CMyClassProp * pClsProp = itObjProp->second;
		if (pObject->mapProps.find(pClass->m_strName) != pObject->mapProps.end()) // 已经具有该属性了，那么不再继续添加。用在增加connstatus和enableconnect上
			continue;

		// 复制属性信息
		CMyObjectProp *pObjProp = new CMyObjectProp();
		pObjProp->m_pObject = pObject;
		pObjProp->n_pClassProp = pClsProp;
		pObject->mapProps[pClsProp->m_strName] = pObjProp;

		// 属性赋值
		pObjProp->m_bEnableControl = pClsProp->m_bEnableControl;
		pObjProp->m_nByteOrderNo = pClsProp->m_nByteOrderNo;
		//pObjProp->m_nDataLen = pClsProp->m_nDataLen;
		pObjProp->m_strDesc = pClsProp->m_strDesc;
		pObjProp->m_strParam = pClsProp->m_strParam;
		pObjProp->m_strAddress = pClsProp->m_strAddress;
		//pObjProp->m_strDataType = pClsProp->m_strDataType;
		pObjProp->m_strSubsysName = pClsProp->m_strSubsysName;
		pObjProp->m_strHisDataInterval = pClsProp->m_strHisDataInterval;
		pObjProp->m_strTrendDataInterval = pClsProp->m_strTrendDataInterval;
		pObjProp->m_strInitValue = pClsProp->m_strInitValue;
		pObjProp->m_nPropTypeId = pClsProp->m_nPropTypeId;

		// 复制报警信息
		map<string, CMyClassPropAlarm *> &mapClassAlarms = pClsProp->m_mapAlarms;
		map<string, CMyObjectPropAlarm *> &mapObjAlarms = pObjProp->m_mapAlarms;
		for (map<string, CMyClassPropAlarm *>::iterator itAlarm = mapClassAlarms.begin(); itAlarm != mapClassAlarms.end(); itAlarm++)
		{
			CMyClassPropAlarm * pClassAlarm = itAlarm->second;
			CMyObjectPropAlarm *pObjAlarm = new CMyObjectPropAlarm();
			pObjAlarm->m_pObjectProp = pObjProp;
			pObjAlarm->m_nId = 0;
			pObjAlarm->m_nLevel = pClassAlarm->m_nLevel;
			pObjAlarm->m_bEnable = pClassAlarm->m_bEnable;
			pObjAlarm->m_strDescription = pClassAlarm->m_strDescription;
			pObjAlarm->m_strJudgeMethod = pClassAlarm->m_strJudgeMethod;
			pObjAlarm->m_strAlarmTypeName = pClassAlarm->m_strAlarmTypeName;
			pObjAlarm->strThresholds = pClassAlarm->strThresholds;
			mapObjAlarms[pObjAlarm->m_strJudgeMethod] = pObjAlarm;
		}
	}
	return 0;
}

int CMainTask::ModifyObjectProp(CMyObjectProp * pObjectProp, string strPropKey, string strPropValue)
{
	if (PKStringHelper::StriCmp(strPropKey.c_str(), "address") == 0)
	{
		pObjectProp->m_strAddress = strPropValue;
	}
	else if (PKStringHelper::StriCmp(strPropKey.c_str(), "pollrate") == 0)
	{
		pObjectProp->m_nPollRateMS = ::atoi(strPropValue.c_str());
	}	
	else
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "ModifyObjectProp, 对象:%s, 属性:%s,要修改的字段:%s 不支持!", 
			pObjectProp->m_pObject->strName.c_str(), pObjectProp->n_pClassProp->m_strName.c_str(), strPropKey.c_str());
		return -1;
	}

	return 0;
}

int CMainTask::ModifyObjectPropAlarm(CMyObjectProp * pObjectProp, string strJudgeMethod, string strPropFieldKey, string strPropFieldValue, int &nAddObjProp, int &nModifyObjProp)
{
	CMyObjectPropAlarm *pAlarm = NULL;
	map<string, CMyObjectPropAlarm *>::iterator itAlarm = pObjectProp->m_mapAlarms.find(strJudgeMethod);
	if (itAlarm == pObjectProp->m_mapAlarms.end())
	{
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "ModifyObjectPropAlarm, 对象:%s, 属性:%s, 判断方法:%s, 要修改的字段:%s, 值:%s, 类中未配置此类型报警，增加到对象!",
			pObjectProp->m_pObject->strName.c_str(), pObjectProp->n_pClassProp->m_strName.c_str(), strJudgeMethod.c_str(), strPropFieldKey.c_str(), strPropFieldValue.c_str());
		
		// 自动加入对象的报警配置
		CMyObjectPropAlarm *pObjAlarm = new CMyObjectPropAlarm();
		pObjAlarm->m_pObjectProp = pObjectProp;
		pObjAlarm->m_nId = 0;
		pObjAlarm->m_nLevel = 0;
		pObjAlarm->m_bEnable = false;
		pObjAlarm->m_strDescription = "";
		pObjAlarm->m_strJudgeMethod = strJudgeMethod;
		pObjAlarm->m_strAlarmTypeName = "";
		pObjAlarm->strThresholds = "";
		pObjAlarm->m_strPropName = pObjectProp->n_pClassProp->m_strName;
		pObjectProp->m_mapAlarms[pObjAlarm->m_strJudgeMethod] = pObjAlarm;

		pAlarm = pObjAlarm;
		nAddObjProp++;
	}
	else
	{
		pAlarm = itAlarm->second;
		nModifyObjProp++;
	}
	if (PKStringHelper::StriCmp(strPropFieldKey.c_str(), "description") == 0)
		pAlarm->m_strDescription = strPropFieldValue;
	else if (PKStringHelper::StriCmp(strPropFieldKey.c_str(), "alarmtype") == 0)
		pAlarm->m_strAlarmTypeName = strPropFieldValue;
	else if (PKStringHelper::StriCmp(strPropFieldKey.c_str(), "enable") == 0)
		pAlarm->m_bEnable = ::atoi(strPropFieldValue.c_str());
	else if (PKStringHelper::StriCmp(strPropFieldKey.c_str(), "priority") == 0)
		pAlarm->m_nLevel = ::atoi(strPropFieldValue.c_str());
	else if (PKStringHelper::StriCmp(strPropFieldKey.c_str(), "threshold") == 0)
		pAlarm->strThresholds = strPropFieldValue;
	else if (PKStringHelper::StriCmp(strPropFieldKey.c_str(), "level") == 0)
		pAlarm->m_nLevel = ::atoi(strPropFieldValue.c_str());
	else{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "ModifyObjectPropAlarm, 对象:%s, 属性:%s, 判断方法:%s, 要修改的字段:%s, 值:%s, 修改的字段不支持!",
			pObjectProp->m_pObject->strName.c_str(), pObjectProp->n_pClassProp->m_strName.c_str(), strJudgeMethod.c_str(), strPropFieldKey.c_str(), strPropFieldValue.c_str());
		return -2;
	}
	return 0;
}

int CMainTask::GenerateTagsFromObjProps()
{
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "GenerateTagsFromObjProps, 对象个数:%d", m_mapName2Object.size());
	int nTagNum = 0;
	for (map<string, CMyObject *>::iterator itObject = m_mapName2Object.begin(); itObject != m_mapName2Object.end(); itObject++)
	{
		CMyObject *pObject = itObject->second;
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "对象:%s,属性个数:%d", pObject->strName.c_str(), pObject->mapProps.size());
		int nPropIndex = 0;
		map<string, CMyObjectProp *> & mapObjProp = pObject->mapProps;
		for (map<string, CMyObjectProp *>::iterator itObjProp = pObject->mapProps.begin(); itObjProp != pObject->mapProps.end(); itObjProp++)
		{
			// 复制属性信息
			CMyObjectProp * pObjProp = itObjProp->second;
			CMyObject *pObject = pObjProp->m_pObject;
			string strTagName = pObjProp->m_pObject->strName + "." + pObjProp->n_pClassProp->m_strName;
			string strTagId = "0";
			nPropIndex++;
			//printf("object:%s,prop:%s,prop index:%d\n", pObject->strName.c_str(), pObjProp->n_pClassProp->m_strName.c_str(), nPropIndex);

			// 判断tag是不是模拟的. 二级变量没有设备，且不会是模拟的
			bool bSimulateTag = false;
			if (pObject->m_pDevice && !pObject->m_pDevice->m_strSimulate.empty())
				bSimulateTag = true;

			CDataTag *pDataTag = NULL;
			//根据表达式计算
			if (pObjProp->m_nPropTypeId == TAGTYPE_CALC)	// 根据device_id进行判断，如果是二次变量点
			{
				pDataTag = new CCalcTag();
				pDataTag->nTagType = VARIABLE_TYPE_CALC;
				((CCalcTag *)pDataTag)->strAddress = pObjProp->m_strAddress;
				pDataTag->pDevice = m_pSimDevice; // 模拟变量
				m_mapName2CalcTag[strTagName] = (CCalcTag*)pDataTag; // 加入表达式
			}
			//累计变量满足条件的时间
			else if (pObjProp->m_nPropTypeId == TAGTYPE_ACCUMULATE_TIME)	// 根据device_id进行判断，如果是累计时间变量点
			{
				pDataTag = new CTagAccumulateTime();
				pDataTag->nTagType = VARIABLE_TYPE_ACCUMULATE_TIME;
				((CTagAccumulate *)pDataTag)->strAddress = pObjProp->m_strAddress;
				pDataTag->pDevice = m_pSimDevice; // 模拟变量
				m_mapName2CalcTag[strTagName] = (CCalcTag*)pDataTag; // 加入表达式
				m_mapName2AccumlateTime[strTagName] = (CTagAccumulateTime*)pDataTag; // 加入表达式
			}
			//累计变量满足条件的次数
			else if (pObjProp->m_nPropTypeId == TAGTYPE_ACCUMULATE_COUNT)	// 根据device_id进行判断，如果是累计次数变量点
			{
				pDataTag = new CTagAccumulateCount();
				pDataTag->nTagType = VARIABLE_TYPE_ACCUMULATE_COUNT;
				pDataTag->pDevice = m_pSimDevice; // 模拟变量
				m_mapName2CalcTag[pDataTag->strName] = (CCalcTag*)pDataTag; // 加入表达式
				m_mapName2AccumlateCount[strTagName] = (CTagAccumulateCount*)pDataTag; // 加入表达式
				
				((CTagAccumulateCount *)pDataTag)->strAddress = pObjProp->m_strAddress;
			}
			else if (pObjProp->m_nPropTypeId == TAGTYPE_MEMVAR)	// 内存变量
			{
				pDataTag = new CMemTag();
				pDataTag->nTagType = VARIABLE_TYPE_MEMVAR;
				pDataTag->pDevice = m_pMemDevice;
				m_mapName2MemTag[strTagName] = (CMemTag *)pDataTag;
			}
			else if (pObjProp->m_nPropTypeId == TAGTYPE_SIMULATE || bSimulateTag) // 模拟量
			{
				pDataTag = new CSimTag();
				pDataTag->nTagType = VARIABLE_TYPE_SIMULATE;
				pDataTag->pDevice = m_pSimDevice;
				m_mapName2SimTag[strTagName] = (CSimTag *)pDataTag;
			}
			else if (pObjProp->m_nPropTypeId == TAGTYPE_DEVICE) // 模拟量	// 设备变量
			{
				CDeviceTag *pDeviceTag = new CDeviceTag();
				pDataTag = pDeviceTag;
				pDataTag->nTagType = VARIABLE_TYPE_DEVICE;
				pDataTag->pDevice = pObject->m_pDevice;
				pDeviceTag->strAddress = pObjProp->m_strAddress;

				m_mapName2DeviceTag[strTagName] = pDeviceTag;
				if(pDeviceTag->pDevice)
					pDeviceTag->pDevice->m_vecDeviceTag.push_back(pDeviceTag);
				// 将tag点增加到驱动map中，以便于在驱动断开时，可以设置所有变量质量为BAD
			}
			else
			{
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "对象属性:%s, 属性类型不正确:%d", strTagName.c_str(), pObjProp->m_nPropTypeId);
				continue;
			}

			pDataTag->pObjProp = pObjProp;
			m_mapName2Tag[strTagName] = pDataTag;

			pDataTag->strName = strTagName;
			pDataTag->pObject = pObject;
			pDataTag->nSubsysID = pObject->nSubsysId;
			pDataTag->nLabelID = pObjProp->n_pClassProp->m_nLabelId;
 
			pDataTag->strName = strTagName;			
			//pDataTag->strDataType = pObjProp->m_strDataType;
			pDataTag->strDesc = pObjProp->m_strDesc;
			pDataTag->strSubsysName = pObjProp->m_strSubsysName;
			pDataTag->strInitValue = pObjProp->m_strInitValue;
			pDataTag->nPollrateMS = pObjProp->m_nPollRateMS;

			pDataTag->nTagDataLen = pObjProp->n_pClassProp->m_nDataLen; // 字节个数
			pDataTag->nDataType = pObjProp->n_pClassProp->m_nDataType;
			pDataTag->nDataTypeClass = pObjProp->n_pClassProp->m_nDataTypeClass;
			//int nDataType;
			//int nDataTypeLenBits;
			//PKMiscHelper::GetTagDataTypeAndLen(pDataTag->strDataType.c_str(), strTagDataLen.c_str(), &nDataType, &nDataTypeLenBits);
			//if (nDataType == TAG_DT_UNKNOWN)
			//{
			//	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "tag:%s datatype:%s not supported, assume int32!!", pDataTag->strName.c_str(), pDataTag->strDataType.c_str());
			//	nDataType = TAG_DT_INT32;
			//	nDataTypeLenBits = 32; // 32位4个字节
			//}
			//pDataTag->nTagDataLen = (nDataTypeLenBits + 7) / 8; // 字节个数
			//pDataTag->nDataType = nDataType;
			//pDataTag->nDataTypeClass = CDataTag::GetDataTypeClass(nDataType);


			// 生成报警
			GenerateTagAlarmsFromObjProp(pObjProp, pDataTag);
			nTagNum++;
		}
	}

	m_nTagNum += nTagNum;
	return 0;
}

int CMainTask::GenerateTagAlarmsFromObjProp(CMyObjectProp *pObjProp, CDataTag *pDataTag)
{
	map<string, CMyObjectPropAlarm *>::iterator itAlarm = pObjProp->m_mapAlarms.begin();
	for (; itAlarm != pObjProp->m_mapAlarms.end(); itAlarm++)
	{
		CMyObjectPropAlarm *pObjPropAlarm = itAlarm->second;
		CAlarmTag *pAlarmTag = new CAlarmTag();
		pAlarmTag->pTag = pDataTag;
		string strAlarmTagName = pDataTag->strName + "." + pObjPropAlarm->m_strJudgeMethod; // 报警名称需要拼接为： tagname.alm_LL.....
		pAlarmTag->bEnable = pObjPropAlarm->m_bEnable;
		pAlarmTag->nLevel = pObjPropAlarm->m_nLevel;
		pAlarmTag->strAlarmTypeName = pObjPropAlarm->m_strAlarmTypeName;
		pAlarmTag->strJudgeMethod = pObjPropAlarm->m_strJudgeMethod;
		CAlarmTag::GetJudgeMethodTypeAndIndex(pObjPropAlarm->m_strJudgeMethod.c_str(), pAlarmTag->nJudgeMethodType, pAlarmTag->strJudgeMethodIndex);
		pAlarmTag->SplitThresholds(pObjPropAlarm->strThresholds.c_str()); // 将threshold进行切分		

		pDataTag->vecAllAlarmTags.push_back(pAlarmTag);

		m_mapName2AlarmTag[strAlarmTagName] = pAlarmTag; // 仅仅在程序启动，加载t_alarm_real时，检查是否有该报警时才有用
	}
	return PK_SUCCESS;
}
int CMainTask::LoadClassAlarmConfig(CPKEviewDbApi &PKEviewDbApi)
{
	int nRet = PK_SUCCESS;

	// 查询tag点
	long lQueryId = -1;
	char szSql[2048] = { 0 };
	vector<vector<string> > vecRow;

	int nTotalAlarmNum = 0;
	sprintf(szSql, "SELECT A.id as alarm_id,A.prop_id as prop_id, A.judgemethod as judgemethod, A.alarmtype_id as alarmtype_id,A.description as description,\
		A.enable as enable, A.priority as priority, A.threshold as threshold, ALMTYPE.name as alarmtype_name,\
		CLSPROP.name as classprop_name, CLS.name as class_name, A.deadtime_produce, A.deadtime_recover\
		FROM t_class_prop_alarm A\
		LEFT JOIN t_class_prop CLSPROP on CLSPROP.id = A.prop_id\
		LEFT JOIN t_class_list CLS on CLSPROP.class_id = CLS.id\
		LEFT JOIN t_alarm_type ALMTYPE on ALMTYPE.id = A.alarmtype_id");
	if (m_nVersion >= VERSION_NODESERVER_SUPPORT_MULTINODE)
		strcat(szSql, " WHERE (CLS.node_id is NULL or CLS.node_id=0 or CLS.node_id='') AND (CLS.enable is NULL or CLS.enable <>0 or CLS.enable='')");

	string strError; 
	nRet = PKEviewDbApi.SQLExecute(szSql, vecRow, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "LoadClassAlarmConfig, fail to query database:%s, return:%d, error:%s", szSql, nRet, strError.c_str());
		return -2;
	}

	for (int iTab = 0; iTab < vecRow.size(); iTab++)
	{
		vector<string> &row = vecRow[iTab];
		int iCol = 0;
		string strAlarmId = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strPropId = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strJudgeMethod = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strAlarmTypeId = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDescription = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strEnable = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strPriority = NULLASSTRING(row[iCol].c_str());
		iCol++;

		string strThreshold = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strAlarmTypeName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strClassPropName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strClassName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDeadtimeProduce = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDeadtimeRecover = NULLASSTRING(row[iCol].c_str());
		iCol++;
		// 类名称和属性名称不能为空
		if (strAlarmId.empty() || strClassName.empty() || strClassPropName.empty())
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "属性:%s 对应的类:%s,类属性:%s, 为空", strDescription.c_str(), strClassName.c_str(), strClassPropName.c_str());
			continue;
		}

		// 根据名称找到类
		map<string, CMyClass *>::iterator itMapClass = m_mapName2Class.find(strClassName); // 这个类不存在，那么可能是类定义错误了，生成这个类！
		if (itMapClass == m_mapName2Class.end())
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "类报警:%s,属性:%s 对应的类:%s 不存在 ", strDescription.c_str(), strClassPropName.c_str(), strClassName.c_str());
			continue;
		}
		CMyClass * pMyClass = itMapClass->second;
		// 检查属性是否已经存在，如果已经存在则不需要增加
		map<string, CMyClassProp *>::iterator itMapProp = pMyClass->m_mapClassProp.find(strClassPropName);
		if (itMapProp == pMyClass->m_mapClassProp.end())
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "类报警:%s,属性:%s 对应的类:%s 中已具有该属性", strDescription.c_str(), strClassPropName.c_str(), strClassName.c_str());
			continue;
		}
		CMyClassProp *pClsProp = itMapProp->second;

		CMyClassPropAlarm *pClsPropAlarm = new CMyClassPropAlarm();
		pClsPropAlarm->m_pClassProp = pClsProp;
		pClsPropAlarm->m_bEnable = ::atoi(strEnable.c_str());
		pClsPropAlarm->m_nId = ::atoi(strAlarmId.c_str());
		pClsPropAlarm->m_nLevel = ::atoi(strPriority.c_str());
		pClsPropAlarm->m_strAlarmTypeName = strAlarmTypeName;
		pClsPropAlarm->m_strJudgeMethod = strJudgeMethod;
		pClsPropAlarm->m_strDescription = strDescription;
		pClsPropAlarm->strThresholds = strThreshold;
		pClsProp->m_mapAlarms[strJudgeMethod] = pClsPropAlarm; // 以judgemethod作为key加入到map中去
		nTotalAlarmNum++;
	}

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "read class prop alarm count:%d!", nTotalAlarmNum);

	return 0;
}

// 查询报警类型信息
int CMainTask::LoadAlarmTypeConfig(CPKEviewDbApi &PKEviewDbApi)
{
	int nRet = PK_SUCCESS;

	// 查询指定的驱动列表
	char szSql[2048] = { 0 };
	sprintf(szSql, "SELECT id,name,description,autoconfirmalarm FROM t_alarm_type");
	vector<vector<string> > vecRow;
	string strError; 
	nRet = PKEviewDbApi.SQLExecute(szSql, vecRow, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "查询报警类型表失败，请检查是否有该表，及无此表是否会对应用造成影响！Query AlarmType Database failed:%s!(查询数据库失败), error:%s", szSql, strError.c_str());
		return -2;
	}

	// 非禁用和非模拟的设备信息
	for (int iTab = 0; iTab < vecRow.size(); iTab++)
	{
		vector<string> &row = vecRow[iTab];
		int iCol = 0;
		string strId = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDesc = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strAutoConfirmAlarm = NULLASSTRING(row[iCol].c_str());
		iCol++;

		bool bAutoConfirmAlarm = false;
		if (strAutoConfirmAlarm.length() <= 0)
			bAutoConfirmAlarm = false;
		else
		{
			int nAutoConfirm = ::atoi(strAutoConfirmAlarm.c_str());
			if (nAutoConfirm != 0)
				bAutoConfirmAlarm = true;
		}

		map<string, CAlarmType *>::iterator it = m_mapName2AlarmType.find(strName);
		if (it != m_mapName2AlarmType.end())
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "报警类型 名称 重复了:%s!", strName.c_str());
			continue;
		}

		CAlarmType *pAlarmType = new CAlarmType();
		pAlarmType->m_strId = strId;
		pAlarmType->m_strName = strName;
		pAlarmType->m_strDescription = strDesc;
		pAlarmType->m_bAutoConfirm = bAutoConfirmAlarm;
		m_mapName2AlarmType[strName] = pAlarmType;

		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "报警类型:%s, 描述:%s, 自动确认报警:%s!", strName.c_str(), strDesc.c_str(), bAutoConfirmAlarm?"否":"是");
	}

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "报警类型个数:%d!", m_mapName2AlarmType.size());

	return 0;
}

// 计算所有tag点的大小
int CMainTask::CalcAllTagDataSize()
{
	m_nAllTagDataSize = 0;
	map<string, CDataTag *>::iterator itMapName2Tag = m_mapName2Tag.begin();
	for (; itMapName2Tag != m_mapName2Tag.end(); itMapName2Tag++)
	{
		CDataTag *pTag = itMapName2Tag->second;
		if (pTag->nTagDataLen <= 0)
		{
            g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "tag:%s datalen:%d <= 0!", pTag->strName.c_str(), pTag->nTagDataLen);
			continue;
		}
		m_nAllTagDataSize += pTag->nTagDataLen;
	}
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "CalcTagDataSize, TagCount:%d, AllTagSize:%d", m_mapName2Tag.size(), m_nAllTagDataSize);

	return 0;
}

int CMainTask::CreateProcessQueue() // 创建共享内存队列
{
	int nQueBufSize = m_nAllTagDataSize * 2;
	if (nQueBufSize < PROCESSQUEUE_DATA_TO_NODESERVER_AREASIZE_MIN)
		nQueBufSize = PROCESSQUEUE_DATA_TO_NODESERVER_AREASIZE_MIN;
	if (nQueBufSize > PROCESSQUEUE_DATA_TO_NODESERVER_AREASIZE_MAX)
		nQueBufSize = PROCESSQUEUE_DATA_TO_NODESERVER_AREASIZE_MAX;
	m_nNodeServerQueSize = nQueBufSize;

	g_pQueData2NodeSrv = new CProcessQueue(MODULENAME_LOCALSERVER);
	int nRet = g_pQueData2NodeSrv->Open(true, nQueBufSize); // 创建40M的共享内大小，方便传输数据
	if (nRet != 0)
	{
        g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Create sharedmemory Queue to RecvData failed, ret:%d, QueueBuffSize:%d, AllTagSize:%d", nRet, nQueBufSize, m_nAllTagDataSize);
		return nRet;
	}
	else
        g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "Create sharedmemory Queue to RecvData Success, QueueBuffSize:%d, AllTagSize:%d", nQueBufSize, m_nAllTagDataSize);

	
	for (map<string, CMyDriver *>::iterator itDriver = m_mapName2Driver.begin(); itDriver != m_mapName2Driver.end(); itDriver++)
	{
		CMyDriver *pDriver = itDriver->second;
		string strDriverModuleName = itDriver->first;
		if (pDriver->m_nType != DRIVER_TYPE_PHYSICALDEVICE)
		{
			continue;
		}
		CMyDriverPhysicalDevice *pPhysicalDrv = (CMyDriverPhysicalDevice *)pDriver;

		// 创建接收控制命令的共享队列
		CProcessQueue *pDrvShm = new CProcessQueue((char *)strDriverModuleName.c_str());
		pPhysicalDrv->m_pDrvShm = pDrvShm;
        unsigned int nQueToDrvBufSize = PROCESSQUEUE_CONTROL_TO_DRIVER_AREASIZE;
        nRet = pDrvShm->Open(true, nQueToDrvBufSize); // 只打开/不创建，在驱动那里创建比较合适！
		if(nRet != PK_SUCCESS)
            g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Open or Create sharedmemory Queue For Drv(%s) To RecvControl failed, ret:%d, size:%d", strDriverModuleName.c_str(), nRet, nQueToDrvBufSize);
		else
            g_logger.LogMessage(PK_LOGLEVEL_INFO, "Open or Create sharedmemory Queue For Drv(%s) To RecvControl Success, size:%d", strDriverModuleName.c_str(), nQueToDrvBufSize);
	}
	return 0;
}
