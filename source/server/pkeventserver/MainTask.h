/**************************************************************
*  Filename:    CtrlProcessTask.h
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: 控制命令处理类.
*
*  @author:     lijingjing
*  @version     05/28/2008  lijingjing  Initial Version
*  @version     01/28/2012  wanyingjie  Initial Version
**************************************************************/

#ifndef _ALARM_ROUTE_TASK_H_
#define _ALARM_ROUTE_TASK_H_

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
class CMainTask : public ACE_Task<ACE_MT_SYNCH>
{
	friend class ACE_Singleton<CMainTask, ACE_Thread_Mutex>;
public:
	CMainTask();
	virtual ~CMainTask();

	int Start();
	void Stop();
protected:
	bool			m_bStop;
	CMemDBSubscriberSync 	m_memDBSubSync;  // 订阅

	time_t m_tmLastLogEvt; // 上次打印事件日志的事件
	long m_nLastLogEvtCount;	// 已经记录的日志条数
	long m_nTotalEvtCount;
	long m_nTotalEvtFailCount;

	time_t m_tmLastExecute;	// 上次执行的时间
	vector<string> m_vecHistoryEvent; // 插入一个历史事件
	vector<string> m_vecAlarmSQL; // 插入或更新报警到历史报警表
	vector<SQLStatus> m_vecFailSQL;	// 执行失败的SQL

	int WriteEvents2DB();
public:
	virtual int svc();

	// 线程初始化
	int OnStart();
	// 线程中停止
	int OnStop();
	// 将控制中继给驱动, [{"name":tag1,"value":"100"},{"name":tag2,"value":"100"}]
	int ParseEventsMessage(char *szEventsInfo, Json::Value &root);

	int GetModuleVersion(char *szModuleName, int *pnVersion);
	int UpdateModuleVersion(char *szModuleName, int nNewVersion);
	int AutoUpdateVersion();
};

// 保存tag点的所有相关信息,供记录日志是使用
typedef struct _CTagInfo
{
	int nTagID;  // tag点id
	char szTagName[PK_NAME_MAXLEN];  // tag点名字
	char szAddress[PK_NAME_MAXLEN];  // tag点名字
	char szValue[PK_NAME_MAXLEN];  // tag点值
	char szTagCname[PK_DESC_MAXLEN];  // tag点中文名
	int nSubsysID;   // 子系统id
	char szSubsysname[PK_DESC_MAXLEN];    // 子系统名
	map<string, string> mapVal2Lable;	// 将tag点的值映射到有意义的名字，如0--关闭，1---打开
	_CTagInfo()
	{
		nTagID = -1;
		memset(szTagCname, 0x00, sizeof(szTagName));
		memset(szAddress, 0x00, sizeof(szAddress));
		memset(szValue, 0x00, sizeof(szValue));
		memset(szTagCname, 0x00, sizeof(szTagCname));
		memset(szSubsysname, 0x00, sizeof(szSubsysname));
		mapVal2Lable.clear();
	}
}CTagInfo;

typedef struct _CAlarmAndTag
{
	int nTagID;
	int nAlarmTagID;
	int nAlarmTypeID;
	int nSubSysID;
	char szSubSysName[PK_DESC_MAXLEN];
	bool bIsAlarmArea;
	char szCname[PK_DESC_MAXLEN];
	char szAlarmTypeName[PK_DESC_MAXLEN];
	char szAlarmTypeCode[PK_NAME_MAXLEN];

	_CAlarmAndTag()
	{
		memset(this, 0, sizeof(_CAlarmAndTag));
	}
}CAlarmAndTag;

typedef struct _CAlarmAndAlarmarea
{
	int nTagID;
	char szCname[PK_DESC_MAXLEN];

	_CAlarmAndAlarmarea()
	{
		memset(this, 0x00, sizeof(_CAlarmAndAlarmarea));
	}

}ALARMANDALARMAREA;

typedef struct _CTriggerAndAlarm
{
	int nTriggerID;
	int nSrcID;
	char szSrcState[PK_NAME_MAXLEN];
	int nPriority;

	_CTriggerAndAlarm()
	{
		memset(this, 0, sizeof(_CTriggerAndAlarm));
	}
}CTriggerAndAlarm;

typedef struct _CTriggerAndAction
{
	int nActionID;
	int nTriggerID;
	char szActionParam[PK_TAGDATA_MAXLEN];

	_CTriggerAndAction()
	{
		memset(this, 0, sizeof(_CTriggerAndAction));
	}
}CTriggerAndAction;

typedef struct _CActionType
{
	int nActionTypeID;
	char szActionTypeName[PK_NAME_MAXLEN];
	char szActionTypeDesc[PK_NAME_MAXLEN];

	_CActionType()
	{
		memset(this, 0, sizeof(_CActionType));
	}
}CActionType;

typedef struct _CSubSysInfo
{
	int nSubSysID;
	char szSubSysName[PK_NAME_MAXLEN];
	_CSubSysInfo()
	{
		memset(this, 0, sizeof(_CSubSysInfo));
	}
}CSubSysInfo;

typedef struct _CLogTypeInfo
{
	int nTypeID;
	char szTypeName[PK_NAME_MAXLEN];
	_CLogTypeInfo()
	{
		memset(this, 0, sizeof(_CLogTypeInfo));
	}
}CLogTypeInfo;
#define MAIN_TASK ACE_Singleton<CMainTask, ACE_Thread_Mutex>::instance()

#endif  // _ALARM_ROUTE_TASK_H_
