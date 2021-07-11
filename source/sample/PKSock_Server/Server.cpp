#include "pksock/pksockserver.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cstring>
using namespace std;

#ifndef PK_SUCCESS
#define PK_SUCCESS							0								// iCV系统中所有成功消息的返回码
#endif

#define DEFAULT_LOCAL_PORT		12000
#define DEFAULT_LOCAL_QUEUE		10
unsigned short g_nPort = DEFAULT_LOCAL_PORT;
int nKeepAlive = 0;
int nKeepIdle = 7200;
int nKeepCount = 3;
int nKeepInterval = 1;

void OnConnChange(HQUEUE hDestQue, int lConnectState)
{
	char szIP[32];
	unsigned short nPort;
	printf("OnConnChanged, %d\n", lConnectState);
	//SOCKSVR_GetQueueAddr(hDestQue, szIP, sizeof(szIP), &nPort);

	//printf("OnConnChange: %s:%d\n", szIP, nPort);
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

int testListen()
{	
	SOCKSVR_Finalize();
	SOCKSVR_Init(OnConnChange, SOCK_SEND_SYNC);
	SOCKSVR_SetPackHeaderParam(5, GetMsgLength);

	if (SOCKSVR_Listen(g_nPort) != PK_SUCCESS)
	{
		printf("Listen failed: %d! \n", g_nPort);
	}
	else
	{
		printf("Listen port %d success!\n", g_nPort);
	}

	char* szRecvBuf = 0;
	int nRecvBytes = 0;
	HQUEUE hSrcQueue = 0;
	int  ret = 0;
	
	while (1)
	{
		ret = SOCKSVR_Recv(&hSrcQueue, &szRecvBuf, &nRecvBytes);

 		if (ret != PK_SUCCESS)
		{
 			printf("Receive failed! \n");
		}
		else
		{
			printf("Server Recv, data(%d Bytes): %s.\n",
				nRecvBytes, szRecvBuf + 5);

			if (SOCKSVR_GetConnectStat(hSrcQueue) == 1)
			{
				ret = SOCKSVR_Send(hSrcQueue, (const char *)szRecvBuf, nRecvBytes);
				if(ret != PK_SUCCESS)
				{
					printf("Send %d failed! \n", nRecvBytes);
				}
				else
					printf("Send %d success! \n", nRecvBytes);

				SOCKSVR_FreeMem(szRecvBuf);
				SOCKSVR_FreeQueue(hSrcQueue);
			}
			else
			{
				printf("Server Recv,not connected.\n");
			}
		}
	}

	return 0;
}

int main(int argc, char**argv)
{
	testListen();	
	
	SOCKSVR_Finalize();
	return 0;
}
