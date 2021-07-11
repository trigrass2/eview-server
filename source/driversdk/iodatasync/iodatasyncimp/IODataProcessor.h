/**************************************************************
 *  Filename:    IODataProcessor.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: ͬ�����ݴ����࣬�������ͬ�����ݲ������ݷ��͵�DIT.
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
	
	// ����ͬ�����ݴ����߳�
	void Start();
	
	// ֹͣͬ�����ݴ����߳�
	void Stop();

	// ������������
	void SetDriverName(const char* szDriverName);

	// ���ûص�����������
	long SetCallback(PFN_IOSyncCallback pfnIOSyncCallback, void* pCallbackParam);

	// ����ͬ������
	long GainSyncData(void* pBuf, long nRecvBytes, IOSYNCDATA *pSyncData);

private:
	// ͬ������
	void ProcessData();

private:
	HQUEUE m_LocalQueue;		// ����Queue
	HQUEUE m_hRemoteQueue;		// ָ����������ͬ��ģ���Զ��QUEUE
	bool   m_bStop;				// �Ƿ�ֹͣ��������ͬ��
	
	char  m_szDriverName[PK_NAME_MAXLEN];	//��������
	PFN_IOSyncCallback m_pfnIOSyncCallback;			// ���յ�ͬ������ʱ�Ļص�����
	void  *m_pCallbackParam;						// �ص������Ĳ���
};

#endif // _IODATA_PROCESSOR_H_
