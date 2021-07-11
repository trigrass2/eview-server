/**************************************************************
 *  Filename:    TaskGroup.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: �豸����Ϣʵ����.
 *
 *  @author:     lijingjing
 *  @version     05/28/2008  lijingjing  Initial Version
**************************************************************/
#ifndef _TAG_PROCESS_TASK_H_
#define _TAG_PROCESS_TASK_H_

#include "ace/Reactor.h"
#include "ace/Select_Reactor.h"
#include "ace/Acceptor.h"
#include <map>
#include <utility>
#include "PredefTagHandler.h"
#include "SimTagHandler.h"
#include "MemTagHandler.h"

class CTagProcessTask: public ACE_Task<ACE_MT_SYNCH>
{
	friend class ACE_Singleton<CTagProcessTask, ACE_Thread_Mutex>;
public:
	ACE_Reactor*	m_pReactor;

	virtual int svc ();
	bool m_bExit;

	CPredefTagHandler m_predefTagHandler;
	CSimTagHandler m_simTagHandler;
	CMemTagHandler m_memTagHandler;

protected:
	char *m_pQueRecBuffer;	// һ�η��Ͷ��TAG����ڴ�ָ��
	unsigned int		m_nQueRecBufferLen;	// һ�η��Ͷ��TAG��ļ�¼�ڴ��С

	char *m_pTagVTQBuffer;	// 1��TAG����ڴ�ָ��
	unsigned int		m_nTagVTQBufferLen;	// 1��TAG����ڴ��С

	char *m_pTagValueBuffer;	// 1��TAG����ڴ�ָ��
	unsigned int		m_nTagValueBufferLen;	// 1��TAG����ڴ��С
public:
	CTagProcessTask();
	virtual ~CTagProcessTask();

	// �����߳�
	int Start();
	// ֹͣ�߳�
	void Stop();

	// Group�߳�����ʱ
	void OnStart();

	// Group�߳�ֹͣʱ
	void OnStop();

public:
	virtual int handle_output (ACE_HANDLE fd = ACE_INVALID_HANDLE);
	int PostWriteTagCmd(CDataTag *pTag, string strTagValue);
	int GetTagDataBuffer(CDataTag *pTag, char *szTagBuffer, int nBufLen, unsigned int nCurSec, unsigned int nCurMSec, const char *szTagValue, int nTagDataLen);
	int SendTagsData(vector<CDataTag *> &vecTags);
};

#define TAGPROC_TASK ACE_Singleton<CTagProcessTask, ACE_Thread_Mutex>::instance()
#endif  // _DEVICE_GROUP_H_
