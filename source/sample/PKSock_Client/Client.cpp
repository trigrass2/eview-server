#include "pksock/pksockclient.h"
#include <stdio.h>
#include <string.h>
#include "ace/OS_NS_unistd.h"
#include "iostream"
using namespace std;


#ifndef PK_SUCCESS
#define PK_SUCCESS							0								// iCV系统中所有成功消息的返回码
#endif

#define DEFAULT_REMOTE_PORT					12000

#define	DEFAULT_BUF_LEN						100

typedef int (* PFN_Send)(HQUEUE hRemoteQueue, char* const &buf);
typedef int (* PFN_Recv)(HQUEUE *phSrcQueue, char* &szRecvBuf, int* pnRecvBytes);


//HQUEUE hRemoteQueue = SOCKCLI_RegRemoteQueue(DEFAULT_REMOTE_QUEUEID, "10.25.77.179", DEFAULT_PORT1);
HQUEUE hRemoteQueue = NULL;

void SendCallback(void *pParam, int lTotalPackage, int lCurrPackage)
{
	printf("Sent(%d/%d)\n", lCurrPackage, lTotalPackage);
}

void ConnCallBack(HQUEUE hDestQue, int nConnectState)
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

int testSendAndRecv()
{	
	char buf[DEFAULT_BUF_LEN];
	memset(buf, 0, DEFAULT_BUF_LEN);
	char *pBuf = buf;
	*pBuf++ = 0x02;

	int nLen = 15;
	char *pBufLen = pBuf;
	//memcpy(pBuf, &nLen, 4);
	pBuf += 4;

	strcpy(pBuf, "test_data.No.");
	pBuf += strlen("test_data.No.");
	
	HQUEUE hSrcQueue = 0;
	
	int i = 0;
	
	while (1)
	{
		i++;
		if (i % 10 == 0)
		{
			//printf("No.%d times\n", i);
		}
		char szTimes[32] = { 0 };
		sprintf(szTimes, "%d", i);
		//strcat(pBuf, szTimes);
		memcpy(pBuf, szTimes, strlen(szTimes));
		int nSendLen = pBuf - buf + strlen(szTimes) + 1;
		pBuf[nSendLen] = '\0';
		memcpy(pBufLen, &nSendLen, 4);
		int ret = PK_SUCCESS;
		ret = SOCKCLI_Send(hRemoteQueue, buf, nSendLen, 1000, NULL, NULL, SOCK_SEND_SYNC);
		if ( ret == PK_SUCCESS)
		{
			printf("SOCKCLI_CONNECTED, Send:%s, %d bytes, %d times\n", buf + 5, pBuf - buf,i);
			
			if (ret != PK_SUCCESS)
			{
				printf("Send failed! \n");
				continue;
			}
			
			char* szRecvBuf = 0;
			int lRecvBytes = 0;
			
			ret = SOCKCLI_Recv(&hSrcQueue, &szRecvBuf, &lRecvBytes, 100000);
			if (ret != PK_SUCCESS)
			{
				SOCKCLI_FreeMem(szRecvBuf);
				SOCKCLI_FreeQueue(hSrcQueue);
				printf("Receive failed! \n");
			}
			else
			{	
				//szRecvBuf[lRecvBytes] = '\0';
				printf("---Client, Recv(%d Bytes): %s.\n",lRecvBytes, szRecvBuf + 5);	
				SOCKCLI_FreeMem(szRecvBuf);
				SOCKCLI_FreeQueue(hSrcQueue);
			}
		}
		else
		{
			printf("SOCKCLI_DISCONNECTED\n");
		}
		
		ACE_OS::sleep(1);
	}
	
	SOCKCLI_FreeQueue(hRemoteQueue);
	
	return 0;
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

int main(int argc, char**argv)
{
	long lRet = SOCKCLI_Init(ConnCallBack, SOCK_SEND_SYNC);
	if (PK_SUCCESS != lRet)
	{
		cout << "SOCKCLI_Init failed";
		return lRet;
	}
	SOCKCLI_SetPackHeaderParam(5, GetMsgLength);

	int nKeepAlive = 0;
	int nKeepIdle = 7200;
	int nKeepCount = 3;
	int nKeepInterval = 1;

	printf("usage: PKSockClient ip port. default ip:127.0.0.1, default port:%d\n", DEFAULT_REMOTE_PORT);
	char szAddr[256] = "127.0.0.1";
	int nRemotePort = DEFAULT_REMOTE_PORT;

	if(argc >= 2)
	{
		strcpy(szAddr, argv[1]);
	}
	if(argc >= 3)
	{
		char szPort[256] = {0};
		strcpy(szPort, argv[2]);
		nRemotePort = ::atoi(szPort);
	}
	printf("now: RemoteIP:%s, RemotePort:%d\n", szAddr, nRemotePort);


	hRemoteQueue = SOCKCLI_RegRemoteQueue(szAddr, nRemotePort);
	
	testSendAndRecv();
	SOCKCLI_Finalize();
	return 0;
}
