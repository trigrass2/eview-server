/**************************************************************
*  Filename:    MainTask.h
*  Copyright:   Shanghai peakinfo Software Co., Ltd.
*
*  Description: mqtt read redis and publish to topic.
*
*  @author:     yanchaodong
*  @version     04/16/2018  yanchaodong  Initial Version
**************************************************************/

#ifndef _EVIEWFORWARD_MAINTASK_H_
#define _EVIEWFORWARD_MAINTASK_H_

#include <ace/Task.h>
#include <string>
#include <map>
#include <vector>
#include "json/json.h"
#include "pkcomm/pkcomm.h"
#include "pkdbapi/pkdbapi.h"
#include "pkdata/pkdata.h"
#include "DbTransferDefine.h"

class CMainTask : public ACE_Task<ACE_MT_SYNCH>
{
	friend class ACE_Singleton<CMainTask, ACE_Thread_Mutex>;
public:
	CMainTask();
	virtual ~CMainTask();
	CPKDbApi	m_dbEview;

	int Start();
	void Stop();
protected:
    bool		m_bStop;

public:
	map<string, CDBConnection *>	m_mapId2DbConn;		// 所有的数据库连接
	map<string, CDBTransferRule *>	m_mapId2TransferRule;	// 所有的转储规则

public:
	virtual int svc();

	// 线程初始化
	int OnStart();
	// 线程中停止
	int OnStop();


	int		LoadConfig();
	int		ReadAllTags(); 
	int		ReadDBConnList();
	int ReadTransferRuleList();
	int ReadTagList();
	int AssignRuleToTimerObject();

public:
	vector<string>		m_vecTagsName;
	string	m_strValueOnBadQuality_Int;	// 质量为BAD时的值。缺省为???
	string	m_strValueOnBadQuality_Real;	// 质量为BAD时的值。缺省为???
	string	m_strValueOnBadQuality_String;	// 质量为BAD时的值。缺省为???
	string	m_strValueOnBadQuality_DateTime;	// 质量为BAD时的值。缺省为???
};

// 保存tag点的所有相关信息,供记录日志是使用

#define MAIN_TASK ACE_Singleton<CMainTask, ACE_Thread_Mutex>::instance()

#endif  // _ALARM_ROUTE_TASK_H_
