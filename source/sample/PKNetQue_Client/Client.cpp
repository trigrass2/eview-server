#include "pksock/pksock.h"
#include <stdio.h>
#include <string.h>
#include "ace/OS_NS_unistd.h"
#include "iostream"
using namespace std;


#ifndef PK_SUCCESS
#define PK_SUCCESS							0								// iCV系统中所有成功消息的返回码
#endif

#define DEFAULT_PORT1						10000

#define DEFAULT_REMOTE_QUEUEID				10
#define DEFAULT_LOCAL_QUEUEID				9
#define	DEFAULT_BUF_LEN						1000000

typedef int (* PFN_Send)(HQUEUE hLocalQueue, HQUEUE hRemoteQueue, char* const &buf);
typedef int (* PFN_Recv)(HQUEUE hLocalQueue, HQUEUE *phSrcQueue, char* &szRecvBuf, int* pnRecvBytes);


//HQUEUE hRemoteQueue = SOCK_RegRemoteQueue(DEFAULT_REMOTE_QUEUEID, "10.25.77.179", DEFAULT_PORT1);
HQUEUE hRemoteQueue = NULL;

void SendCallback(void *pParam, int lTotalPackage, int lCurrPackage)
{
	printf("Sent(%d/%d)\n", lCurrPackage, lTotalPackage);
}



void ConnCallBack(HQUEUE hLocalQue, HQUEUE hDestQue, int nConnectState)
{
	int nLocalQueID = -1, nRemoteQueID = -1;
	SOCK_GetQueueID(hLocalQue, &nLocalQueID);
	SOCK_GetQueueID(hDestQue, &nRemoteQueID);
	char szRemoteIP[32];
	unsigned short nRemotePort;
	SOCK_GetQueueAddr(hDestQue, szRemoteIP, sizeof(szRemoteIP), &nRemotePort);

	char szLocalIP[32];
	unsigned short nLocalPort;
	SOCK_GetQueueAddr(hLocalQue, szLocalIP, sizeof(szLocalIP), &nLocalPort);

	switch(nConnectState)
	{
	case 1:
		printf("SOCK_CONNECTED(RemoteQue: %d, RemoteIP:%s:%d, LocalIP:%s:%d).\n", nRemoteQueID, szRemoteIP, nRemotePort, szLocalIP, nLocalPort);
		break;

	case 0:
		printf("SOCK_DISCONNECTED (LocalQue: %d, RemoteIP:%s:%d, LocalIP:%s:%d).\n", nLocalQueID, szRemoteIP, nRemotePort, szLocalIP, nLocalPort);
		break;

	default:
		printf("nConnectState unknown: %d.\n", nConnectState);		
	}
}

int testStub(PFN_Send send, PFN_Recv recv)
{
	HQUEUE hLocalQueue = SOCK_RegLocalQueue(DEFAULT_LOCAL_QUEUEID);
	
	//SOCK_SetConnTimeoutDelay(2);
	
	char buf[DEFAULT_BUF_LEN];
	memset(buf, 0, DEFAULT_BUF_LEN);
	strcpy(buf, "_test_data_");
	
	HQUEUE hSrcQueue = 0;
	
	int i = 0;
	
	while (1)
	{
		i++;
		if (i % 10 == 0)
		{
			printf("%d\n", i);
		}
		
		int ret = /*PK_SUCCESS;//*/send(hLocalQueue, hRemoteQueue, buf);
		
		if ( ret == PK_SUCCESS)
		{
			printf("SOCK_CONNECTED\n");
			
			if (ret != PK_SUCCESS)
			{
				printf("Send failed! \n");
				continue;
			}
			
			char* szRecvBuf = 0;
			int lRecvBytes = 0;
			
			ret = recv(hLocalQueue, &hSrcQueue, szRecvBuf, &lRecvBytes);
			
			if (ret != PK_SUCCESS)
			{
				SOCK_FreeMem(szRecvBuf);
				SOCK_FreeQueue(hSrcQueue);
				printf("Receive failed! \n");
			}
			else
			{
				int nRemoteQueueID;
				SOCK_GetQueueID(hSrcQueue, &nRemoteQueueID);
				
				int nLocalQueueID;
				SOCK_GetQueueID(hLocalQueue, &nLocalQueueID);
				
				printf("Client(Que %d --> Que %d), Recv(%d Bytes): %s.\n",
					nRemoteQueueID, nLocalQueueID, lRecvBytes, szRecvBuf);	
				SOCK_FreeMem(szRecvBuf);
				SOCK_FreeQueue(hSrcQueue);
			}
		}
		else
		{
			printf("SOCK_DISCONNECTED\n");
		}
		
		ACE_OS::sleep(1);
	}
	
	
	
	SOCK_FreeQueue(hRemoteQueue);
	SOCK_FreeQueue(hLocalQueue);
	
	return 0;
}

