/**************************************************************
 *  Filename:    ServiceMainTask.cpp
 *  Copyright:   Shanghai Peakinfo Co., Ltd.
 *
 *  Description: 服务主进程源文件.
 *
 *  @author:     wanghongwei
 *  @version     11/03/2016  @Reviser  Initial Version
 **************************************************************/
  
#include "SockClientTask.h"
#include "pklog/pklog.h"
extern CPKLog PKLog;

using namespace std;

void ConnCallBack_Client(HQUEUE hDestQue, int nConnectState)
{
	int nLocalQueID = -1, nRemoteQueID = -1;
	char szRemoteIP[32];
	unsigned short nRemotePort;
	SOCKCLI_GetQueueAddr(hDestQue, szRemoteIP, sizeof(szRemoteIP), &nRemotePort);

	switch(nConnectState)
	{
	case 1:
		printf("SOCKCLI_CONNECTED(RemoteQue: %d, RemoteIP:%s:%d).\n", nRemoteQueID, szRemoteIP, nRemotePort);
		break;

	case 0:
		printf("SOCKCLI_DISCONNECTED (LocalQue: %d, RemoteIP:%s:%d).\n", nLocalQueID, szRemoteIP, nRemotePort);
		break;

	default:
		printf("nConnectState unknown: %d.\n", nConnectState);		
	}
}


// 从头部得到长度。同时校验合法性
int GetMsgLength(char *pHeader, int nLen)
{
	if(nLen != 5)
		return -1;
	if(*pHeader != 0x02)
		return -2;
	pHeader ++;

	int nMsgLength = 0;
	memcpy(&nMsgLength, pHeader, 4);
	if(nMsgLength > 1024*1024*100)
		return -3;
	return nMsgLength;
}

#define DEFAULT_BUF_LEN		100
int CSockClientTask::SendAndRecvFromSvr()
{
	char buf[DEFAULT_BUF_LEN];
	memset(buf, 0, DEFAULT_BUF_LEN);
	char *pBuf = buf;
	*pBuf++ = 0x02;

	int nLen = 15;
	memcpy(pBuf, &nLen, 4);
	pBuf += 4;

	strcpy(pBuf, "test_data");
	pBuf += 10;

	int ret = 0;
	//ret = send(hRemoteQueue, buf);
	ret = SOCKCLI_Send(m_hRemoteQueue, buf, pBuf - buf, 1000, NULL, NULL, SOCK_SEND_SYNC);
	if ( ret == 0)
	{
		printf("SOCKCLI_CONNECTED\n");
		if (ret != 0)
		{
			printf("Send failed! \n");
			return -1;
		}

		char* szRecvBuf = 0;
		int lRecvBytes = 0;

		HQUEUE hSrcQueue;
		ret = SOCKCLI_Recv(&hSrcQueue, &szRecvBuf, &lRecvBytes, 100000);
		if (ret != 0)
		{
			SOCKCLI_FreeMem(szRecvBuf);
			SOCKCLI_FreeQueue(hSrcQueue);
			printf("Receive failed! \n");
		}
		else
		{	
			printf("Client, Recv(%d Bytes): %s.\n",
				lRecvBytes, szRecvBuf);	
			SOCKCLI_FreeMem(szRecvBuf);
			SOCKCLI_FreeQueue(hSrcQueue);
		}
	}
	else
	{
		printf("SOCKCLI_DISCONNECTED\n");
	}
	return 0;
}

//ctor
CSockClientTask::CSockClientTask()
{
	m_bStop = false;
}
//dctor
CSockClientTask::~CSockClientTask()
{
}


/**
 *  任务的主流程.
 *		1.监控该服务器的活动情况
 *
 *  @return		ICV_SUCCESS.
 *
 *  @version	10/10/2016	wanghongwei   Initial Version
 */
int CSockClientTask::svc()
{
	int nRet = 0;
	this->OnStart();

	while(!m_bStop)
	{
		SendAndRecvFromSvr();
		ACE_OS::sleep(1);
	}

	this->OnStop();
	return 0;	
}

/**
 *  服务启动函数.
 *
 *  @return		ICV_SUCCESS. 
 *
 *  @version	10/10/2016	wanghongwei   Initial Version 
 */
int CSockClientTask::StartTask()
{	
	return 0;
	m_bStop = false;
	int nRet = 0;
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "The ExecuteTask is starting...");
	nRet = (long)this->activate(THR_NEW_LWP | THR_JOINABLE |THR_INHERIT_SCHED);
	if(nRet != 0)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "Failed to start ServiceMainTask, errorcode(%d).", nRet);
		return -1;
	}

	return 0;
}

/**
 *  内部启动函数.
 *
 *  @return		ICV_SUCCESS. 
 *
 *  @version	10/10/2016	wanghongwei   Initial Version 
 */
int CSockClientTask::OnStart()
{
	long lRet = SOCKCLI_Init(ConnCallBack_Client, SOCK_SEND_SYNC);
	if (0 != lRet)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "SOCKCLI_Init failed");
		return lRet;
	}
	SOCKCLI_SetPackHeaderParam(5, GetMsgLength);
	m_hRemoteQueue = SOCKCLI_RegRemoteQueue("127.0.0.1", 12000);

	return 0;
}

/**
 *  服务停止函数.
 *
 *  @return		ICV_SUCCESS. 
 *
 *  @version	10/10/2016	wanghongwei   Initial Version 
 */
int CSockClientTask::StopTask()
{ 
	m_bStop = true;
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "The ServiceMainTask is stopping...");

	int nWaitResult = wait();
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "The ServiceMainTask has stopped(%d).", nWaitResult);
	return nWaitResult;
}

/**
 *  内部停止函数.
 *
 *  @return		ICV_SUCCESS. 
 *
 *  @version	10/10/2016	wanghongwei   Initial Version 
 */
int CSockClientTask::OnStop()
{ 
	SOCKCLI_FreeQueue(m_hRemoteQueue);
	m_hRemoteQueue = NULL;
	SOCKCLI_Finalize();

	return 0;
}
 

/**
 *  读取配置文件.
 *
 *  @param		-[in]  const char* szFilePath: [配置文件]
 *
 *  @return		ICV_SUCCESS. 
 *
 *  @version	10/10/2016	wanghongwei   Initial Version 
 */
int CSockClientTask::ReadConfig()
{
	return 0;
}