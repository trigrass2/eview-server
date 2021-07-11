/**************************************************************
 *  Filename:    CVNDK.h
 *  Copyright:   Shanghai Peakinfo Co., Ltd.
 *
 *  Description: PKSockServer header file
 *
**************************************************************/
#ifndef _PEAK_PKSOCK_SERVER_H_
#define _PEAK_PKSOCK_SERVER_H_

#define SOCK_NO_TIMEOUT						-1			// �����ó�ʱ�����޵ȴ�(���޵ȴ���Ϊ�ȴ�ʱ��Ϊ1��)
#define RANDOM_QUEUEID						    0			// �������QueueID

// ����ѡ��
#define SOCK_SEND_SYNC						    0				// ͬ������
#define SOCK_SEND_ASYNC						1				// �첽����
#define SOCK_SEND_INIT_OPTION				2				// ����SOCK_Initʱָ���ķ���ѡ��

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
typedef void (*PFN_SendCallBack)(void *pUserData, int lTotalPackageNum, int lCurrPackageNum); // ���͵Ļص�����������Ҫ��������ʱ�������м���Ҫ���ؽ��ȣ���Ҫ�趨ÿ�����ĳ���
typedef int (*PFN_GetPackTotalLen)(char *pHeader, int nHeaderLen); // �õ������ܳ��ȡ�������ظ������ʾ��������Ͽ�����
#endif // PKSOCK_CALLBACK_DEFINE

PKSOCK_DLLEXPORT int	SOCKSVR_Init(PFN_OnConnChange pfnOnConnChangeCallback = 0, int nSyncFlag = SOCK_SEND_SYNC);

PKSOCK_DLLEXPORT int	SOCKSVR_Listen(unsigned short nPort, int nSingleConn = 0);

PKSOCK_DLLEXPORT int	SOCKSVR_Recv(HQUEUE *hRemoteQueue, char **ppszRecvBuf, int *pnRecvBytes, int nTimeoutMS = SOCK_NO_TIMEOUT);

PKSOCK_DLLEXPORT int	SOCKSVR_Send(HQUEUE hDestQueue, const char *pszSendBuf, int lSendBytes, int nSendTimeoutInMS = 10000, PFN_SendCallBack pfnSendCallback = 0, void *pCallbackParam = 0, int nSyncFlag = SOCK_SEND_INIT_OPTION);

PKSOCK_DLLEXPORT int	SOCKSVR_FreeMem(char *pszBuff);

PKSOCK_DLLEXPORT int	SOCKSVR_Finalize();

PKSOCK_DLLEXPORT int	SOCKSVR_GetQueueAddr(HQUEUE hQueue, char* szIPStr, int nIPStrBufLen, unsigned short *pnPort, unsigned int *pnIP = 0); // szIP ������Ļ���������

PKSOCK_DLLEXPORT int	SOCKSVR_FreeQueue(HQUEUE hQueue);

PKSOCK_DLLEXPORT int	SOCKSVR_GetConnectStat(HQUEUE hRemoteQueue); //��ȡ����״̬

PKSOCK_DLLEXPORT int	SOCKSVR_DisConnect(HQUEUE hRemoteQueue);

PKSOCK_DLLEXPORT int	SOCKSVR_SetSendTimeout(int nConnTimeoutInMs);

PKSOCK_DLLEXPORT HQUEUE	SOCKSVR_CopyQueue(HQUEUE hQueue);

PKSOCK_DLLEXPORT int	SOCKSVR_SetPackHeaderParam(int nPackHeaderLen, PFN_GetPackTotalLen pfnGetPackTotalLen);
#endif // _PEAK_PKSOCK_SERVER_H_