int test1_send(HQUEUE& hLocalQueue, HQUEUE& hRemoteQueue, char* const &buf)
{
	return SOCK_SendWithStampID(hRemoteQueue, hLocalQueue, buf, strlen(buf) + 1);
}

int test1_recv(HQUEUE &hLocalQueue, HQUEUE &hSrcQueue, char* &szRecvBuf, int& lRecvBytes)
{	
	return SOCK_RecvWithStampID(hLocalQueue, &hSrcQueue, &szRecvBuf, &lRecvBytes);
}

int test2_send(HQUEUE& hLocalQueue, HQUEUE& hRemoteQueue, char* const &buf)
{
	return SOCK_Send(hRemoteQueue, hLocalQueue, buf, strlen(buf) + 999, 2, SendCallback, NULL);
}
 
int test2_recv(HQUEUE &hLocalQueue, HQUEUE &hSrcQueue, char* &szRecvBuf, int& lRecvBytes)
{	
	return SOCK_Recv(hLocalQueue, &hSrcQueue, &szRecvBuf, &lRecvBytes,1000);
}

int test3_send(HQUEUE& hLocalQueue, HQUEUE& hRemoteQueue, char* const &buf)
{
	return SOCK_Send(hRemoteQueue, hLocalQueue, buf, strlen(buf) + 1, SOCK_SEND_ASYNC);
}

int test3_recv(HQUEUE &hLocalQueue, HQUEUE &hSrcQueue, char* &szRecvBuf, int& lRecvBytes)
{	
	return SOCK_Recv(hLocalQueue, &hSrcQueue, &szRecvBuf, &lRecvBytes,1000);
}

int test4_send(HQUEUE hLocalQueue, HQUEUE hRemoteQueue, char* const &buf)
{
	return SOCK_Send(hRemoteQueue, hLocalQueue, buf, strlen(buf) + 1, 1000, NULL, NULL, SOCK_SEND_SYNC);
}

int test4_recv(HQUEUE hLocalQueue, HQUEUE *phSrcQueue, char* &pszRecvBuf, int *pnRecvBytes)
{	
	return SOCK_Recv(hLocalQueue, phSrcQueue, &pszRecvBuf, pnRecvBytes,1000000);
}

int main(int argc, char**argv)
{
	long lRet = SOCK_Init(ConnCallBack, SOCK_SEND_SYNC);
	if (PK_SUCCESS != lRet)
	{
		cout << "SOCK_Init failed";
		return lRet;
	}

	char szAddr[256] = "127.0.0.1";
	int nKeepAlive = 0;
	int nKeepIdle = 7200;
	int nKeepCount = 3;
	int nKeepInterval = 1;
	cout << "Please input the remote server address: 127.0.0.1";
	//cin >> szAddr;
	if(strlen(szAddr) == 0)
		strcpy(szAddr, "127.0.0.1");

	hRemoteQueue = SOCK_RegRemoteQueue(DEFAULT_REMOTE_QUEUEID, szAddr, 12333);
	
	//cout << "Please input the nKeepAlive:";
	//cin >> nKeepAlive;
	//if (nKeepAlive != 0)
	//{
	//	cout << "Please input the nKeepIdle:";
	//	cin >> nKeepIdle;
	//	cout << "Please input the nKeepInterval:";
	//	cin >> nKeepInterval;
	//	cout << "Please input the nKeepCount:";
	//	cin >> nKeepCount;
	//	//SOCK_SetKeepAlive(nKeepAlive, nKeepIdle, nKeepInterval, nKeepCount);
	//}

	
	testStub(test4_send, test4_recv);
	SOCK_Finalize();
	return 0;
}
