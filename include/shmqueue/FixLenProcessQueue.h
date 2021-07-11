/**************************************************************
 *  Filename:    ShmQueue.h
 *  Copyright:   Shanghai Baosight Software Co., Ltd.
 *
 *  Description: ShmQueue.
 *
 *  @author:     chenzhiquan
 *  @version     06/05/2008  chenzhiquan  Initial Version
**************************************************************/
#ifndef _PROCESS_QUEUE_H_
#define _PROCESS_QUEUE_H_

#include <ace/Process_Mutex.h>
#include <ace/Thread_Mutex.h>
#include <ace/OS_NS_string.h>
#include "shmqueue/errcode_shmqueue.h"
#include <string>
using namespace std;

#define PROCESSQUEUE_DEFAULT_RECORD_LENGHT				8192				// ÿ��֪ͨ��¼��С�����Ա���һ���㣬ÿ��tag���ֵ���4096+50�ֽڣ�ʱ����ȣ����������1��tag���������ݵĴ�С4148�Ĵ�С
#define PROCESSQUEUE_DEFAULT_RECORD_COUNT				10000				// ֪ͨ��¼����

class ACE_Mem_Map;

// ����Ŀǰ�����ڽ��̹������͸����������֮��ͨ��ʹ��
// ÿ�����������������ļ�:*.CTRL��*.DATA
class CProcessQueue
{
	typedef struct 
	{
		unsigned short usRecordLength;		//	Length Of Current Record
	}tagProcessQueue_RecordHead;			// Header of Record

	// ���еĿ������ݽṹ�����ǿ�������
	typedef struct  
	{
		ACE_UINT16	nReadRecordNo;
		ACE_UINT16	nWriteRecordNo;
		ACE_UINT16	nTotalRecords;
		ACE_UINT16	nRecordLength;
	}tagProcessQueue_CtrlBlock; 

private:
	string m_strQueFilePath;
	string m_strQueFileName;
	bool					m_bCreateQue;			// �Ƿ����ʱ����Queue
	int						m_nTotalRecNo;
	int						m_nRecordLen;	
	ACE_Mem_Map*			m_pShmCtrlBlk;			// ���еĿ�������Map����Ӧ*.CTRL�ļ�
	ACE_Mem_Map*			m_pShmDataArea;			// ���еĿ������ݽṹMap����Ӧ*.DATA�ļ�

	ACE_Process_Mutex		m_CtrlProcessMutex;		// ���еĿ������ݽṹ�Ľ��̼以���ź�����Process Mutex For Control
	tagProcessQueue_CtrlBlock*	m_pCtrlBlock;		// �ڴ���ָ��������ݽṹ��ָ�룬��Ӧ*.CTRL�ļ�
	char *					m_pDataBuf;				// �ڴ���ָ�����ڴ��ָ��

	ACE_Thread_Mutex		m_mutexEnqueBuf;
	char *					m_pszEnqueueBufTmp;	// ��ͳ�����ʱ��Ϊ�˼��ٲ��ϵ��ڴ���䵼�³�������new�����ڴ棬ʹ��һ�¸û���

private:
	int Setup(int nRecordLen, int nTotalRecNo, bool bCreate);


public:
	CProcessQueue(char *szPath, char *szFileName, bool bCreate = true, int nRecordLen = PROCESSQUEUE_DEFAULT_RECORD_LENGHT, int nRecordCount = PROCESSQUEUE_DEFAULT_RECORD_COUNT);
	virtual ~CProcessQueue();
	
	virtual int OpenRWQueue();
	// enqueue a buffer
	virtual int EnQueue(const char* szBuf, int nLength);
	
	// dequeue�������߱����ȷ�����ڴ����ʹ��
	virtual int DeQueue(char *szRecordBuf, int *plRecordBufLen);

	// get queue records' number
	virtual int GetQueueRecordNum(int* lCount);
	int IsQueueCreated();
};

#endif//_BDB_QUEUE_H_
