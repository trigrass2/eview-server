/**************************************************************
 *  Filename:    CtrlProcessTask.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 动态加载用户权限信息的类，避免每次查询数据库的低效率.
 *
 *  @author:     lijingjing
 *  @version     05/28/2008  lijingjing  Initial Version
 *  @version     01/28/2012  wanyingjie  Initial Version
**************************************************************/

#ifndef _USER_AUTH_TASK_H_
#define _USER_AUTH_TASK_H_

#include <ace/Task.h>
#include <string>
#include <map>

using namespace  std;

class CDataTag;
class CUserAuthTask : public ACE_Task<ACE_MT_SYNCH>
{
	friend class ACE_Singleton<CUserAuthTask, ACE_Thread_Mutex>;
public:
	CUserAuthTask();
	virtual ~CUserAuthTask();

	int Start();
	void Stop();

protected:
	bool			m_bStop;
	map<string, string> m_mapLoginName2RoleName;			// 用户登录名--》群组名
    map<string, map<string,bool> >	m_mapRoleName2SysNameMap;	// 群组名+子系统名,true.没有权限则查不到该项，返回false
	ACE_Thread_Mutex	m_mutex;// 保护上面两个map的mutex

public:
	virtual int svc();

	// 线程初始化
	int OnStart();
	// 线程中停止
	void OnStop();

	int LoadAuthFromDB();
	bool HasAuthByTagAndLoginName(string &strLoginName, CDataTag *pTag);
};

#define USERAUTH_TASK ACE_Singleton<CUserAuthTask, ACE_Thread_Mutex>::instance()

#endif  // _CTRL_PROC_TASK_H_
