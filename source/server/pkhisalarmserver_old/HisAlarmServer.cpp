/**************************************************************
 *  Filename:    RMService.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: RM Service Impl..
 *
 *  @author:     chenzhiquan
 *  @version     05/20/2008  chenzhiquan  Initial Version
**************************************************************/
#include "HisAlarmServer.h"
#include <ace/Get_Opt.h>
#include <iostream>
#include "pkserver/ServerBase.h"

#include "pklog/pklog.h"
#include "common/pkcomm.h"
#include <ace/OS_NS_sys_stat.h>
#include "common/pkGlobalHelper.h"
#include "MainTask.h"


CServerBase* g_pServerBase = new CHisAlarmServer();
CHisAlarmServer *g_pServer = (CHisAlarmServer *)g_pServerBase;
/**
 *  Constructor.
 *
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 */
CHisAlarmServer::CHisAlarmServer() : CServerBase("HisAlarmServer", true)
{
}

CHisAlarmServer::~CHisAlarmServer()
{
}

/**
 *  Initialize Service.
 *
 *  @param  -[in,out]  int  argc: [comment]
 *  @param  -[in,out]  ACE_TCHAR*  args[]: [comment]
 *
 *  @version     05/19/2008  chenzhiquan  Initial Version.
 */
int CHisAlarmServer::OnStart( int argc, ACE_TCHAR* args[] )
{
	printf(_("========== HisAlarmServer Init =========\n"));

	string strRunTimePath = PKComm.GetRunTimeDataPath();
	// 创建接受驱动数据的共享队列
	string strPbSrvPath = strRunTimePath + ACE_DIRECTORY_SEPARATOR_STR + MODULENAME_HISALARMSERVER;
	PKFileHelper::CreateDir(strPbSrvPath.c_str());
	// 初始记为正常启动
	UpdateServerStatus(0, "started");
	UpdateServerStartTime();

	int nErr = MAIN_TASK->Start();
	return nErr;
}

int CHisAlarmServer::OnStop()
{
	MAIN_TASK->Stop();
	return 0;
}

// name :procename:status,procname:starttime
int CHisAlarmServer::UpdateServerStatus( int nStatus, char *szErrTip )
{
	return PK_SUCCESS;
}

// name :procename:status,procname:starttime
int CHisAlarmServer::UpdateServerStartTime()
{
	return PK_SUCCESS;
}
