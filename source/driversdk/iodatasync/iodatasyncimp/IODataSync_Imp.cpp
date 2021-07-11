/**************************************************************
 *  Filename:    IODataSync_Imp.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 提供给驱动调用的发送数据接口.
 *
 *  @author:     shijunpu
 *  @version     05/30/2008  shijunpu  Initial Version
**************************************************************/

#include "IODataProcessor.h"
#include "pknetque/pknetque.h"
//#include "driversdk/IODataSync.h"
#include "errcode/ErrCode_iCV_DA.hxx"
#include "pksock/pksock_errcode.h"

HQUEUE	g_hLocalQueue;				// 驱动用于将数据发送到驱动数据同步模块的本地QUEUE
HQUEUE	g_hTransferQueue;			// 指定驱动数据同步模块用于接收本地驱动数据的QUEUE
CIODataProcessor g_Processor;		// 同步数据处理，负责接收同步数据并将数据写入到DIT
bool	g_bSyncInit = false;

#define PK_IODATASYNC_DEFAULT_TCPPORT	50006
/**
 *  驱动数据同步DLL初始化.
 *
 *
 *  @version     07/04/2008  shijunpu  Initial Version.
 */
EXPORT32 long IOSync_Init(const char* szDriverName, PFN_IOSyncCallback pfnIOSyncCallback, void* pCallbackParam)
{
	long nStatus = PK_SUCCESS;
	return nStatus;

	if (g_bSyncInit)
	{
		return EC_ICV_DA_IOSYNC_INIT_ALREADY; //已经初始化过
	}

	if(szDriverName == NULL)
		return EC_ICV_DA_INVALID_PARAMETER;

	// 初始化网络服务，异步发送
	nStatus = PKQUE_Init(PKQUE_ASYNC_SEND, NULL);
	if (nStatus != PK_SUCCESS)
	{
//		LH_CRITICAL("初始化CVNDK失败!");
		return EC_PKSOCK_INITFAILED;
	}

	// 注册本地QUEUE
	g_hLocalQueue = PKQUE_RegLocalQueue(QUEUEID_LOCALDRV_FROM);
	if (g_hLocalQueue == NULL)
	{
		//		LH_CRITICAL("注册本地QUEUE失败!");
		return EC_PKSOCK_QUEUEERROR;
	}
	unsigned short nPort = PK_IODATASYNC_DEFAULT_TCPPORT;//PKComm::GetServicePort(SERVICE_NAME, ICV_IODATASYNC_DEFAULT_TCPPORT);

	// 注册远程QUEUE
	g_hTransferQueue = PKQUE_RegRemoteQueue(QUEUEID_LOCALDRV_TO, LOCAL_IP, nPort);
	
	if (g_hTransferQueue == NULL)
	{
		//		LH_CRITICAL("注册远程QUEUE失败!");
		return EC_PKSOCK_QUEUEERROR;
	}

	g_Processor.SetDriverName(szDriverName);

	// 设置回调函数
	g_Processor.SetCallback(pfnIOSyncCallback, pCallbackParam);

	// 启动数据同步线程，用于接收对等节点发送的数据
	g_Processor.Start();

	g_bSyncInit = true;

	return PK_SUCCESS;
}

/**
 *  驱动数据同步DLL释放资源.
 *
 *
 *  @version     07/04/2008  shijunpu  Initial Version.
 */
EXPORT32 long IOSync_Finalize()
{
	return PK_SUCCESS;

	// 停止数据同步线程
	g_Processor.Stop();

	if (g_hLocalQueue != NULL)
	{
		// 释放本地QUEUE
		PKQUE_FreeQueue(g_hLocalQueue);
	}
	
	if (g_hTransferQueue != NULL)
	{
		// 释放远程QUEUE
		PKQUE_FreeQueue(g_hTransferQueue);
	}

	//  终止网络服务
	long nStatus = PKQUE_Finalize();

	if (nStatus != PK_SUCCESS)
	{
		return nStatus;
	}
	
	g_bSyncInit = false;

	return PK_SUCCESS;
}

/**
 *  发送同步数据到驱动数据同步模块.
 *  提供给驱动的接口，驱动调用该接口将同步数据发送到驱动数据同步模块.
 *
 *  @param  -[in]  IOSYNCDATA*  pSyncData: [comment]
 *
 *  @version     05/31/2008  shijunpu  Initial Version.
 */
EXPORT32 long IOSync_SendtoPeer(IOSYNCDATA *pSyncData)
{
	return PK_SUCCESS;
	
	if (!g_bSyncInit)
	{
		return EC_ICV_DA_IOSYNC_NOT_INIT;
	}

	// 待发送数据：驱动名称  +设备名称 + 数据块名称 + 数据质量 + 时间戳 + 实时数据
	long nSendBytes = PK_NAME_MAXLEN + PK_NAME_MAXLEN 
			+ PK_NAME_MAXLEN + pSyncData->nLength + sizeof(pSyncData->nQuality)
			+ sizeof(pSyncData->cvTimeStamp) + sizeof(char);

	char* pSendBuf = new char[nSendBytes];

	char* ptr = pSendBuf;
	
	// 消息类型
	memcpy(ptr, &pSyncData->chMessageType, sizeof(char));
	ptr += sizeof(char);

	// 驱动名称
	memcpy(ptr, pSyncData->szDriverName, sizeof(pSyncData->szDriverName));
	ptr += sizeof(pSyncData->szDriverName);
	
	// 设备名称
	memcpy(ptr, pSyncData->szDeviceName, sizeof(pSyncData->szDeviceName));
	ptr += sizeof(pSyncData->szDeviceName);

	// 数据块名称
	memcpy(ptr, pSyncData->szDataBlockName, sizeof(pSyncData->szDataBlockName));
	ptr += sizeof(pSyncData->szDataBlockName);

	// 数据质量
	memcpy(ptr, &pSyncData->nQuality, sizeof(pSyncData->nQuality));
	ptr += sizeof(pSyncData->nQuality);

	// 时间戳
	memcpy(ptr, &pSyncData->cvTimeStamp, sizeof(HighResTime));
	ptr += sizeof(HighResTime);

	// 存在实时数据
	if (pSyncData->nLength != 0  && pSyncData->pszData != NULL)
	{
		memcpy(ptr, pSyncData->pszData, pSyncData->nLength);
	}
	else if (pSyncData->nLength != 0 && pSyncData->pszData == NULL)
	{
		pSyncData->nLength = 0;
	}

	// 发送数据到对等冗余节点的驱动数据同步模块
	long nStatus = PKQUE_Send(g_hTransferQueue, g_hLocalQueue, pSendBuf, nSendBytes);

	delete[] pSendBuf;

	return nStatus;
}
