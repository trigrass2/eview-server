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

#define	PROCESSQUEUE_PROCESSMGR_CONTROL_AREASIZE		40 * 1024			// ���̹������Ŀ���������ڴ��С,�̶��ģ���Ϊ�Ⱥ�˳��δ֪�ʶ�ȡ�̶�ֵ
#define	PROCESSQUEUE_PROCESSMGR_CONTROL_RECORD_LENGTH	4 * 1024			// ÿ�����̹������Ŀ���������С
#define PROCESSQUEUE_CONTROL_TO_DRIVER_AREASIZE         4 * 1024 * 1024     // �������տ�������Ķ��д�С���̶�������pknodeserver����ʱ�Ҳ���������

class ACE_Mem_Map;

// ���еĿ������ݽṹ�����ǿ�������
typedef struct tagPKPROCESSQUEINFO
{
	ACE_UINT32	nVersion;	// �汾�ţ�ȱʡΪ0
	ACE_UINT32	nRearPos;	// ����β��λ�ã�����ڹ����ڴ���ʼ���ֽ���������������ӡ�����ָ����һ��Ҫ��ӵ�λ�ã��������1��Ԫ��λ�ã�
	ACE_UINT32	nFrontPos;	// ����ͷ��λ�ã�����ڹ����ڴ���ʼ���ֽ���������������ӡ�����ָ���һ��Ԫ��ͷ��
	ACE_UINT32	nRecordNum;	// ��ǰ�ļ�¼����
	ACE_UINT32	nTotalBytes;// ���е��ܳ����ֽ�
	char			szQueName[128]; // Queue����
}PKPROCESSQUEINFO;

// ����Ŀǰ�����ڽ��̹������͸����������֮��ͨ��ʹ��
// ÿ�����������������ļ�:*.CTRL��*.DATA
class CProcessQueue
{

private:
	string m_strQueFilePath;
	string m_strQueFileName;
	string m_strCtrlBlockFilePath;
	string m_strDataBlockFilePath;

 	ACE_Mem_Map*				m_pShmCtrlBlk;			// ���еĿ�������Map����Ӧ*.CTRL�ļ�
	ACE_Mem_Map*				m_pShmDataArea;			// ���еĿ������ݽṹMap����Ӧ*.DATA�ļ�

	ACE_Process_Mutex			m_mutexCtrlBlock;		// ���еĿ������ݽṹ�Ľ��̼以���ź�����Process Mutex For Control
	PKPROCESSQUEINFO*			m_pSharedCtrlBlock;		// �ڴ���ָ��������ݽṹ��ָ�룬��Ӧ*.CTRL�ļ�
	char *						m_pSharedDataBuf;				// �ڴ���ָ�����ڴ��ָ��

public:
	CProcessQueue(const char *szQueName, const char *szPath = NULL ); // ȱʡ���λ�ã�{HOMEPATH}/data
	virtual ~CProcessQueue();

public:
	// ����Queue. ȱʡ������������ȡ����ʱ����Ҫ���ݴ�С
	int Open(bool bAutoCreate = false, int nSharedDataBytes = 0);

	// enqueue a buffer
	int EnQueue(const char* szRecordBuf, unsigned int nLength);
	
	// dequeue�������߱����ȷ�����ڴ����ʹ��
	int DeQueue(char *szRecordBuf, unsigned int nRecordBufLen, unsigned int *pnRetRecordBufLen);

	// get queue records' number
	int GetRecordNum(unsigned int *pnCount);

	// ��ȡQueue����
	int GetQueueName(char *szQueueName, unsigned int nQueNameLen);

	// ��ȡQueue·��
	int GetQueuePath(char *szQueuePath, unsigned int nQuePathLen);

	// ��ȡQueue��Ϣ
	int GetQueueInfo(PKPROCESSQUEINFO *pQueInfo);
};

#endif//_BDB_QUEUE_H_
