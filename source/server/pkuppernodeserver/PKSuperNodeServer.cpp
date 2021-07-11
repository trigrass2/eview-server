/**************************************************************
 *  Filename:    RMService.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: RM Service Impl..
 *
 *  @author:     chenzhiquan
 *  @version     05/20/2008  chenzhiquan  Initial Version
**************************************************************/
#include "PKSuperNodeServer.h"
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
#include "pkcrashdump/pkcrashdump.h"

CPKLog g_logger;

CPKSuperNodeServer *g_pPKNodeServer = NULL;
//CProcessQueue * g_pQueData2NodeSrv;
//
//// �������Ϊbad��szTagVal����Ϊ��
//// �����ʱ�䣬ʱ��nSecΪ0
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
//	// tag��
//	int nKeyLen = strlen(szKey);
//	memcpy(pData, &nKeyLen, sizeof(int));
//	pData += sizeof(int);
//	memcpy(pData, szKey, nKeyLen);
//	pData += nKeyLen;
//
//	int nValueLen = strlen(szValue);
//	// tagֵ�ĳ��Ⱥ�tagֵ����
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
CPKSuperNodeServer::CPKSuperNodeServer() : CPKServerBase(true)
{
}

CPKSuperNodeServer::~CPKSuperNodeServer()
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
int CPKSuperNodeServer::OnStart( int argc, char* args[] )
{
	printf(_("========== PKSuperNodeServer Init =========\n"));

	//string strRunTimePath = PKComm::GetRunTimeDataPath();
	// ���������������ݵĹ������
	//string strPbSrvPath = strRunTimePath + ACE_DIRECTORY_SEPARATOR_STR + MODULENAME_LOCALSERVER;
	//PKFileHelper::CreateDir(strPbSrvPath.c_str());
	//g_pQueData2NodeSrv = new CProcessQueue((char *)strPbSrvPath.c_str(), MODULENAME_LOCALSERVER, true, PROCESSQUEUE_DEFAULT_RECORD_LENGHT, PROCESSQUEUE_DRVDATA_TO_NODE_SERVER_RECORD_COUNT);

	// ��ʼ��Ϊ��������
	UpdateServerStatus(0, "started");
	UpdateServerStartTime();

	int nErr = MAIN_TASK->Start();
	return nErr;
}

int CPKSuperNodeServer::OnStop()
{
	MAIN_TASK->Stop();
	//DATAPROC_TASK->Stop();
	//CTRLPROC_TASK->Stop();
	//if(g_pQueData2NodeSrv)
	//	delete g_pQueData2NodeSrv;
	//g_pQueData2NodeSrv = NULL;
	return 0;
}


// name :procename:status,procname:starttime
int CPKSuperNodeServer::UpdateServerStatus( int nStatus, char *szErrTip )
{
	//char szTime[PK_HOSTNAMESTRING_MAXLEN] = {'\0'};
	//PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));
	//// ���½���״̬
	//char szKey[PK_NAME_MAXLEN] = {0};
	//sprintf(szKey, "%s:status", m_szProcName);
	//char szValue[PK_TAGDATA_MAXLEN] = {0};
	//sprintf(szValue, "{\"v\":\"%s\",\"t\":\"%s\",\"q\":\"%d\",\"m\":\"%s\"}","ok",szTime, nStatus, szErrTip);
	//int nRet = PublishKVSimple2NodeServer(ACTIONTYPE_DRV2SERVER_SET_KV_SIMPLE, szKey, szValue);
	return 0;
}

// name :procename:status,procname:starttime
int CPKSuperNodeServer::UpdateServerStartTime()
{
	//char szTime[PK_HOSTNAMESTRING_MAXLEN] = {'\0'};
	//PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));

	//// ���½���״̬
	//char szKey[PK_NAME_MAXLEN] = {0};
	//char szValue[PK_TAGDATA_MAXLEN] = {0};
	//// ��������ʱ��
	//sprintf(szKey, "%s:starttime", m_szProcName);
	//sprintf(szValue, "{\"v\":\"%s\",\"t\":\"%s\",\"q\":\"%d\"}", szTime, szTime, 0);
	//int nRet = PublishKVSimple2NodeServer(ACTIONTYPE_DRV2SERVER_SET_KV_SIMPLE, szKey, szValue);
	return 0;
}

int main(int argc, char* args[])
{
	g_logger.SetLogFileName(NULL);
	g_pPKNodeServer = new CPKSuperNodeServer();
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "===============PKSuperNodeServer::main()======================", argc);
	int nRet = g_pPKNodeServer->Main(argc, args);
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "PKSuperNodeServer::Main Return(%d)", nRet);
	if (NULL != g_pPKNodeServer)
	{
		delete g_pPKNodeServer;
		g_pPKNodeServer = NULL;
	}
	return nRet;
}