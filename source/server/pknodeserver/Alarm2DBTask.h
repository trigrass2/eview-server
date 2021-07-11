/**************************************************************
*  Filename:    AlarmToDB.h
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: �����������.
*
*  @author:     shijunpu
*  @version     05/28/2008  lijingjing  Initial Version
*  @version     01/28/2012  wanyingjie  Initial Version
**************************************************************/

#ifndef _ALARM_TODB_TASK_H_
#define _ALARM_TODB_TASK_H_

#include <ace/Task.h>
#include <string>
#include <map>
#include <vector>
#include "pkmemdbapi/pkmemdbapi.h"
#include "json/json.h"
#include "common/eviewdefine.h"
#include "sqlite/CppSQLite3.h"
#include "pkeviewdbapi/pkeviewdbapi.h"

using namespace  std;

class SQLStatus{
public:
	string	strSQL;
	time_t	tmFirstExecute;
	int		nExecuteCount;

public:
	SQLStatus(string strSQLIn)
	{
		strSQL = strSQLIn;
		time(&tmFirstExecute);
		nExecuteCount = 1;
	}
};

// 
class CAlarm2DBTask : public ACE_Task<ACE_MT_SYNCH>
{
	friend class ACE_Singleton<CAlarm2DBTask, ACE_Thread_Mutex>;
public:
	CAlarm2DBTask();
	virtual ~CAlarm2DBTask();

	int Start();
	void Stop();
protected:
	bool					m_bStop;
	CMemDBSubscriberSync 	m_memDBSubSync;  // ����

	time_t m_tmLastLogEvt; // �ϴδ�ӡ�¼���־���¼�
	long m_nLastLogEvtCount;	// �Ѿ���¼����־����
	long m_nTotalEvtCount;
	long m_nTotalEvtFailCount;

	time_t				m_tmLastExecute;	// �ϴ�ִ�е�ʱ��
	vector<string>		m_vecHistoryEvent; // ����һ����ʷ�¼�
	vector<string>		m_vecAlarmSQL; // �������±�������ʷ������
	vector<SQLStatus>	m_vecFailSQL;	// ִ��ʧ�ܵ�SQL
	int					m_nWaitCommitCount;
	time_t				m_tmLastCommit;

	int WriteEvents2DB();
public:
	virtual int svc();

	// �̳߳�ʼ��
	int OnStart();
	// �߳���ֹͣ
	int OnStop();
	// �������м̸�����, [{"name":tag1,"value":"100"},{"name":tag2,"value":"100"}]
	int ParseEventsMessage(char *szEventsInfo, Json::Value &root);
	int ParseEventMessages(vector<string> &vecMessage);
};

#define ALARM2DB_TASK ACE_Singleton<CAlarm2DBTask, ACE_Thread_Mutex>::instance()

#endif  // _ALARM_TODB_TASK_H_
