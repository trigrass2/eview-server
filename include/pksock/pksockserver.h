/**************************************************************
 *  Filename:    CVNDK.h
 *  Copyright:   Shanghai Peakinfo Co., Ltd.
 *
 *  Description: PKSockServer header file
 *
**************************************************************/
#ifndef _PEAK_PKSOCK_SERVER_H_
#define _PEAK_PKSOCK_SERVER_H_

#define SOCK_NO_TIMEOUT						-1			// 不设置超时，无限等待(无限等待改为等待时间为1天)
#define RANDOM_QUEUEID						    0			// 随机生成QueueID

// 发送选项
#define SOCK_SEND_SYNC						    0				// 同步发送
#define SOCK_SEND_ASYNC						1				// 异步发送
#define SOCK_SEND_INIT_OPTION				2				// 采用SOCK_Init时指定的发送选项

#ifdef _WIN32
#	ifdef pksockserver_EXPORTS
#		define PKSOCK_DLLEXPORT extern "C"  __declspec(dllexport) 
#	else
#		define PKSOCK_DLLEXPORT extern "C" __declspec(dllimport) 
#	endif
#else
#	define PKSOCK_DLLEXPORT extern "C" __attribute__ ((visibility ("default")))
#endif

#ifndef PKSOCK_CALLBACK_DEFINE
#define PKSOCK_CALLBACK_DEFINE
typedef void* HQUEUE;
typedef void (*PFN_OnConnChange)(HQUEUE hDestQue, int lConnectState);
typedef void (*PFN_SendCallBack)(void *pUserData, int lTotalPackageNum, int lCurrPackageNum); // 发送的回调函数。当需要分批发送时（比如中间需要返回进度，需要设定每个包的长度
typedef int (*PFN_GetPackTotalLen)(char *pHeader, int nHeaderLen); // 得到包的总长度。如果返回负数则表示包错误，需断开重连
#endif // PKSOCK_CALLBACK_DEFINE

PKSOCK_DLLEXPORT int	SOCKSVR_Init(PFN_OnConnChange pfnOnConnChangeCallback = 0, int nSyncFlag = SOCK_SEND_SYNC);

PKSOCK_DLLEXPORT int	SOCKSVR_Listen(unsigned short nPort, int nSingleConn = 0);

PKSOCK_DLLEXPORT int	SOCKSVR_Recv(HQUEUE *hRemoteQueue, char **ppszRecvBuf, int *pnRecvBytes, int nTimeoutMS = SOCK_NO_TIMEOUT);

PKSOCK_DLLEXPORT int	SOCKSVR_Send(HQUEUE hDestQueue, const char *pszSendBuf, int lSendBytes, int nSendTimeoutInMS = 10000, PFN_SendCallBack pfnSendCallback = 0, void *pCallbackParam = 0, int nSyncFlag = SOCK_SEND_INIT_OPTION);

PKSOCK_DLLEXPORT int	SOCKSVR_FreeMem(char *pszBuff);

PKSOCK_DLLEXPORT int	SOCKSVR_Finalize();

PKSOCK_DLLEXPORT int	SOCKSVR_GetQueueAddr(HQUEUE hQueue, char* szIPStr, int nIPStrBufLen, unsigned short *pnPort, unsigned int *pnIP = 0); // szIP 必须给的缓冲区够大

PKSOCK_DLLEXPORT int	SOCKSVR_FreeQueue(HQUEUE hQueue);

PKSOCK_DLLEXPORT int	SOCKSVR_GetConnectStat(HQUEUE hRemoteQueue); //获取连接状态

PKSOCK_DLLEXPORT int	SOCKSVR_DisConnect(HQUEUE hRemoteQueue);

PKSOCK_DLLEXPORT int	SOCKSVR_SetSendTimeout(int nConnTimeoutInMs);

PKSOCK_DLLEXPORT HQUEUE	SOCKSVR_CopyQueue(HQUEUE hQueue);

PKSOCK_DLLEXPORT int	SOCKSVR_SetPackHeaderParam(int nPackHeaderLen, PFN_GetPackTotalLen pfnGetPackTotalLen);
#endif // _PEAK_PKSOCK_SERVER_H_
