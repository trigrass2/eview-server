/**************************************************************
 *  Filename:    RMService.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: RM Service Impl..
 *
 *  @author:     chenzhiquan
 *  @version     05/20/2008  chenzhiquan  Initial Version
**************************************************************/
#include "DBToEviewServer.h"
#include <ace/Get_Opt.h>
#include <iostream>
#include "pkserver/PKServerBase.h"
#include "pklog/pklog.h"
#include "pkcomm/pkcomm.h"
#include <ace/OS_NS_sys_stat.h>
#include "pkcomm/pkcomm.h"
#include "MainTask.h"
#include "pkcrashdump/pkcrashdump.h"

CPKLog g_logger;
CDBToEviewServer *g_pDBToEviewServer = NULL;

/**
 *  Constructor.
 *
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 */
CDBToEviewServer::CDBToEviewServer() : CPKServerBase(false)
{
}

CDBToEviewServer::~CDBToEviewServer()
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

int CDBToEviewServer::OnStart( int argc, ACE_TCHAR* args[] )
{
	printf(("========== DataServer Init =========\n"));

	//获取运行时数据目录;
	string strRunTimePath = PKComm::GetRunTimeDataPath();
	// 创建接受驱动数据的共享队列;
	string strPbSrvPath = strRunTimePath + ACE_DIRECTORY_SEPARATOR_STR + "pkdbtoeviewserver";
	PKFileHelper::CreateDir(strPbSrvPath.c_str());

	int nErr = MAIN_TASK->Start();
	//LOG_PROCESS->Start();
	return nErr;
}

int CDBToEviewServer::OnStop()
{
	MAIN_TASK->Stop();
	return 0;
}

int main(int argc, char* args[])
{
	g_logger.SetLogFileName(PKComm::GetProcessName());
	g_pDBToEviewServer = new CDBToEviewServer();
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "===============PKDBToEviewServer::Main(), argc(%d) start...===============", argc);
	int nRet = g_pDBToEviewServer->Main(argc, args);
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "PKDBToEviewServer::Main Return(%d)", nRet);
	if (NULL != g_pDBToEviewServer)
	{
		delete g_pDBToEviewServer;
		g_pDBToEviewServer = NULL;
	}
	return nRet;
}
