/**************************************************************
 *  Filename:    ServiceMainTask.cpp
 *  Copyright:   Shanghai Peakinfo Co., Ltd.
 *
 *  Description: 服务主进程源文件.
 *
 *  @author:     wanghongwei
 *  @version     11/03/2016  @Reviser  Initial Version
 **************************************************************/
  
#include "SockServerTask.h"
#include "pklog/pklog.h"
extern CPKLog PKLog;

using namespace std;

void ConnCallBack_Server(HQUEUE hDestQue, int nConnectState)
{
	int nLocalQueID = -1, nRemoteQueID = -1;
	char szRemoteIP[32];
	unsigned short nRemotePort;
	SOCKSVR_GetQueueAddr(hDestQue, szRemoteIP, sizeof(szRemoteIP), &nRemotePort);

	switch(nConnectState)
	{
	case 1:
		printf("SOCKSVR_CONNECTED(RemoteQue: %d, RemoteIP:%s:%d).\n", nRemoteQueID, szRemoteIP, nRemotePort);
		break;

	case 0:
		printf("SOCKSVR_DISCONNECTED (LocalQue: %d, RemoteIP:%s:%d).\n", nLocalQueID, szRemoteIP, nRemotePort);
		break;

	default:
		printf("nConnectState unknown: %d.\n", nConnectState);		
	}
}


// 从头部得到长度。同时校验合法性
extern int GetMsgLength(char *pHeader, int nLen);

#define DEFAULT_BUF_LEN		100
int CSockServerTask::RecvFromCliAndResponse()
{
	HQUEUE hSrcQueue = 0;
	char* szRecvBuf = 0;
	int nRecvBytes = 0;
	int nRet = 0;
	nRet = SOCKSVR_Recv(&hSrcQueue, &szRecvBuf, &nRecvBytes);
	if (nRet != 0)
	{
		printf("Receive failed! \n");
		return -1;
	}

	printf("Server Recv, data(%d Bytes): %s.\n", nRecvBytes, szRecvBuf + 5);

	if (SOCKSVR_GetConnectStat(hSrcQueue) == 0)
	{
		printf("Server Recv, but not connected.\n");
		return -2;
	}

	nRet = SOCKSVR_Send(hSrcQueue, (const char *)szRecvBuf, nRecvBytes);
	if(nRet != 0)
	{
		printf("Send %d failed! \n", nRecvBytes);
	}
	else
		printf("Send %d success! \n", nRecvBytes);

	SOCKSVR_FreeMem(szRecvBuf);
	SOCKSVR_FreeQueue(hSrcQueue);

	return nRet;
}

//ctor
CSockServerTask::CSockServerTask()
{
	m_bStop = false;
}
//dctor
CSockServerTask::~CSockServerTask()
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
int CSockServerTask::svc()
{
	int nRet = 0;
	this->OnStart();

	while(!m_bStop)
	{
		RecvFromCliAndResponse();
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
int CSockServerTask::StartTask()
{	
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
int CSockServerTask::OnStart()
{
	long lRet = SOCKSVR_Init(ConnCallBack_Server, SOCK_SEND_SYNC);
	if (0 != lRet)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "SOCKCLI_Init failed");
		return lRet;
	}
	SOCKSVR_SetPackHeaderParam(5, GetMsgLength);

	if (SOCKSVR_Listen(13000) != 0)
	{
		printf("Listen failed: %d! \n", 13000);
	}
	else
	{
		printf("Listen port %d success!\n", 13000);
	}

	return 0;
}

/**
 *  服务停止函数.
 *
 *  @return		ICV_SUCCESS. 
 *
 *  @version	10/10/2016	wanghongwei   Initial Version 
 */
int CSockServerTask::StopTask()
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
int CSockServerTask::OnStop()
{
	SOCKSVR_Finalize();

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
int CSockServerTask::ReadConfig()
{
	return 0;
}