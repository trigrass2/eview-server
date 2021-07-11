/**************************************************************
 *  Filename:    TaskGroup.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: �豸����Ϣʵ����.
 *
 *  @author:     lijingjing
 *  @version     05/28/2008  lijingjing  Initial Version
**************************************************************/
#ifndef _TIMER_TASK_H_
#define _TIMER_TASK_H_

#include "ace/Reactor.h"
#include "ace/Select_Reactor.h"
#include "ace/Acceptor.h"
#include <ace/SOCK_Acceptor.h>
#include <map>
#include <utility>
#include <vector>
using namespace std;
#include "pkdata/pkdata.h"
#include "RuleTimer.h"
#include "HourTimer.h"

class CMainTask;
class CTimer;
class CTimerTask : public ACE_Task<ACE_MT_SYNCH>
{
	friend class ACE_Singleton<CTimerTask, ACE_Thread_Mutex>;
public:
	ACE_Reactor*	m_pReactor;

	virtual int svc ();
	bool m_bExit;

public:
	CTimerTask();
	virtual ~CTimerTask();

	// �����߳�
	int Start();
	// ֹͣ�߳�
	void Stop();

	// Group�߳�����ʱ
	void OnStart();

	// Group�߳�ֹͣʱ
	void OnStop();

public:
	vector<CRuleTimer *>	m_vecRuleTimer;
	vector<CHourTimer *>	m_vecHourTimer;
	PKHANDLE m_hPkData;

protected:
	virtual int handle_timeout(const ACE_Time_Value &current_time, const void *act);	// collect data
	virtual int handle_output (ACE_HANDLE fd = ACE_INVALID_HANDLE);

	int CheckAndConnectDBs(); // ���������ݿ�����
	int AutoCreateTable(CDBConnection *pDBCon, string strTableName, string strCreateSQL);
	int AutoCreateStatTables();
};

#define TRANSFER_TIMER_TASK ACE_Singleton<CTimerTask, ACE_Thread_Mutex>::instance()
#endif  // _DEVICE_GROUP_H_
