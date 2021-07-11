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
	unsigned char chMessageType;	// 消息类型
	char szDriverName[PK_NAME_MAXLEN];  //驱动名称
	char szDeviceName[PK_NAME_MAXLEN];  //设备名称
	char szDataBlockName[PK_NAME_MAXLEN];	//数据块名称
	unsigned short nQuality;						// 数据质量
	int  nLength;									// 数据长度
	char *pszData;									// 待同步数据，实时数据
	HighResTime cvTimeStamp;						// 时间戳
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

// 驱动数据同步DLL初始化
EXPORT32 long IOSync_Init(const char* szDriverName, PFN_IOSyncCallback pfnIOSyncCallback, void* pCallbackParam);

// 驱动数据同步DLL释放资源
EXPORT32 long IOSync_Finalize();

// 发送数据到本地驱动数据同步模块的接口
EXPORT32 long IOSync_SendtoPeer(IOSYNCDATA *pData);


// 驱动数据同步模块使用的QUEUE
#define QUEUEID_LOCALDRV_TO 1		// 接收本机驱动数据的QUEUE

// 驱动使用的QUEUE
#define PROCESSOR_QUEUEID	2		// 非活动节点接收数据的QUEUE
#define	QUEUEID_LOCALDRV_FROM	3	// 驱动用于将数据发送到数据同步模块

#define PEER_IP		"127.0.0.1"
#define LOCAL_IP	"127.0.0.1"

#define SERVICE_NAME	"IODataSync"

#define DEFAULT_RECV_TIMEOUT	1000	// 1000ms

#define MESSAGE_SYNCDATA	0
#define MESSAGE_CONNECT		1

#endif  // _IODATA_SYNC_H_
