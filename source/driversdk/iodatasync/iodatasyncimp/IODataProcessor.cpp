/**************************************************************
 *  Filename:    IODataProcessor.cpp
 *  Copyright:   Shanghai  Software Co., Ltd.
 *
 *  Description: 同步数据处理类，负责接收同步数据并将数据发送到.
 *
 *  @author:     shijunpu
 *  @version     05/30/2008  shijunpu  Initial Version
**************************************************************/

#include "IODataProcessor.h"
#include "driversdk/IODataSync.h"
#include "errcode/ErrCode_iCV_DA.hxx"
#include "Quality.h"
#include "pkdriver/pkdrvcmn.h"

#define LH_DEBUG(X) g_logger.LogMessage((PK_LOGLEVEL_DEBUG,X))
#define LH_ERROR(X) g_logger.LogMessage((PK_LOGLEVEL_ERROR,X))
#define LH_INFO(X)  g_logger.LogMessage((PK_LOGLEVEL_INFO,X))
#define LH_WARN(X)  g_logger.LogMessage((PK_LOGLEVEL_WARN,X))
#define LH_CRITICAL(X) g_logger.LogMessage((PK_LOGLEVEL_CRITICAL,X))

extern CPKLog g_logger;
#define ICV_IODATASYNC_DEFAULT_TCPPORT	50006

#define DEFAULT_BUF_SIZE 256
#define CHECK_NULL_PARAMETER_RETURN_ERR(X) {if(!X) return EC_ICV_DA_INVALID_PARAMETER;}

/**
 *  构造函数.
 *
 *
 *  @version     05/31/2008  shijunpu  Initial Version.
 */
CIODataProcessor::CIODataProcessor()
{
	m_LocalQueue = 0;
	m_hRemoteQueue = 0;
	m_bStop = false;
	m_pCallbackParam = NULL;
	m_pfnIOSyncCallback = NULL;
	memset(m_szDriverName, 0, PK_NAME_MAXLEN);
}

/**
 *  析构函数.
 *
 *
 *  @version     05/31/2008  shijunpu  Initial Version.
 */
CIODataProcessor::~CIODataProcessor()
{

}

/**
 *  继承自ACE_Task.
 *
 *
 *  @version     05/31/2008  shijunpu  Initial Version.
 */
int CIODataProcessor::svc()
{
	ProcessData();

	if (m_LocalQueue != 0)
	{
		PKQUE_FreeQueue(m_LocalQueue);
	}
	
	if (m_hRemoteQueue != NULL)
	{
		// 释放远程QUEUE
		PKQUE_FreeQueue(m_hRemoteQueue);
	}
	return PK_SUCCESS;
}

/**
 *  启动同步数据处理线程.
 *
 *
 *  @version     05/31/2008  shijunpu  Initial Version.
 */
void CIODataProcessor::Start()
{
	this->activate();
}


/**
 *  停止同步数据处理线程.
 *
 *
 *  @version     05/31/2008  shijunpu  Initial Version.
 */
void CIODataProcessor::Stop()
{
	m_bStop = true;
	this->wait();
}

/**
 *  设置回调函数及参数.
 *
 *  @param  -[in]  PFN_IOSyncCallback  pfnIOSyncCallback: [comment]
 *  @param  -[in]  void*  pCallbackParam: [comment]
 *
 *  @version     09/02/2008  shijunpu  Initial Version.
 */
long CIODataProcessor::SetCallback(PFN_IOSyncCallback pfnIOSyncCallback, void* pCallbackParam)
{
	m_pfnIOSyncCallback = pfnIOSyncCallback;
	m_pCallbackParam = pCallbackParam;
	return PK_SUCCESS;
}

/**
 *  同步数据.
 *
 *
 *  @version     05/31/2008  shijunpu  Initial Version.
 */
