/**************************************************************
 *  Filename:    IODataProcessor.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 同步数据处理类，负责接收同步数据并将数据发送到DIT.
 *
 *  @author:     lijingjing
 *  @version     05/30/2008  lijingjing  Initial Version
**************************************************************/

#ifndef _IODATA_PROCESSOR_H_
#define _IODATA_PROCESSOR_H_

#include <ace/Task.h>
#include <ace/Synch.h>

#include "driversdk/IODataSync.h"
#include "common/CommHelper.h"
#include "pknetque/pknetque.h"

class CIODataProcessor : public ACE_Task<ACE_MT_SYNCH>
{
public:
	CIODataProcessor();
	virtual ~CIODataProcessor();

	virtual int svc();
	
	// 启动同步数据处理线程
	void Start();
	
	// 停止同步数据处理线程
	void Stop();

	// 设置驱动名称
	void SetDriverName(const char* szDriverName);

	// 设置回调函数及参数
	long SetCallback(PFN_IOSyncCallback pfnIOSyncCallback, void* pCallbackParam);

	// 解析同步数据
	long GainSyncData(void* pBuf, long nRecvBytes, IOSYNCDATA *pSyncData);

private:
	// 同步数据
	void ProcessData();

private:
	HQUEUE m_LocalQueue;		// 本地Queue
	HQUEUE m_hRemoteQueue;		// 指定驱动数据同步模块的远程QUEUE
	bool   m_bStop;				// 是否停止驱动数据同步
	
	char  m_szDriverName[PK_NAME_MAXLEN];	//驱动名称
	PFN_IOSyncCallback m_pfnIOSyncCallback;			// 接收到同步数据时的回调函数
	void  *m_pCallbackParam;						// 回调函数的参数
};

#endif // _IODATA_PROCESSOR_H_
