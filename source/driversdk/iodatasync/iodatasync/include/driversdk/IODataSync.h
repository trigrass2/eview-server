#ifndef _IODATA_SYNC_H_
#define _IODATA_SYNC_H_

#include "pkcomm/pkcomm.h"
#include "pklog/pklog.h"
#include "common/eviewdefine.h"

#ifdef _WIN32
#define EXPORT32 __declspec(dllexport)
#else
#define EXPORT32
#endif

typedef struct data_struct
{
	unsigned char chMessageType;	// ��Ϣ����
	char szDriverName[PK_NAME_MAXLEN];  //��������
	char szDeviceName[PK_NAME_MAXLEN];  //�豸����
	char szDataBlockName[PK_NAME_MAXLEN];	//���ݿ�����
	unsigned short nQuality;						// ��������
	int  nLength;									// ���ݳ���
	char *pszData;									// ��ͬ�����ݣ�ʵʱ����
	HighResTime cvTimeStamp;						// ʱ���
	data_struct()
	{
		memset(this, 0, sizeof(data_struct));
	}

	~data_struct()
	{
		if(pszData != NULL)
		{
			delete[] pszData;
		}
		pszData = NULL;
	}

}IOSYNCDATA;

typedef void (*PFN_IOSyncCallback)(IOSYNCDATA* pSyncData, void* pCallbackParam);

// ��������ͬ��DLL��ʼ��
EXPORT32 long IOSync_Init(const char* szDriverName, PFN_IOSyncCallback pfnIOSyncCallback, void* pCallbackParam);

// ��������ͬ��DLL�ͷ���Դ
EXPORT32 long IOSync_Finalize();

// �������ݵ�������������ͬ��ģ��Ľӿ�
EXPORT32 long IOSync_SendtoPeer(IOSYNCDATA *pData);


// ��������ͬ��ģ��ʹ�õ�QUEUE
#define QUEUEID_LOCALDRV_TO 1		// ���ձ����������ݵ�QUEUE

// ����ʹ�õ�QUEUE
#define PROCESSOR_QUEUEID	2		// �ǻ�ڵ�������ݵ�QUEUE
#define	QUEUEID_LOCALDRV_FROM	3	// �������ڽ����ݷ��͵�����ͬ��ģ��

#define PEER_IP		"127.0.0.1"
#define LOCAL_IP	"127.0.0.1"

#define SERVICE_NAME	"IODataSync"

#define DEFAULT_RECV_TIMEOUT	1000	// 1000ms

#define MESSAGE_SYNCDATA	0
#define MESSAGE_CONNECT		1

#endif  // _IODATA_SYNC_H_
