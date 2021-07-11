/**************************************************************
 *  Filename:    ActionTask.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 加载服务端动作动态库运行的线程类.
 *
 *  @author:     zhaobin
 *  @version     01/06/2009  zhaobin  Initial Version
**************************************************************/
#ifndef _ACTIONTASK_INCLUDE_HEADER_
#define _ACTIONTASK_INCLUDE_HEADER_

#include "ace/Task.h"
#include "pkcomm/pkcomm.h"
#include "common/eviewdefine.h"

typedef long (*PFN_InitAction)(); //  初始化Action的函数
typedef long (*PFN_ExitAction)(); // 退出时调用的函数
// 动作执行调用函数。
// 注意, 返回值不是动作的真正返回值，动作的返回值必须用回调函数返回, 这是因为有些动作如定时是异步返回结果的, 统一期间用回调函数返回结果
typedef long (*PFN_ExecuteAction)(char *szActionId, char *szTagNameList, char *szTagValue, char *szHoldSeconds, char *szActionParam4, char *szPriority, char* szJsonEventParams, char *szResult, int nResultBufLen);


class CActionTask : public ACE_Task<ACE_MT_SYNCH>   
{
public:
	CActionTask();
	virtual ~CActionTask();

	//设定线程对应的触发源类型ID, 触发源判断的动态库名称
	void	SetActionInfo(const char *szActionName, const char* szDllName);
	int		Start();
	void	Stop();
    int		PostAction(char *szActionId, char *szActionParam, char *szActionParam2, char *szActionParam3, char *szActionParam4, char *szPriority, char* szJsonEventParams);

protected:
	bool m_bStop;
	bool m_bStopped;
	// 线程初始化
	int OnStart();
	// 线程中停止
	void OnStop();
	//线程入口函数
	int		svc();

public:
	//动态库名称
	char	m_szActionTypeName[PK_LONGFILENAME_MAXLEN];
	char	m_szActionDllName[PK_LONGFILENAME_MAXLEN];
	//ACE的动态库，用来加载触发源判定动态库
	ACE_DLL				 m_dllAction;
	PFN_InitAction		m_pfnInitAction;
	PFN_ExitAction		m_pfnExitAction;
	PFN_ExecuteAction	m_pfnExecuteAction;
};


#endif // ifndef _ACTIONTASK_INCLUDE_HEADER_
