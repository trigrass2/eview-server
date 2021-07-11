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
#include "shmqueue/ProcessQueue.h"
#include "shmqueue/errcode_shmqueue.h"
#include <string>
using namespace std;

#define	PROCESSQUEUE_PROCESSMGR_CONTROL_AREASIZE		40 * 1024			// 进程管理器的控制命令共享内存大小,固定的，因为先后顺序未知故而取固定值
#define	PROCESSQUEUE_PROCESSMGR_CONTROL_RECORD_LENGTH	4 * 1024			// 每条进程管理器的控制命令大大小
#define PROCESSQUEUE_CONTROL_TO_DRIVER_AREASIZE         4 * 1024 * 1024     // 驱动接收控制命令的队列大小，固定，避免pknodeserver启动时找不到而报错！

class ACE_Mem_Map;

// 队列的控制数据结构，不是控制命令
typedef struct tagPKPROCESSQUEINFO
{
	ACE_UINT32	nVersion;	// 版本号，缺省为0
	ACE_UINT32	nRearPos;	// 队列尾部位置（相对于共享内存起始的字节数），从这里入队。总是指向下一个要入队的位置（不适最后1个元素位置）
	ACE_UINT32	nFrontPos;	// 队列头部位置（相对于共享内存起始的字节数），从这里出队。总是指向第一个元素头部
	ACE_UINT32	nRecordNum;	// 当前的记录个数
	ACE_UINT32	nTotalBytes;// 队列的总长度字节
	char			szQueName[128]; // Queue名称
}PKPROCESSQUEINFO;

// 本类目前仅仅在进程管理器和各个服务进程之间通信使用
// 每个队列有两个本地文件:*.CTRL和*.DATA
class CProcessQueue
{

private:
	string m_strQueFilePath;
	string m_strQueFileName;
	string m_strCtrlBlockFilePath;
	string m_strDataBlockFilePath;

 	ACE_Mem_Map*				m_pShmCtrlBlk;			// 队列的控制数据Map，对应*.CTRL文件
	ACE_Mem_Map*				m_pShmDataArea;			// 队列的控制数据结构Map，对应*.DATA文件

	ACE_Process_Mutex			m_mutexCtrlBlock;		// 队列的控制数据结构的进程间互斥信号量，Process Mutex For Control
	PKPROCESSQUEINFO*			m_pSharedCtrlBlock;		// 内存中指向控制数据结构的指针，对应*.CTRL文件
	char *						m_pSharedDataBuf;				// 内存中指向共享内存的指针

public:
	CProcessQueue(const char *szQueName, const char *szPath = NULL ); // 缺省存放位置：{HOMEPATH}/data
	virtual ~CProcessQueue();

public:
	// 建立Queue. 缺省不创建仅仅获取，此时不需要数据大小
	int Open(bool bAutoCreate = false, int nSharedDataBytes = 0);

	// enqueue a buffer
	int EnQueue(const char* szRecordBuf, unsigned int nLength);
	
	// dequeue，调用者必须先分配好内存才能使用
	int DeQueue(char *szRecordBuf, unsigned int nRecordBufLen, unsigned int *pnRetRecordBufLen);

	// get queue records' number
	int GetRecordNum(unsigned int *pnCount);

	// 获取Queue名称
	int GetQueueName(char *szQueueName, unsigned int nQueNameLen);

	// 获取Queue路径
	int GetQueuePath(char *szQueuePath, unsigned int nQuePathLen);

	// 获取Queue信息
	int GetQueueInfo(PKPROCESSQUEINFO *pQueInfo);
};

#endif//_BDB_QUEUE_H_
