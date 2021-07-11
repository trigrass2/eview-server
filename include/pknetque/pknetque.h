/**************************************************************
 *  Filename:    CVNDK.h
 *  Copyright:   Shanghai Baosight Software Co., Ltd.
 *
 *  Description: CVNDKͷ�ļ�.
 *
 *  @author:     lijingjing
 *  @version     05/14/2008  lijingjing  Initial Version
 *  @version     11/20/2009  luwen		 ���keepalive
 *  @version     05/28/2011  chenzhiquan  �滻ACE_ReactorΪboost.asio,ȡ��keepalive��֧��keepalive������֧�ֲ�����
 *										  ������������֮��ļ�����첽ģʽ�²�������Ϊ���ӵ�������ͨѶ����������
**************************************************************/

#ifndef _PKQUE_H_
#define _PKQUE_H_
#include "stdio.h"

#define NDK_NO_TIMEOUT					-1		// �����ó�ʱ�����޵ȴ�(���޵ȴ���Ϊ�ȴ�ʱ��Ϊ1��from NetQueue1.1.0)
#define RANDOM_QUEUEID					0		// �������QueueID

// ����״̬
#define	PKQUE_CONNECTED					0		// �����ѽ���
#define	PKQUE_DISCONNECTED				1		// ����δ����

// ����ѡ��
#define PKQUE_SYNC_SEND					0		// ͬ������
#define PKQUE_ASYNC_SEND				1		// �첽����
#define PKQUE_INIT_OPTION				2		// ����PKQUE_Initʱָ���ķ���ѡ��

#define PKQUE_ACCEPT_SINGLE_CONNECTION	1		//һ��ipͬʱֻ����һ������
#define PKQUE_ACCEPT_MULTI_CONNECTION	0		//һ��ip����������ͬʱ����

#define DEFAULT_CONN_TIMEOUT			10		// Ĭ�����ӳ�ʱ, 1��
#define DEFAULT_SEND_TIMEOUT			60000	// Ĭ�Ϸ��ͳ�ʱ, 60��

#define KEEP_ALIVE_ON					1       //����keepalive
#define KEEP_ALIVE_OFF					0		//�ر�keepalive


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

PKQUE_DLLEXPORT int	PKQUE_GetConnectStat(HQUEUE hRemoteQueue); //��ȡ����״̬

PKQUE_DLLEXPORT int	PKQUE_Connect(HQUEUE hRemoteQueue, int nConnTimeout = DEFAULT_CONN_TIMEOUT);

PKQUE_DLLEXPORT int	PKQUE_DisConnect(HQUEUE hRemoteQueue);

PKQUE_DLLEXPORT int	PKQUE_SetConnTimeout(int nConnTimeout);

PKQUE_DLLEXPORT int	PKQUE_SetSendTimeout(int nConnTimeoutInMs);

PKQUE_DLLEXPORT	HQUEUE	PKQUE_CopySrcQueue(HQUEUE hQueue);

PKQUE_DLLEXPORT int	PKQUE_SetKeepAliveFlag(int nKeepAlive);

PKQUE_DLLEXPORT int QUE_GetQueueAddr(HQUEUE hQueue, char* szIPStr, int nIPStrBufLen, unsigned short *pnPort, unsigned int *pnIP);
#endif
