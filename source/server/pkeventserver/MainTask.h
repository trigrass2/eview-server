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
	CMemDBSubscriberSync 	m_memDBSubSync;  // ����

	time_t m_tmLastLogEvt; // �ϴδ�ӡ�¼���־���¼�
	long m_nLastLogEvtCount;	// �Ѿ���¼����־����
	long m_nTotalEvtCount;
	long m_nTotalEvtFailCount;

	time_t m_tmLastExecute;	// �ϴ�ִ�е�ʱ��
	vector<string> m_vecHistoryEvent; // ����һ����ʷ�¼�
	vector<string> m_vecAlarmSQL; // �������±�������ʷ������
	vector<SQLStatus> m_vecFailSQL;	// ִ��ʧ�ܵ�SQL

	int WriteEvents2DB();
public:
	virtual int svc();

	// �̳߳�ʼ��
	int OnStart();
	// �߳���ֹͣ
	int OnStop();
	// �������м̸�����, [{"name":tag1,"value":"100"},{"name":tag2,"value":"100"}]
	int ParseEventsMessage(char *szEventsInfo, Json::Value &root);

	int GetModuleVersion(char *szModuleName, int *pnVersion);
	int UpdateModuleVersion(char *szModuleName, int nNewVersion);
	int AutoUpdateVersion();
};

// ����tag������������Ϣ,����¼��־��ʹ��
typedef struct _CTagInfo
{
	int nTagID;  // tag��id
	char szTagName[PK_NAME_MAXLEN];  // tag������
	char szAddress[PK_NAME_MAXLEN];  // tag������
	char szValue[PK_NAME_MAXLEN];  // tag��ֵ
	char szTagCname[PK_DESC_MAXLEN];  // tag��������
	int nSubsysID;   // ��ϵͳid
	char szSubsysname[PK_DESC_MAXLEN];    // ��ϵͳ��
	map<string, string> mapVal2Lable;	// ��tag���ֵӳ�䵽����������֣���0--�رգ�1---��
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
