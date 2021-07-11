#include "Driver.h"
#include "TaskGroup.h"
#include "pkcomm/pkcomm.h"
#include "MainTask.h"
#include "pklog/pklog.h"
#include "pkDriverCommon.h"
#include "shmqueue/ProcessQueue.h"
#include "common/eviewcomm_internal.h"

extern CPKLog g_logger;
extern void PKDEVICE_Reset(PKDEVICE *pDevice);
extern void PKDRIVER_Reset(PKDRIVER *pDriver);
extern CProcessQueue *g_pQueData2NodeServer;

CDriver::CDriver(void)
{
	m_bAutoDeleted = true;	// 析构时要删掉成员Grp
	m_pOutDriverInfo = new PKDRIVER();
	PKDRIVER_Reset(m_pOutDriverInfo);
	Py_CDriverConstructor(this);
}

void CDriver::Copy2OutDriverInfo()
{
	m_pOutDriverInfo->_pInternalRef = (void *)this;
	PKStringHelper::Safe_StrNCpy((char *)m_pOutDriverInfo->szName, m_strName.c_str(), PK_NAME_MAXLEN);
	PKStringHelper::Safe_StrNCpy((char *)m_pOutDriverInfo->szDesc, m_strDesc.c_str(), PK_DESC_MAXLEN);

	PKStringHelper::Safe_StrNCpy((char *)m_pOutDriverInfo->szParam1, m_strParam1.c_str(), PK_PARAM_MAXLEN);
	PKStringHelper::Safe_StrNCpy((char *)m_pOutDriverInfo->szParam2, m_strParam2.c_str(), PK_PARAM_MAXLEN);
	PKStringHelper::Safe_StrNCpy((char *)m_pOutDriverInfo->szParam3, m_strParam3.c_str(), PK_PARAM_MAXLEN);
	PKStringHelper::Safe_StrNCpy((char *)m_pOutDriverInfo->szParam4, m_strParam4.c_str(), PK_PARAM_MAXLEN);

	int nDeviceNum = 0;
	std::map<std::string, CTaskGroup*>::iterator iterDevGrp = m_mapTaskGroups.begin();
	for (; iterDevGrp != m_mapTaskGroups.end(); iterDevGrp++)
	{
		CTaskGroup * pTaskGrp = (CTaskGroup *)iterDevGrp->second;
		std::map<std::string, CDevice*>::iterator itDev = pTaskGrp->m_mapDevices.begin();
		for (; itDev != pTaskGrp->m_mapDevices.end(); itDev++)
		{
			nDeviceNum++;
		}
	}

	if (nDeviceNum <= 0)
		return;

	m_pOutDriverInfo->nDeviceNum = nDeviceNum;
	m_pOutDriverInfo->ppDevices = new PKDEVICE*[nDeviceNum];
	memset(m_pOutDriverInfo->ppDevices, 0, sizeof(PKDEVICE *) * nDeviceNum);

	int iDeviceIndex = 0;
	for (iterDevGrp = m_mapTaskGroups.begin(); iterDevGrp != m_mapTaskGroups.end(); iterDevGrp++)
	{
		CTaskGroup * pTaskGrp = (CTaskGroup *)iterDevGrp->second;
		std::map<std::string, CDevice*>::iterator itDev = pTaskGrp->m_mapDevices.begin();
		for (; itDev != pTaskGrp->m_mapDevices.end(); itDev++)
		{
			CDevice *pDevice = itDev->second;
			pDevice->CopyDeviceInfo2OutDevice();
			m_pOutDriverInfo->ppDevices[iDeviceIndex] = pDevice->m_pOutDeviceInfo;
			iDeviceIndex++;
		}
	} 
}

CDriver::~CDriver(void)
{	
	if(m_bAutoDeleted)
	{
		std::map<std::string, CTaskGroup*>::iterator iterDevGrp = m_mapTaskGroups.begin();
		for (; iterDevGrp != m_mapTaskGroups.end(); iterDevGrp++)
		{
			CTaskGroup * pTaskGrp = (CTaskGroup *)iterDevGrp->second;
			SAFE_DELETE(pTaskGrp);
		}
	}

	m_mapTaskGroups.clear();
}


