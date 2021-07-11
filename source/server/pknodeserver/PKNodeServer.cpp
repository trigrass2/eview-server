/**************************************************************
 *  Filename:    RMService.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: RM Service Impl..
 *
 *  @author:     chenzhiquan
 *  @version     05/20/2008  chenzhiquan  Initial Version
**************************************************************/
#include "PKNodeServer.h"
#include <ace/Get_Opt.h>
#include <iostream>
#include "pkserver/PKServerBase.h"

#include "pklog/pklog.h"
#include "pkcomm/pkcomm.h"
#include <ace/OS_NS_sys_stat.h>
#include "NodeCommonDef.h"
#include "MainTask.h"
#include "CtrlProcTask.h"
#include "DataProcTask.h"
#include "TagExprCalc.h"
#include "pkcrashdump/pkcrashdump.h"

CPKLog g_logger;
extern map<string, CDataTag *>	m_mapName2Tag;
CTagExprCalc g_tagExprCalc;

CPKNodeServer *g_pPKNodeServer = NULL;

// 如果质量为bad，szTagVal可能为空
// 如果无时间，时间nSec为0
//int PublishKVSimple2NodeServer(int nActionType, const char *szKey, char *szValue)
//{
//	unsigned int nSec;
//	unsigned int mSec;
//	nSec = PKTimeHelper::GetHighResTime(&mSec);
//
//	char szData[PROCESSQUEUE_DEFAULT_RECORD_LENGHT] = {0};
//
//	// ActionType
//	char *pData = szData;
//	memcpy(pData, &nActionType, sizeof(int));
//	pData += sizeof(int);
//
//	// tag名
//	int nKeyLen = strlen(szKey);
//	memcpy(pData, &nKeyLen, sizeof(int));
//	pData += sizeof(int);
//	memcpy(pData, szKey, nKeyLen);
//	pData += nKeyLen;
//
//	int nValueLen = strlen(szValue);
//	// tag值的长度和tag值内容
//	memcpy(pData, &nValueLen, sizeof(int));
//	pData += sizeof(int);
//	if(nValueLen > 0)
//	{
//		memcpy(pData, szValue, nValueLen);
//		pData += nValueLen;
//	}
//
//	int nRet = g_pQueData2NodeSrv->EnQueue(szData, pData - szData);
//	if(nRet == PK_SUCCESS)
//	{
//		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, _("Driver[%s] send tagdata to PKNodeServer success, type:%d, len:%d, content:%s"), "PKNodeServer", nActionType, nValueLen, szData);
//	}
//	else
//		g_logger.LogMessage(PK_LOGLEVEL_ERROR, _("Driver[%s} send tagdata to PKNodeServer failed, type:%d, len:%d, content:%s, ret:%d"), "PKNodeServer", nActionType, nValueLen, szData, nRet);
//	return nRet;
//}

/**
 *  Constructor.
 *
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 */
CPKNodeServer::CPKNodeServer() : CPKServerBase(true)
{
}

CPKNodeServer::~CPKNodeServer()
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
int CPKNodeServer::OnStart( int argc, char* args[] )
{
	printf(_("========== PKNodeServer Init =========\n"));

	// 初始记为正常启动
	//UpdateServerStatus(0, "started");
	//UpdateServerStartTime();

	int nErr = MAIN_TASK->Start();
	return nErr;
}

int CPKNodeServer::OnStop()
{
	MAIN_TASK->Stop();
	//DATAPROC_TASK->Stop();
	//CTRLPROC_TASK->Stop();
	return 0;
}

// name :procename:status,procname:starttime
//int CPKNodeServer::UpdateServerStatus( int nStatus, char *szErrTip )
//{
//	char szTime[PK_HOSTNAMESTRING_MAXLEN] = {'\0'};
//	PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));
//	// 更新进程状态
//	char szKey[PK_NAME_MAXLEN] = {0};
//	sprintf(szKey, "%s:status", m_szProcName);
//	char szValue[PK_TAGDATA_MAXLEN] = {0};
//	sprintf(szValue, "{\"v\":\"%s\",\"t\":\"%s\",\"q\":\"%d\",\"m\":\"%s\"}","ok",szTime, nStatus, szErrTip);
//	int nRet = PublishKVSimple2NodeServer(ACTIONTYPE_DRV2SERVER_SET_KV_SIMPLE, szKey, szValue);
//	return nRet;
//}

// name :procename:status,procname:starttime
//int CPKNodeServer::UpdateServerStartTime()
//{
//	char szTime[PK_HOSTNAMESTRING_MAXLEN] = {'\0'};
//	PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));
//
//	// 更新进程状态
//	char szKey[PK_NAME_MAXLEN] = {0};
//	char szValue[PK_TAGDATA_MAXLEN] = {0};
//	// 更新启动时间
//	sprintf(szKey, "%s:starttime", m_szProcName);
//	sprintf(szValue, "{\"v\":\"%s\",\"t\":\"%s\",\"q\":\"%d\"}", szTime, szTime, 0);
//	int nRet = PublishKVSimple2NodeServer(ACTIONTYPE_DRV2SERVER_SET_KV_SIMPLE, szKey, szValue);
//	return nRet;
//}

int main(int argc, char* args[])
{
	g_logger.SetLogFileName("pknodeserver");
	g_pPKNodeServer = new CPKNodeServer();
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "===============PKNodeServer::main()======================", argc);
	int nRet = g_pPKNodeServer->Main(argc, args);
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "PKNodeServer::Main Return(%d)", nRet);
	if (NULL != g_pPKNodeServer)
	{
		delete g_pPKNodeServer;
		g_pPKNodeServer = NULL;
	}
	return nRet;
}