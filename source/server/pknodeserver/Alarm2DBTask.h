/**************************************************************
*  Filename:    AlarmToDB.h
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: 控制命令处理类.
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
	CMemDBSubscriberSync 	m_memDBSubSync;  // 订阅

	time_t m_tmLastLogEvt; // 上次打印事件日志的事件
	long m_nLastLogEvtCount;	// 已经记录的日志条数
	long m_nTotalEvtCount;
	long m_nTotalEvtFailCount;

	time_t				m_tmLastExecute;	// 上次执行的时间
	vector<string>		m_vecHistoryEvent; // 插入一个历史事件
	vector<string>		m_vecAlarmSQL; // 插入或更新报警到历史报警表
	vector<SQLStatus>	m_vecFailSQL;	// 执行失败的SQL
	int					m_nWaitCommitCount;
	time_t				m_tmLastCommit;

	int WriteEvents2DB();
public:
	virtual int svc();

	// 线程初始化
	int OnStart();
	// 线程中停止
	int OnStop();
	// 将控制中继给驱动, [{"name":tag1,"value":"100"},{"name":tag2,"value":"100"}]
	int ParseEventsMessage(char *szEventsInfo, Json::Value &root);
	int ParseEventMessages(vector<string> &vecMessage);
};

#define ALARM2DB_TASK ACE_Singleton<CAlarm2DBTask, ACE_Thread_Mutex>::instance()

#endif  // _ALARM_TODB_TASK_H_
