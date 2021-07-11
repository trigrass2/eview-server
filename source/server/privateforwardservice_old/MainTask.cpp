/**************************************************************
 *  Filename:    RecvThread.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 接收消息线程.
 *
 *  @author:     liuqifeng
 *  @version     04/13/2009  liuqifeng  Initial Version
**************************************************************/
// RecvThread.cpp: implementation of the CRecvThread class.
//
//////////////////////////////////////////////////////////////////////

#include <ace/INET_Addr.h>
#include <ace/Log_Msg.h>
#include "ClientSockHandler.h"
#include "ace/Select_Reactor.h"
#include "ace/OS_NS_strings.h" 
#include "MainTask.h"
#include "SystemConfig.h"
#include "common/pklog.h"
#include "SystemConfig.h"
#include "ace/SOCK_Connector.h"

extern CPKLog	PKLog;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
ACE_DLL g_dllforwardaccessor;
PFN_Initialize		g_pfnInitialize = NULL;
PFN_ReadAllTagsData	g_pfnReadAllTagsData = NULL;
PFN_UnInitialize	g_pfnUnInitialize = NULL;
#define DLLNAME_FORWARDACCESSOR			"forwardaccessor"


CMainTask::CMainTask():m_cycleReader(this)
{ 
	ACE_Select_Reactor *pSelectReactor = new ACE_Select_Reactor();
	m_pReactor = new ACE_Reactor(pSelectReactor, 1);
	reactor(m_pReactor);
}

CMainTask::~CMainTask()
{
	if (m_pReactor != NULL)
	{
		delete m_pReactor;
		m_pReactor = NULL;
	}
}

//线程服务
int CMainTask::svc()
{
	long nErr = g_dllforwardaccessor.open(DLLNAME_FORWARDACCESSOR);
	if (nErr == 0)
	{
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, " load Module %s 成功", DLLNAME_FORWARDACCESSOR);
	}
	else
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, " load Module %s 失败", DLLNAME_FORWARDACCESSOR);

	// 导入初始化函数
	g_pfnInitialize = (PFN_Initialize)g_dllforwardaccessor.symbol("Initialize"); 
	if(g_pfnInitialize == NULL)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, " 从%s 获取导出函数%s失败",g_dllforwardaccessor.dll_name_, "Initialize");
	}
	g_pfnReadAllTagsData = (PFN_ReadAllTagsData)g_dllforwardaccessor.symbol("ReadAllTagsData"); 
	if(g_pfnReadAllTagsData == NULL)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, " 从%s 获取导出函数%s失败",g_dllforwardaccessor.dll_name_, "ReadAllTagsData");
	}
	g_pfnUnInitialize = (PFN_UnInitialize)g_dllforwardaccessor.symbol("UnInitialize"); 
	if(g_pfnUnInitialize == NULL)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, " 从%s 获取导出函数%s失败",g_dllforwardaccessor.dll_name_, "UnInitialize");
	}

	this->reactor()->owner(ACE_OS::thr_self ());
	m_cycleReader.m_nPollRate = SYSTEM_CONFIG->m_nCycleMS;
	m_cycleReader.StartTimer();

	this->reactor()->reset_reactor_event_loop();
	this->reactor()->run_reactor_event_loop();

	m_cycleReader.StopTimer();
	return 0;
}

void CMainTask::Stop()
{ 
	// 发出关闭响应起得请求
	this->reactor()->end_reactor_event_loop();
	
	// 等待响应器关闭后退出
	this->wait();
}


int CMainTask::Start()
{
	return this->activate();
}

