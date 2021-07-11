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

#ifndef _CTRL_PROC_TASK_H_
#define _CTRL_PROC_TASK_H_

#include <ace/Task.h>
#include <string>
#include <map>
#include "pkmemdbapi/pkmemdbapi.h"
#include "json/json.h"
#include "json/writer.h"
#include "PKNodeServer.h"
#include "EAProcessor.h"

using namespace  std;

class CProcessQueue;
class CCtrlProcTask : public ACE_Task<ACE_MT_SYNCH>
{
	friend class ACE_Singleton<CCtrlProcTask, ACE_Thread_Mutex>;
public:
	CCtrlProcTask();
	virtual ~CCtrlProcTask();

	int Start();
	void Stop();

	Json::FastWriter            m_jsonWriter;
protected:
	bool			m_bStop;

	CMemDBSubscriberSync		m_redisSub; // 订阅
	CMemDBClientAsync			m_redisPub; // 推送消息
	CEAProcessor				m_eaprocessor;  
public:
	virtual int svc();

	// 线程初始化
	int OnStart();
	// 线程中停止
	void OnStop();
	// 将控制中继给驱动, [{"name":tag1,"value":"100"},{"name":tag2,"value":"100"}]
	int RelayControlCmd2Driver(const char *szCtrlCmd, Json::Value &root);
	int ConfirmAlarm(string &strTagFilter, string &strJudgeMethod, string &strLevelFilter, string strProduceTime, string &strUsername, string &strHowToProcess);
	string GetControlLabel(string strTagName, int nControlVal);
	int LogControl2Event(const char *szUserName, CDataTag *pDataTag, const char *szTagVal, const char *szTime);
	int OnCtrlOfPredefinedVar(string &strTagName, string &strTagVal, string &strUserName, char *szTime);
	int SuppressAlarm(string strTagName, string strJudgeMethod, string strProuceTime, string  strSuppressTime, string strUserName);
};

#define CONTROLPROCESS_TASK ACE_Singleton<CCtrlProcTask, ACE_Thread_Mutex>::instance()

#endif  // _CTRL_PROC_TASK_H_
