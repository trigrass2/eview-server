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

#define PROCESSQUEUE_DEFAULT_RECORD_LENGHT				8192				// 每个通知记录大小，可以保存一个点，每个tag点的值最多4096+50字节（时间戳等），必须大于1个tag点所有内容的大小4148的大小
#define PROCESSQUEUE_DEFAULT_RECORD_COUNT				10000				// 通知记录个数

class ACE_Mem_Map;

// 本类目前仅仅在进程管理器和各个服务进程之间通信使用
// 每个队列有两个本地文件:*.CTRL和*.DATA
class CProcessQueue
{
	typedef struct 
	{
		unsigned short usRecordLength;		//	Length Of Current Record
	}tagProcessQueue_RecordHead;			// Header of Record

	// 队列的控制数据结构，不是控制命令
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
	bool					m_bCreateQue;			// 是否调用时创建Queue
	int						m_nTotalRecNo;
	int						m_nRecordLen;	
	ACE_Mem_Map*			m_pShmCtrlBlk;			// 队列的控制数据Map，对应*.CTRL文件
	ACE_Mem_Map*			m_pShmDataArea;			// 队列的控制数据结构Map，对应*.DATA文件

	ACE_Process_Mutex		m_CtrlProcessMutex;		// 队列的控制数据结构的进程间互斥信号量，Process Mutex For Control
	tagProcessQueue_CtrlBlock*	m_pCtrlBlock;		// 内存中指向控制数据结构的指针，对应*.CTRL文件
	char *					m_pDataBuf;				// 内存中指向共享内存的指针

	ACE_Thread_Mutex		m_mutexEnqueBuf;
	char *					m_pszEnqueueBufTmp;	// 入和出队列时，为了减少不断的内存分配导致长期运行new不到内存，使用一下该缓存

private:
	int Setup(int nRecordLen, int nTotalRecNo, bool bCreate);


public:
	CProcessQueue(char *szPath, char *szFileName, bool bCreate = true, int nRecordLen = PROCESSQUEUE_DEFAULT_RECORD_LENGHT, int nRecordCount = PROCESSQUEUE_DEFAULT_RECORD_COUNT);
	virtual ~CProcessQueue();
	
	virtual int OpenRWQueue();
	// enqueue a buffer
	virtual int EnQueue(const char* szBuf, int nLength);
	
	// dequeue，调用者必须先分配好内存才能使用
	virtual int DeQueue(char *szRecordBuf, int *plRecordBufLen);

	// get queue records' number
	virtual int GetQueueRecordNum(int* lCount);
	int IsQueueCreated();
};

#endif//_BDB_QUEUE_H_
