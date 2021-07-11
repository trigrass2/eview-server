/**************************************************************
 *  Filename:    CtrlProcessTask.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: �����������.
 *
 *  @author:     lijingjing
 *  @version     05/28/2008  lijingjing  Initial Version
 *  @version     01/28/2012  wanyingjie  Initial Version
**************************************************************/

#ifndef _RAWDATA_PROC_TASK_H_
#define _RAWDATA_PROC_TASK_H_

#include <ace/Task.h>
#include "pkmemdbapi/pkmemdbapi.h"
#include "shmqueue/ProcessQueue.h" 
#include "json/json.h"
#include "json/writer.h"
#include "NodeCommonDef.h"
#include <map>
using namespace  std;

typedef struct _TAGDATA_JSONSTR{
	string strName;	// TAG������
	string strVTQ;	// TAG�����ݡ�����
	string strTime4Debug; // TAG���ʱ���
}TAGDATA_JSONSTR;

class CDataProcTask : public ACE_Task<ACE_MT_SYNCH>
{
	friend class ACE_Singleton<CDataProcTask, ACE_Thread_Mutex>;
public:
	CDataProcTask();
	virtual ~CDataProcTask();

	int Start();
	void Stop();

public: 
	Json::FastWriter            m_jsonWriter;

protected:
	bool			m_bStop;
	bool			m_bStopped;

	unsigned int	m_nPackNumRecvRecent;
	unsigned int	m_nPackNumRecvFromStart;
	ACE_Time_Value  m_tvRecentRecvPack;		// �ϴδ�ӡ
	ACE_Time_Value  m_tvFirstRecvPack;		// ��һ�ν��յ����ݰ�
	unsigned int	m_nTimeoutPackNumFromStart; // �ӳ�������������ʱ�İ��ĸ���
	unsigned int	m_nTimeoutMaxSecFromStart;	// �ӳ�������������ʱ�İ������ʱʱ��
	unsigned int	m_nTimeoutPackNumRecent;	// �����ʱ�İ��ĸ���
	unsigned int	m_nTimeoutMaxSecRecent;		// �����ʱ�İ������ʱʱ��

	unsigned int	m_nRecvTagCountRecent;	// һ��ʱ�����յ�����������������tag��
	unsigned int	m_nValidTagCountRecent;	// һ��ʱ�����յ�����������������Чtag��
	
	string			m_strMinTagTimestampRecent;	// ���һ��ʱ�����Сʱ���
	string			m_strMaxTagTimestampRecent; // ���һ��ʱ������ʱ���
	unsigned int	m_nCommitTagNumRecent;		// ���һ��ʱ���ύ��tag����
	unsigned int	m_nCommitTimesRecent;		// ���һ��ʱ���ύ�Ĵ���
	string			m_strCommitDetail;			// ÿ���ύ������
	ACE_Time_Value	m_tvLastCommitTagData;		// �ϴ��ύtag�㵽�ڴ����ݿ��ʱ��
	ACE_Time_Value	m_tvLastCaclTags;			// �ϴμ��������ʱ��

	ACE_Time_Value	m_tvLastCheckDriverAlive;	// �ϴμ�������Ƿ������е�ʱ��	
	void CheckNodeAlive(ACE_Time_Value tvNow);
 	// ������������tag������ΪBAD�������ر��ˣ�
	int UpdateTagsOfNodeToBad(CNodeInfo *pNode);


	ACE_Time_Value m_tvLastPrint;
	int CheckAndPrintLowerNodeStatus(ACE_Time_Value tvNow);
public:
	vector<string> split(string str,string pattern);

	ACE_Thread_Mutex		m_mutexTagDataCache;
	list<TAGDATA_JSONSTR>	m_vecTagDataCached;
 
protected:
	virtual int svc();

	// �̳߳�ʼ��
	int OnStart();
	// �߳���ֹͣ
	void OnStop();
 	void PrintDataPackStatInfo(ACE_Time_Value &tvNow, ACE_Time_Value &tvPack);
 	void CheckAndCommitTagData(ACE_Time_Value tvNow );
 
};

#define DATAPROC_TASK ACE_Singleton<CDataProcTask, ACE_Thread_Mutex>::instance()

#endif  // _RAWDATA_PROC_TASK_H_
