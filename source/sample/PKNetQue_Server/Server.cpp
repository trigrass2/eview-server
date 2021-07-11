#include "pksock/pksock.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
using namespace std;

#ifndef PK_SUCCESS
#define PK_SUCCESS							0								// iCV系统中所有成功消息的返回码
#endif

#define DEFAULT_PORT	12000
#define DEFAULT_LOCAL_QUEUE	10
unsigned short g_nPort = DEFAULT_PORT;
int nKeepAlive = 0;
int nKeepIdle = 7200;
int nKeepCount = 3;
int nKeepInterval = 1;

void OnConnChange(HQUEUE hLocalQue, HQUEUE hDestQue, int lConnectState)
{
	char szIP[32];
	unsigned short nPort;
	printf("OnConnChanged, %d\n", lConnectState);
	//SOCK_GetQueueAddr(hDestQue, szIP, sizeof(szIP), &nPort);

	//printf("OnConnChange: %s:%d\n", szIP, nPort);
}

int test1()
{	
	SOCK_Finalize();
	SOCK_Init(OnConnChange, SOCK_SEND_SYNC);

	if (SOCK_Listen(g_nPort + 1) != PK_SUCCESS)
	{
		printf("Listen failed: %d! \n", g_nPort);
	}

	//SOCK_SetKeepAlive(nKeepAlive, nKeepIdle, nKeepInterval, nKeepCount);
	if (SOCK_Listen(g_nPort) != PK_SUCCESS)
	{
		printf("Listen failed: %d! \n", g_nPort);
	}
	else
	{
		printf("Listen port %d success!\n", g_nPort);
	}

	HQUEUE hLocalQueue = SOCK_RegLocalQueue(DEFAULT_LOCAL_QUEUE);

	char* szRecvBuf = 0;
	int nRecvBytes = 0;
	HQUEUE hSrcQueue = 0;
	int  ret = 0;
	
	while (1)
	{
		ret = SOCK_Recv(hLocalQueue, &hSrcQueue, &szRecvBuf, &nRecvBytes);

 		if (ret != PK_SUCCESS)
		{
 			printf("Receive failed! \n");
		}
		else
		{
			int nLocalQueueID, nRemoteQueueID;
			SOCK_GetQueueID(hLocalQueue, &nLocalQueueID);
			SOCK_GetQueueID(hSrcQueue, &nRemoteQueueID);

			printf("Server Recv(Que %d --> Que %d), data(%d Bytes): %s.\n",
				nRemoteQueueID, nLocalQueueID, nRecvBytes, szRecvBuf);

			if (SOCK_GetConnectStat(hSrcQueue) == 1)
			{
				ret = SOCK_Send(hSrcQueue, hLocalQueue, (const char *)szRecvBuf, nRecvBytes);

				if(ret != PK_SUCCESS)
				{
					printf("Send failed! \n");
				}
				
				SOCK_FreeMem(szRecvBuf);
				SOCK_FreeQueue(hSrcQueue);
			}
		}
	}
		
	SOCK_FreeQueue(hLocalQueue);

	return 0;
}

int main(int argc, char**argv)
{
	SOCK_Init(SOCK_SEND_SYNC);
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
	//	SOCK_SetKeepAlive(nKeepAlive, nKeepIdle, nKeepInterval, nKeepCount);
	//}
	test1();	
	
	SOCK_Finalize();
	return 0;
}
