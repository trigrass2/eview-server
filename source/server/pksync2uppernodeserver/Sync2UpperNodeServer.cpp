/**************************************************************
 *  Filename:    RMService.cpp
 *  Copyright:   Shanghai peakinfo Software Co., Ltd.
 *
 *  Description: RM Service Impl..
 *
**************************************************************/
#include "Sync2UpperNodeServer.h"
#include <ace/Get_Opt.h>
#include <iostream>
#include "pkserver/PKServerBase.h"
#include "pklog/pklog.h"
#include "pkcomm/pkcomm.h"
#include <ace/OS_NS_sys_stat.h>
#include "pkcrashdump/pkcrashdump.h"

CPKLog g_logger;
CSyncToUpperNodeServer *g_syncToUpperNodedServer = NULL;

/**
 *  Constructor.
 *
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 */
CSyncToUpperNodeServer::CSyncToUpperNodeServer() : CPKServerBase(true)
{
}

CSyncToUpperNodeServer::~CSyncToUpperNodeServer()
{
}

/**
 *  Initialize Service,Access of Program;
 *
 *  @param  -[in,out]  int  argc: [comment]
 *  @param  -[in,out]  ACE_TCHAR*  args[]: [comment]
 *
 *  @version     05/19/2008  chenzhiquan  Initial Version.;
 第二个参数为客户端名！！！
 */

int CSyncToUpperNodeServer::OnStart( int argc, ACE_TCHAR* args[] )
{
	printf(("========== SyncToSuperNodeServer Init =========\n"));

	//获取运行时数据目录;
	string strRunTimePath = PKComm::GetRunTimeDataPath();
    // 创建接受驱动数据的共享队列;

	// 初始记为正常启动;
	//UpdateServerStatus(0, "started");
	//UpdateServerStartTime();

	int nErr = MAIN_TASK->Start();
	return nErr;
}

int CSyncToUpperNodeServer::OnStop()
{
	MAIN_TASK->Stop();
	return 0;
}

//#ifdef YCD
int main(int argc, char* args[])
{
	g_logger.SetLogFileName(PKComm::GetProcessName());
	g_syncToUpperNodedServer = new CSyncToUpperNodeServer();
    g_logger.LogMessage(PK_LOGLEVEL_INFO, "===============SyncToSuperNodeServer::Main(), argc(%d) start...===============", argc);

    int nRet = g_syncToUpperNodedServer->Main(argc, args);
    g_logger.LogMessage(PK_LOGLEVEL_INFO, "SyncToSuperNodeServer::Main exit, Return(%d)", nRet);
    if (NULL != g_syncToUpperNodedServer)
	{
        delete g_syncToUpperNodedServer;
		g_syncToUpperNodedServer = NULL;
	}
	return nRet;
}
//#endif