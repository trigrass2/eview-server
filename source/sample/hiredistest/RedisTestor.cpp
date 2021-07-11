/**************************************************************
 *  Filename:    RMService.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: RM Service Impl..
 *
 *  @author:     chenzhiquan
 *  @version     05/20/2008  chenzhiquan  Initial Version
**************************************************************/
#include "RedisTestor.h"
#include <ace/Get_Opt.h>
#include <iostream>

#include "pklog/pklog.h"
#include "common/pkcomm.h"
#include <ace/OS_NS_sys_stat.h>
#include "common/pkGlobalHelper.h"
#include "redisproxy/redisproxy.h"
#include "Task1.h"
#include "Task2.h"

CRedisProxy		g_redisSub; // 订阅
CRedisProxy		g_redisRW; // 订阅

#define _(STRING) STRING

CServerBase* g_pServer = new CRedisTestor();

/**
 *  Constructor.
 *
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 */
CRedisTestor::CRedisTestor() : CServerBase("RedisTestor", true)
{
}

CRedisTestor::~CRedisTestor()
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
int CRedisTestor::OnStart( int argc, ACE_TCHAR* args[] )
{
	printf(_("========== PbServer Init =========\n"));

	// 初始记为正常启动
	int nRet = PK_SUCCESS;
	nRet = g_redisSub.Initialize(PK_LOCALHOST_IP, PDB_LISTEN_PORT, PDB_ACCESS_PASSWORD);
	if(nRet != PK_SUCCESS)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "procmgr Main Task, Initialize Sub failed, ret:%d", nRet);
	}
	nRet = g_redisRW.Initialize(PK_LOCALHOST_IP, PDB_LISTEN_PORT, PDB_ACCESS_PASSWORD);
	if(nRet != PK_SUCCESS)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "procmgr Main Task, Initialize RW failed, ret:%d", nRet);
	}


	int nErr = TASK1->Start();
	nErr = TASK2->Start();
	return nErr;
}

int CRedisTestor::OnStop()
{
	TASK1->Stop();
	TASK2->Stop();
	return 0;
}
