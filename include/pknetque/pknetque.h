/**************************************************************
 *  Filename:    CVNDK.h
 *  Copyright:   Shanghai Baosight Software Co., Ltd.
 *
 *  Description: CVNDK头文件.
 *
 *  @author:     lijingjing
 *  @version     05/14/2008  lijingjing  Initial Version
 *  @version     11/20/2009  luwen		 添加keepalive
 *  @version     05/28/2011  chenzhiquan  替换ACE_Reactor为boost.asio,取消keepalive（支持keepalive，但不支持参数）
 *										  废弃两次连接之间的间隔，异步模式下不存在因为连接导致其他通讯不正常现象
**************************************************************/

#ifndef _PKQUE_H_
#define _PKQUE_H_
#include "stdio.h"

#define NDK_NO_TIMEOUT					-1		// 不设置超时，无限等待(无限等待改为等待时间为1天from NetQueue1.1.0)
#define RANDOM_QUEUEID					0		// 随机生成QueueID

// 连接状态
#define	PKQUE_CONNECTED					0		// 连接已建立
#define	PKQUE_DISCONNECTED				1		// 连接未建立

// 发送选项
#define PKQUE_SYNC_SEND					0		// 同步发送
#define PKQUE_ASYNC_SEND				1		// 异步发送
#define PKQUE_INIT_OPTION				2		// 采用PKQUE_Init时指定的发送选项

#define PKQUE_ACCEPT_SINGLE_CONNECTION	1		//一个ip同时只允许一个连接
#define PKQUE_ACCEPT_MULTI_CONNECTION	0		//一个ip允许多个连接同时建立

#define DEFAULT_CONN_TIMEOUT			10		// 默认连接超时, 1秒
#define DEFAULT_SEND_TIMEOUT			60000	// 默认发送超时, 60秒

#define KEEP_ALIVE_ON					1       //开启keepalive
#define KEEP_ALIVE_OFF					0		//关闭keepalive


#ifdef _WIN32
#	ifdef pknetque_EXPORTS
#		define PKQUE_DLLEXPORT extern "C"  __declspec(dllexport) 
#	else
#		define PKQUE_DLLEXPORT extern "C" __declspec(dllimport) 
#	endif
#else
#	define PKQUE_DLLEXPORT extern "C" __attribute__ ((visibility ("default")))
#endif

typedef void* HQUEUE;
typedef void (*PFN_ConnCallBack)(HQUEUE hLocalQue, HQUEUE hDestQue, int lConnectState);
typedef void (*PFN_SendCallBack)(char *pUserData, int lTotalPackage, int lCurrPackage);


PKQUE_DLLEXPORT int	PKQUE_Init(int nSyncFlag = PKQUE_SYNC_SEND, PFN_ConnCallBack pfnConnState = NULL);

PKQUE_DLLEXPORT int	PKQUE_Listen(unsigned short nPort, int nSingleConn = PKQUE_ACCEPT_MULTI_CONNECTION);

PKQUE_DLLEXPORT HQUEUE	PKQUE_RegRemoteQueue(int lQueueID, const char* IP, unsigned short nPort);

PKQUE_DLLEXPORT HQUEUE	PKQUE_RegLocalQueue(int lQueueID);

PKQUE_DLLEXPORT int	PKQUE_Recv(HQUEUE hLocalQueue, HQUEUE *hSrcQueue, char **pRecvBuf, int *plRecvBytes, int msTimeout = NDK_NO_TIMEOUT);

PKQUE_DLLEXPORT int	PKQUE_Send(HQUEUE hDestQueue, HQUEUE hLocalQueue, const char *pSendBuf, int lSendBytes, int nSyncFlag = PKQUE_INIT_OPTION, int lPackageLength = 0, PFN_SendCallBack pfnSendCallback = NULL, char *pParam = NULL);

PKQUE_DLLEXPORT int	PKQUE_SendWithStampID(HQUEUE hDestQueue, HQUEUE hLocalQueue, const char *pSendBuf, int lSendBytes, int nSyncFlag = PKQUE_INIT_OPTION);

PKQUE_DLLEXPORT int	PKQUE_RecvWithStampID(HQUEUE hLocalQueue, HQUEUE *hSrcQueue, char **pRecvBuf, int *plRecvBytes, int msTimeout = NDK_NO_TIMEOUT);

PKQUE_DLLEXPORT int	PKQUE_Finalize();

PKQUE_DLLEXPORT int	PKQUE_FreeMem(char* &pszBuff);

PKQUE_DLLEXPORT int	PKQUE_FreeQueue(HQUEUE &hQueue); 

PKQUE_DLLEXPORT int	PKQUE_GetQueueID(HQUEUE hQueue, int *pnQueueID);

PKQUE_DLLEXPORT int	PKQUE_GetStampID(HQUEUE hQueue, unsigned int *pnStampID);

PKQUE_DLLEXPORT int	PKQUE_GetConnectStat(HQUEUE hRemoteQueue); //获取连接状态

PKQUE_DLLEXPORT int	PKQUE_Connect(HQUEUE hRemoteQueue, int nConnTimeout = DEFAULT_CONN_TIMEOUT);

PKQUE_DLLEXPORT int	PKQUE_DisConnect(HQUEUE hRemoteQueue);

PKQUE_DLLEXPORT int	PKQUE_SetConnTimeout(int nConnTimeout);

PKQUE_DLLEXPORT int	PKQUE_SetSendTimeout(int nConnTimeoutInMs);

PKQUE_DLLEXPORT	HQUEUE	PKQUE_CopySrcQueue(HQUEUE hQueue);

PKQUE_DLLEXPORT int	PKQUE_SetKeepAliveFlag(int nKeepAlive);

PKQUE_DLLEXPORT int QUE_GetQueueAddr(HQUEUE hQueue, char* szIPStr, int nIPStrBufLen, unsigned short *pnPort, unsigned int *pnIP);
#endif