void CIODataProcessor::ProcessData()
{
	long nStatus = PK_SUCCESS;

	m_LocalQueue = PKQUE_RegLocalQueue(PROCESSOR_QUEUEID);

	// 得到ICV所在节点的对等冗余节点的IP地址
	const char* szIPAddr = "0.0.0.0";//PKComm::GetPeerIPAddr();
	if (szIPAddr == NULL)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR,"获取对等冗余节点的IP地址失败!");
		return;
	}
	else
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "获取对等冗余节点的IP地址成功(IP:%s)!", szIPAddr);
	}

	unsigned short nPort = ICV_IODATASYNC_DEFAULT_TCPPORT;//PKComm::GetServicePort(SERVICE_NAME, ICV_IODATASYNC_DEFAULT_TCPPORT);

	m_hRemoteQueue = PKQUE_RegRemoteQueue(QUEUEID_LOCALDRV_TO, szIPAddr, nPort);
	if (m_hRemoteQueue == NULL)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "注册远程QUEUE失败!");
		return;
	}

	while (!m_bStop)
	{
		if(NULL == m_LocalQueue)
		{
			m_LocalQueue = PKQUE_RegLocalQueue(PROCESSOR_QUEUEID);
		}
		// 与对等冗余节点的数据同步模块的连接是否存在
		if (PKQUE_GetConnectStat(m_hRemoteQueue) != 1)
		{
			// 不存在，则重新连接对等冗余节点的数据同步模块
			char szBuf[DEFAULT_BUF_SIZE];
			memset(szBuf, MESSAGE_CONNECT, sizeof(char));
			memcpy(szBuf + 1, m_szDriverName, PK_NAME_MAXLEN);
			PKQUE_Send(m_hRemoteQueue, m_LocalQueue, szBuf, sizeof(char) + PK_NAME_MAXLEN);
		}

		HQUEUE hSrcQueue = 0;
		char* pRecvBuf = 0;
		int nRecvBytes = 0;
		nStatus = PKQUE_Recv(m_LocalQueue, &hSrcQueue, &pRecvBuf, &nRecvBytes, DEFAULT_RECV_TIMEOUT);
		if (nStatus != PK_SUCCESS)
		{
		//	LH_DEBUG("未接收到同步数据!");
			PKQUE_FreeQueue(hSrcQueue);
			PKQUE_FreeMem(pRecvBuf);
			continue;
		}
	
		// 接收到同步数据
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "非活动节点: 接收同步数据成功!");
		if (m_pfnIOSyncCallback != NULL)
		{
			// 解析出同步数据
			IOSYNCDATA syncData;
			GainSyncData(pRecvBuf, nRecvBytes, &syncData);
			
			// 执行回调函数，处理同步数据
			if (m_pfnIOSyncCallback != NULL)
			{
				m_pfnIOSyncCallback(&syncData, m_pCallbackParam);
			}
		}

		PKQUE_FreeQueue(hSrcQueue);
		PKQUE_FreeMem(pRecvBuf);
	}
}

/**
 *  解析同步数据.
 *
 *  @param  -[in]  void*  pBuf: [comment]
 *  @param  -[in]  long  nRecvBytes: [comment]
 *  @param  -[out]  IOSYNCDATA*  pSyncData: [comment]
 *
 *  @version     07/28/2008  shijunpu  Initial Version.
 */
long CIODataProcessor::GainSyncData(void* pBuf, long nRecvBytes, IOSYNCDATA *pSyncData)
{
	CHECK_NULL_PARAMETER_RETURN_ERR(pBuf);

	// 第一个字节为消息类型
	char* pTemp = (char*) pBuf + 1;

	//  驱动名称
	memcpy(pSyncData->szDriverName, pTemp, PK_NAME_MAXLEN);
	pTemp += PK_NAME_MAXLEN;
	
	// 设备名称
	memcpy(pSyncData->szDeviceName, pTemp, PK_NAME_MAXLEN);
	pTemp += PK_NAME_MAXLEN;
	
	// 数据块名称
	memcpy(pSyncData->szDataBlockName, pTemp, PK_NAME_MAXLEN);
	pTemp += PK_NAME_MAXLEN;
	
	// 数据质量
	memcpy(&pSyncData->nQuality, pTemp, sizeof(pSyncData->nQuality));
	pTemp += sizeof(pSyncData->nQuality);
	
	// 时间戳
	memcpy(&pSyncData->cvTimeStamp, pTemp, sizeof(HighResTime));
	pTemp += sizeof(HighResTime);

	pSyncData->nLength = nRecvBytes - PK_NAME_MAXLEN - PK_NAME_MAXLEN
			- PK_NAME_MAXLEN - sizeof(pSyncData->nQuality) 
			- sizeof(HighResTime) - sizeof(char);
	
	if (pSyncData->nLength >= 0)
	{
		pSyncData->pszData = new char[pSyncData->nLength];
		memcpy(pSyncData->pszData, pTemp, pSyncData->nLength);
	}

	return PK_SUCCESS;
}


/**
 *  设置驱动名称.
 *
 *  @param  -[in]  const char*  szDriverName: [comment]
 *
 *  @version     09/03/2008  shijunpu  Initial Version.
 */
void CIODataProcessor::SetDriverName(const char* szDriverName)
{
	if (szDriverName == NULL)
		return;

	int nCopyLen = PK_NAME_MAXLEN - 1;
	if ((int)strlen(szDriverName) < nCopyLen)
	{
		nCopyLen = strlen(szDriverName);
	}

	memcpy(m_szDriverName, szDriverName, nCopyLen);
	m_szDriverName[nCopyLen] = '\0';
}
