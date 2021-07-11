/**************************************************************
 *  Filename:    IODataSync_Imp.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: �ṩ���������õķ������ݽӿ�.
 *
 *  @author:     shijunpu
 *  @version     05/30/2008  shijunpu  Initial Version
**************************************************************/

#include "IODataProcessor.h"
#include "pknetque/pknetque.h"
//#include "driversdk/IODataSync.h"
#include "errcode/ErrCode_iCV_DA.hxx"
#include "pksock/pksock_errcode.h"

HQUEUE	g_hLocalQueue;				// �������ڽ����ݷ��͵���������ͬ��ģ��ı���QUEUE
HQUEUE	g_hTransferQueue;			// ָ����������ͬ��ģ�����ڽ��ձ����������ݵ�QUEUE
CIODataProcessor g_Processor;		// ͬ�����ݴ����������ͬ�����ݲ�������д�뵽DIT
bool	g_bSyncInit = false;

#define PK_IODATASYNC_DEFAULT_TCPPORT	50006
/**
 *  ��������ͬ��DLL��ʼ��.
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
		return EC_ICV_DA_IOSYNC_INIT_ALREADY; //�Ѿ���ʼ����
	}

	if(szDriverName == NULL)
		return EC_ICV_DA_INVALID_PARAMETER;

	// ��ʼ����������첽����
	nStatus = PKQUE_Init(PKQUE_ASYNC_SEND, NULL);
	if (nStatus != PK_SUCCESS)
	{
//		LH_CRITICAL("��ʼ��CVNDKʧ��!");
		return EC_PKSOCK_INITFAILED;
	}

	// ע�᱾��QUEUE
	g_hLocalQueue = PKQUE_RegLocalQueue(QUEUEID_LOCALDRV_FROM);
	if (g_hLocalQueue == NULL)
	{
		//		LH_CRITICAL("ע�᱾��QUEUEʧ��!");
		return EC_PKSOCK_QUEUEERROR;
	}
	unsigned short nPort = PK_IODATASYNC_DEFAULT_TCPPORT;//PKComm::GetServicePort(SERVICE_NAME, ICV_IODATASYNC_DEFAULT_TCPPORT);

	// ע��Զ��QUEUE
	g_hTransferQueue = PKQUE_RegRemoteQueue(QUEUEID_LOCALDRV_TO, LOCAL_IP, nPort);
	
	if (g_hTransferQueue == NULL)
	{
		//		LH_CRITICAL("ע��Զ��QUEUEʧ��!");
		return EC_PKSOCK_QUEUEERROR;
	}

	g_Processor.SetDriverName(szDriverName);

	// ���ûص�����
	g_Processor.SetCallback(pfnIOSyncCallback, pCallbackParam);

	// ��������ͬ���̣߳����ڽ��նԵȽڵ㷢�͵�����
	g_Processor.Start();

	g_bSyncInit = true;

	return PK_SUCCESS;
}

/**
 *  ��������ͬ��DLL�ͷ���Դ.
 *
 *
 *  @version     07/04/2008  shijunpu  Initial Version.
 */
EXPORT32 long IOSync_Finalize()
{
	return PK_SUCCESS;

	// ֹͣ����ͬ���߳�
	g_Processor.Stop();

	if (g_hLocalQueue != NULL)
	{
		// �ͷű���QUEUE
		PKQUE_FreeQueue(g_hLocalQueue);
	}
	
	if (g_hTransferQueue != NULL)
	{
		// �ͷ�Զ��QUEUE
		PKQUE_FreeQueue(g_hTransferQueue);
	}

	//  ��ֹ�������
	long nStatus = PKQUE_Finalize();

	if (nStatus != PK_SUCCESS)
	{
		return nStatus;
	}
	
	g_bSyncInit = false;

	return PK_SUCCESS;
}

/**
 *  ����ͬ�����ݵ���������ͬ��ģ��.
 *  �ṩ�������Ľӿڣ��������øýӿڽ�ͬ�����ݷ��͵���������ͬ��ģ��.
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

	// ���������ݣ���������  +�豸���� + ���ݿ����� + �������� + ʱ��� + ʵʱ����
	long nSendBytes = PK_NAME_MAXLEN + PK_NAME_MAXLEN 
			+ PK_NAME_MAXLEN + pSyncData->nLength + sizeof(pSyncData->nQuality)
			+ sizeof(pSyncData->cvTimeStamp) + sizeof(char);

	char* pSendBuf = new char[nSendBytes];

	char* ptr = pSendBuf;
	
	// ��Ϣ����
	memcpy(ptr, &pSyncData->chMessageType, sizeof(char));
	ptr += sizeof(char);

	// ��������
	memcpy(ptr, pSyncData->szDriverName, sizeof(pSyncData->szDriverName));
	ptr += sizeof(pSyncData->szDriverName);
	
	// �豸����
	memcpy(ptr, pSyncData->szDeviceName, sizeof(pSyncData->szDeviceName));
	ptr += sizeof(pSyncData->szDeviceName);

	// ���ݿ�����
	memcpy(ptr, pSyncData->szDataBlockName, sizeof(pSyncData->szDataBlockName));
	ptr += sizeof(pSyncData->szDataBlockName);

	// ��������
	memcpy(ptr, &pSyncData->nQuality, sizeof(pSyncData->nQuality));
	ptr += sizeof(pSyncData->nQuality);

	// ʱ���
	memcpy(ptr, &pSyncData->cvTimeStamp, sizeof(HighResTime));
	ptr += sizeof(HighResTime);

	// ����ʵʱ����
	if (pSyncData->nLength != 0  && pSyncData->pszData != NULL)
	{
		memcpy(ptr, pSyncData->pszData, pSyncData->nLength);
	}
	else if (pSyncData->nLength != 0 && pSyncData->pszData == NULL)
	{
		pSyncData->nLength = 0;
	}

	// �������ݵ��Ե�����ڵ����������ͬ��ģ��
	long nStatus = PKQUE_Send(g_hTransferQueue, g_hLocalQueue, pSendBuf, nSendBytes);

	delete[] pSendBuf;

	return nStatus;
}
