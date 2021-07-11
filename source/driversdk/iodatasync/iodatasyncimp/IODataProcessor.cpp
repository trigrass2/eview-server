/**************************************************************
 *  Filename:    IODataProcessor.cpp
 *  Copyright:   Shanghai  Software Co., Ltd.
 *
 *  Description: ͬ�����ݴ����࣬�������ͬ�����ݲ������ݷ��͵�.
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
 *  ���캯��.
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
 *  ��������.
 *
 *
 *  @version     05/31/2008  shijunpu  Initial Version.
 */
CIODataProcessor::~CIODataProcessor()
{

}

/**
 *  �̳���ACE_Task.
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
		// �ͷ�Զ��QUEUE
		PKQUE_FreeQueue(m_hRemoteQueue);
	}
	return PK_SUCCESS;
}

/**
 *  ����ͬ�����ݴ����߳�.
 *
 *
 *  @version     05/31/2008  shijunpu  Initial Version.
 */
void CIODataProcessor::Start()
{
	this->activate();
}


/**
 *  ֹͣͬ�����ݴ����߳�.
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
 *  ���ûص�����������.
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
 *  ͬ������.
 *
 *
 *  @version     05/31/2008  shijunpu  Initial Version.
 */
void CIODataProcessor::ProcessData()
{
	long nStatus = PK_SUCCESS;

	m_LocalQueue = PKQUE_RegLocalQueue(PROCESSOR_QUEUEID);

	// �õ�ICV���ڽڵ�ĶԵ�����ڵ��IP��ַ
	const char* szIPAddr = "0.0.0.0";//PKComm::GetPeerIPAddr();
	if (szIPAddr == NULL)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR,"��ȡ�Ե�����ڵ��IP��ַʧ��!");
		return;
	}
	else
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "��ȡ�Ե�����ڵ��IP��ַ�ɹ�(IP:%s)!", szIPAddr);
	}

	unsigned short nPort = ICV_IODATASYNC_DEFAULT_TCPPORT;//PKComm::GetServicePort(SERVICE_NAME, ICV_IODATASYNC_DEFAULT_TCPPORT);

	m_hRemoteQueue = PKQUE_RegRemoteQueue(QUEUEID_LOCALDRV_TO, szIPAddr, nPort);
	if (m_hRemoteQueue == NULL)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "ע��Զ��QUEUEʧ��!");
		return;
	}

	while (!m_bStop)
	{
		if(NULL == m_LocalQueue)
		{
			m_LocalQueue = PKQUE_RegLocalQueue(PROCESSOR_QUEUEID);
		}
		// ��Ե�����ڵ������ͬ��ģ��������Ƿ����
		if (PKQUE_GetConnectStat(m_hRemoteQueue) != 1)
		{
			// �����ڣ����������ӶԵ�����ڵ������ͬ��ģ��
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
		//	LH_DEBUG("δ���յ�ͬ������!");
			PKQUE_FreeQueue(hSrcQueue);
			PKQUE_FreeMem(pRecvBuf);
			continue;
		}
	
		// ���յ�ͬ������
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "�ǻ�ڵ�: ����ͬ�����ݳɹ�!");
		if (m_pfnIOSyncCallback != NULL)
		{
			// ������ͬ������
			IOSYNCDATA syncData;
			GainSyncData(pRecvBuf, nRecvBytes, &syncData);
			
			// ִ�лص�����������ͬ������
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
 *  ����ͬ������.
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

	// ��һ���ֽ�Ϊ��Ϣ����
	char* pTemp = (char*) pBuf + 1;

	//  ��������
	memcpy(pSyncData->szDriverName, pTemp, PK_NAME_MAXLEN);
	pTemp += PK_NAME_MAXLEN;
	
	// �豸����
	memcpy(pSyncData->szDeviceName, pTemp, PK_NAME_MAXLEN);
	pTemp += PK_NAME_MAXLEN;
	
	// ���ݿ�����
	memcpy(pSyncData->szDataBlockName, pTemp, PK_NAME_MAXLEN);
	pTemp += PK_NAME_MAXLEN;
	
	// ��������
	memcpy(&pSyncData->nQuality, pTemp, sizeof(pSyncData->nQuality));
	pTemp += sizeof(pSyncData->nQuality);
	
	// ʱ���
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
 *  ������������.
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
