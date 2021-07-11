/**************************************************************
 *  Filename:    RMService.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: RM Service Impl..
 *
 *  @author:     chenzhiquan
 *  @version     05/20/2008  chenzhiquan  Initial Version
**************************************************************/
#include "PKEventServer.h"
#include <ace/Get_Opt.h>
#include <iostream>
#include "pkserver/PKServerBase.h"
#include "pklog/pklog.h"
#include "pkcomm/pkcomm.h"
#include <ace/OS_NS_sys_stat.h>
#include "pkcomm/pkcomm.h"
#include "MainTask.h"

CPKLog PKLog;
CEventServer *g_pEventServer = NULL;

/**
 *  Constructor.
 *
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 */
CEventServer::CEventServer() : CPKServerBase("pkeventserver", true)
{
}

CEventServer::~CEventServer()
{
}

/**
 *  Initialize Service,Access of Program;
 *
 *  @param  -[in,out]  int  argc: [comment]
 *  @param  -[in,out]  ACE_TCHAR*  args[]: [comment]
 *
 *  @version     05/19/2008  chenzhiquan  Initial Version.;
 */

int CEventServer::OnStart( int argc, ACE_TCHAR* args[] )
{
	printf(("========== DataServer Init =========\n"));

	//获取运行时数据目录;
	string strRunTimePath = PKComm::GetRunTimeDataPath();
	// 创建接受驱动数据的共享队列;
	string strPbSrvPath = strRunTimePath + ACE_DIRECTORY_SEPARATOR_STR + MODULENAME_EVENTSERVER;
	PKFileHelper::CreateDir(strPbSrvPath.c_str());

	// 初始记为正常启动;
	UpdateServerStatus(0, "started");
	UpdateServerStartTime();

	int nErr = MAIN_TASK->Start();
	//LOG_PROCESS->Start();
	return nErr;
}

int CEventServer::OnStop()
{
	MAIN_TASK->Stop();
	return 0;
}

// name :procename:status,procname:starttime;
int CEventServer::UpdateServerStatus( int nStatus, char *szErrTip )
{
	return PK_SUCCESS;
}

// name :procename:status,procname:starttime;
int CEventServer::UpdateServerStartTime()
{
	return PK_SUCCESS;
}

int main(int argc, char* args[])
{
	PKLog.SetLogFileName(PKComm::GetProcessName());
	g_pEventServer = new CEventServer();
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "===============PKEventServer::Main(), argc(%d) start...===============", argc);
	int nRet = g_pEventServer->Main(argc, args);
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "PKEventServer::Main Return(%d)", nRet);
	if (NULL != g_pEventServer)
	{
		delete g_pEventServer;
		g_pEventServer = NULL;
	}
	return nRet;
}