/* 将变量的配置信息写入到Shm中，以便在进行控制时可以快速根据tag点信息找到对应的设备信息
db0:
驱动状态数据，放在db0：key为driver:drivername，value为string，包含 vtq：{t:2015-09-17 18:06:57.329,q:0}）
设备状态数据，放在db0：key为driver:device:devicename，value为string，包含 vtq：{t:2015-09-17 18:06:57.329,q:0}）
所有变量状态，放在db0：key为tagname，value为字符串，包含vtq：{t:2015-09-17 18:06:57.329,q:0}）

db1:
要写入的数据配置数据包括：tag点对应的配置（tagname为key，value为hash表：drivername、devicename、tag点信息等)
*/
int CDriver::InitDriverShmData()
{
	char szTime[PK_HOSTNAMESTRING_MAXLEN] = { '\0' };
	PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));

	int nRet = PK_SUCCESS;
	// 增加驱动的信息
	std::map<std::string, CTaskGroup *>::iterator itTaskGrp = this->m_mapTaskGroups.begin();
	for (; itTaskGrp != this->m_mapTaskGroups.end(); itTaskGrp++)
	{
		CTaskGroup *pTaskGroup = (CTaskGroup *)itTaskGrp->second;

		// 更新设备组运行状态到Shm（线程）
		pTaskGroup->UpdateTaskStatus(TASK_STATUS_INIT_CODE, STATUS_INIT_TEXT, szTime);

		std::map<std::string, CDevice *>::iterator itDevice = pTaskGroup->m_mapDevices.begin();
		for (; itDevice != pTaskGroup->m_mapDevices.end(); itDevice++)
		{
			CDevice *pDevice = (CDevice *)itDevice->second;
			pDevice->InitShmDeviceInfo();
		}
	}

	return nRet;
}

int CDriver::Start()
{
	this->ClearShmDataOfDriver(); // 清空驱动中Shm的实时数据和配置

	int nStatus = InitDriverShmData();
	if (nStatus != PK_SUCCESS)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "初始化过程数据库失败!");
	}

	if (g_pfnInitDriver)
	{
		if ((nStatus = g_pfnInitDriver(this->m_pOutDriverInfo)) != PK_SUCCESS)
		{
			g_logger.LogMessage(PK_LOGLEVEL_CRITICAL, "调用驱动初始化失败, 返回: %d", nStatus);
			return nStatus;
		}
	}

	nStatus = AddTaskGrps(this->m_mapTaskGroups);
	if (nStatus != PK_SUCCESS)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "添加设备组到Shm中失败!");
		return -1;
	}

	// 更新驱动运行状态到Shm
	this->UpdateDriverStartTime(); // 更新启动时间
	this->UpdateDriverDeviceNum(); // 更新设备数量
	// 更新驱动的状态
	char szTime[PK_HOSTNAMESTRING_MAXLEN] = { '\0' };
	PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));
	this->UpdateDriverStatus(DRIVER_STATUS_INIT_CODE, STATUS_INIT_TEXT, szTime);
	return 0;
}

int CDriver::ClearShmDataOfDriver()
{
	int nRet = 0;
	int nDrvNameLen = g_strDrvName.length();
	char szCmd[PROCESSQUEUE_PROCESSMGR_CONTROL_RECORD_LENGTH] = { 0 };
	char *pCmd = szCmd;

	// 放入驱动
	memcpy(pCmd, &nDrvNameLen, sizeof(int));
	pCmd += sizeof(int);
	memcpy(pCmd, g_strDrvName.c_str(), g_strDrvName.length());
	pCmd += g_strDrvName.length();

	PublishClearDrvShm2NodeServer(ACTIONTYPE_DRV2SERVER_CLEAR_DRIVER, szCmd, pCmd - szCmd);
	return nRet;
}

int CDriver::Stop()
{
	// 停止其他线程
	map<string, CTaskGroup*>::iterator iter = this->m_mapTaskGroups.begin();
	for (; iter != this->m_mapTaskGroups.end(); iter++)
	{
		iter->second->Stop();
	}
	this->UpdateDriverStatus(-10000, "stopped", NULL);
	this->ClearShmDataOfDriver();

	Py_UnInitDriver(this);
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "driver %s stop...", g_strDrvName.c_str());
	return 0;
}

