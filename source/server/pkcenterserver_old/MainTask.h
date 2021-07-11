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

#ifndef _CTRL_PROCESS_TASK_H_
#define _CTRL_PROCESS_TASK_H_

#include <ace/Task.h>
#include "tinyxml/tinyxml.h"
#include "json/json.h"
#include "redisproxy/redisproxy.h"

#include <map>
using namespace  std;

class CTaskGroup;
class CDevice;
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
	bool			m_bStopped;
	CRedisProxy		m_redisSub;		// 订阅初始化数据
	CRedisProxy		m_redisRW;		// 将初始化数据写到内存数据库
protected:
	virtual int svc();

	// 线程初始化
	int OnStart();

	// 线程中停止
	void OnStop();

	int UpdateTagNum(Json::Value jsonCmd);
	int UpdateObject(Json::Value jsonCmd);
	int UpdateTag(Json::Value jsonCmd);
	int ParseAndSetInitData2MenDB(string &strMessage,Json::Value &root);
};

#define MAIN_TASK ACE_Singleton<CMainTask, ACE_Thread_Mutex>::instance()

#endif  // _CTRL_PROCESS_TASK_H_
