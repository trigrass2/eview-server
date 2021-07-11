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

#ifndef _MAIN_TASK_H_
#define _MAIN_TASK_H_

#include <ace/Task.h>
#include <string>
#include <map>
#include <vector>
#include "json/json.h"
#include "pkdata/pkdata.h"
#include "pkinifile/pkinifile.h"
#include "pkdbapi/pkdbapi.h"

using namespace  std;

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

protected:

	int				m_nPeriod;	//轮询周期，缺省为10秒
	string			m_strEviewServerIp;
	vector<std::pair<string, string> >	m_vecTag2StringPair;
	PKHANDLE		m_hPkData;

	vector<string>	m_vecSQLToExecute; // 插入或更新报警到历史报警表

	ACE_Time_Value	m_tvLastQueryDB;	// 上次轮询数据库的时间
	int LoadConfig();
	int QueryDBToTags();
	int SetTagValueToOK(string strTagName, string strTagValue);
	int SetTagValueToBad(string strTagValue, string strBadValue);
public:
	virtual int svc();

	// 线程初始化
	int OnStart();
	// 线程中停止
	int OnStop();

};

#define MAIN_TASK ACE_Singleton<CMainTask, ACE_Thread_Mutex>::instance()

#endif  // _MAIN_TASK_H_