int CDriver::AddTaskGrps(map<string, CTaskGroup*> mapAddTaskGrp)
{
	int nStatus = PK_SUCCESS;
	int iThreadNo = 0;
	map<string, CTaskGroup*>::iterator itTaskGrp = mapAddTaskGrp.begin();
	for (; itTaskGrp != mapAddTaskGrp.end(); itTaskGrp++)
	{
		iThreadNo++;
		this->m_mapTaskGroups.insert(map<string, CTaskGroup*>::value_type(itTaskGrp->first, itTaskGrp->second));
		itTaskGrp->second->m_pMainTask = MAIN_TASK;
		CTaskGroup* pTaskGroup = (*itTaskGrp).second;
		int lStarted = pTaskGroup->Start();
		if (lStarted != PK_SUCCESS)
		{
			//记录日志，读取配置文件失败
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "start devicegroup task(%s) failed, return:%d, will delete this device group!", (char *)(*itTaskGrp).second->m_drvObjBase.m_strName.c_str(), lStarted);

			g_logger.LogMessage(PK_LOGLEVEL_WARN, "(AddTaskGrps2 in Shm)delete TaskGroup name: %s,device num:%d", pTaskGroup->m_drvObjBase.m_strName.c_str(), pTaskGroup->m_mapDevices.size());
			SAFE_DELETE(pTaskGroup);

			this->m_mapTaskGroups.erase(itTaskGrp++);
			continue;
		}

		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "===success to start No.%d/%d DeviceGroup THREAD(id:%s), include %d devices!", iThreadNo, mapAddTaskGrp.size(), (char *)pTaskGroup->m_drvObjBase.m_strName.c_str(), pTaskGroup->m_mapDevices.size());
	}

	return PK_SUCCESS;
}

// 更新驱动状态
int CDriver::UpdateDriverStatus(int nStatus, char *szErrTip, char *pszTime)
{
	char szTime[PK_HOSTNAMESTRING_MAXLEN] = {'\0'};
	char *pRealTime = pszTime;
	if(!pRealTime)
	{
		PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));
		pRealTime = szTime;
	}
	char szKey[PK_HOSTNAMESTRING_MAXLEN] = {0};
	PKStringHelper::Snprintf(szKey, sizeof(szKey), "%s:status", g_strDrvName.c_str());
	char szStatus[PROCESSQUEUE_PROCESSMGR_CONTROL_RECORD_LENGTH] = {0};
	sprintf(szStatus, "{\"v\":\"0\",\"t\":\"%s\",\"q\":\"%d\",\"m\":\"%s\"}",pRealTime, nStatus, szErrTip);
	
	int nRet = PublishKVSimple2NodeServer(ACTIONTYPE_DRV2SERVER_SET_KV_SIMPLE, szKey, szStatus, strlen(szStatus));
	// 状态为0表示驱动停止运行，将该驱动的点置空
	if (nStatus == 0)
		ResetTagsValue(g_strDrvName);

	return nRet;
}

// 驱动停止后，将驱动的相关点置空
void CDriver::ResetTagsValue(string g_strDrvName)
{

}

// 更新驱动状态
int CDriver::UpdateDriverOnlineConfigTime(int nStatus, char *szErrTip, char *pszTime)
{
	char szTime[PK_HOSTNAMESTRING_MAXLEN] = {'\0'};
	char *pRealTime = pszTime;
	if(!pRealTime)
	{
		PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));
		pRealTime = szTime;
	}

	char szKey[PK_HOSTNAMESTRING_MAXLEN]={0};
	PKStringHelper::Snprintf(szKey, sizeof(szKey), "%s:lastcfgloadtime", g_strDrvName.c_str());
	char szStatus[PROCESSQUEUE_PROCESSMGR_CONTROL_RECORD_LENGTH] = {0};
	sprintf(szStatus, "{\"v\":\"%d\",\"t\":\"%s\",\"q\":\"%d\"}",nStatus, pRealTime, nStatus, szErrTip);

	int nRet = PublishKVSimple2NodeServer(ACTIONTYPE_DRV2SERVER_SET_KV_SIMPLE, szKey, szStatus, strlen(szStatus));
	return nRet;
}

int CDriver::UpdateDriverStartTime()
{
	char szTime[PK_HOSTNAMESTRING_MAXLEN] = {'\0'};
	PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));

	char szKey[PK_HOSTNAMESTRING_MAXLEN]={0};
	PKStringHelper::Snprintf(szKey, sizeof(szKey), "%s:starttime", g_strDrvName.c_str());
	char szSTT[PROCESSQUEUE_PROCESSMGR_CONTROL_RECORD_LENGTH]={0};
	sprintf(szSTT, "{\"v\":\"%s\",\"t\":\"%s\",\"q\":\"0\"}", szTime, szTime);

	int nRet = PublishKVSimple2NodeServer(ACTIONTYPE_DRV2SERVER_SET_KV_SIMPLE, szKey, szSTT, strlen(szSTT));
	return nRet;
}

int CDriver::UpdateDriverDeviceNum()
{
	char szTime[PK_HOSTNAMESTRING_MAXLEN] = {'\0'};
	PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));

	int nDeviceNum = GetDeviceNum();
	char szKey[PK_HOSTNAMESTRING_MAXLEN]={0};
	PKStringHelper::Snprintf(szKey, sizeof(szKey), "%s:devicecount", g_strDrvName.c_str());
	char szSTT[PROCESSQUEUE_PROCESSMGR_CONTROL_RECORD_LENGTH]={0};
	sprintf(szSTT, "{\"v\":\"%d\",\"t\":\"%s\",\"q\":\"0\"}", nDeviceNum, szTime);
	int nRet = PublishKVSimple2NodeServer(ACTIONTYPE_DRV2SERVER_SET_KV_SIMPLE, szKey, szSTT, strlen(szSTT));
	return nRet;
}

int CDriver::GetDeviceNum()
{
	int nDeviceNum = 0;
	std::map<std::string, CTaskGroup*>::iterator iterDevGrp = m_mapTaskGroups.begin();
	for (; iterDevGrp != m_mapTaskGroups.end(); iterDevGrp++)
	{
		CTaskGroup * pTaskGrp = (CTaskGroup *)iterDevGrp->second;
		nDeviceNum += pTaskGrp->m_mapDevices.size();
	}

	return nDeviceNum;
}

// 将消息发布给NodeServer
// 格式：类型（1字节，tag数据|清除驱动）,数据
// tag数据，json格式
// 清除驱动：驱动名称（32字节）
int CDriver::PublishClearDrvShm2NodeServer(int nActionType, const char *szContent, int nContentLen)
{
	char szData[PROCESSQUEUE_PROCESSMGR_CONTROL_RECORD_LENGTH] = { 0 };
	char *pData = szData;

	// 放入时间戳
	unsigned int nPackTimestampSec = 0;
	unsigned int nPackTimestampMSec = 0;
	nPackTimestampSec = PKTimeHelper::GetHighResTime(&nPackTimestampMSec);
	memcpy(pData, &nPackTimestampSec, sizeof(unsigned int));
	pData += sizeof(unsigned int);
	memcpy(pData, &nPackTimestampMSec, sizeof(unsigned int));
	pData += sizeof(unsigned int);

	// 放入动作类型
	memcpy(pData, &nActionType, sizeof(int));
	pData += sizeof(int);
	memcpy(pData, szContent, nContentLen);
	pData += nContentLen;

	long lRet = 0;
	CProcessQueue *pQue = g_pQueData2NodeServer;
	if (pQue)
	{
		long lRet = pQue->EnQueue(szData, pData - szData);
		if (lRet == PK_SUCCESS)
		{
			g_logger.LogMessage(PK_LOGLEVEL_DEBUG, _("Driver[%s] send data to NodeServer success, type:%d, actionType:%d, len:%d"), g_strDrvName.c_str(), nActionType, sizeof(int) + nContentLen);
		}
		else
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, _("Driver[%s] send data to NodeServer failed, type:%d, actionType:%d, len:%d"), g_strDrvName.c_str(), nActionType, sizeof(int) + nContentLen);

		return 0;
	}
	else
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "PublishMsg2NodeServer.Get Que(data2NodeServer) failed");

	return -100;
}
