/**************************************************************
 *
 *  @author:     shijunpu
 *  @version     05/20/2008  shijunpu  Initial Version
**************************************************************/
#include "DbTransferServer.h"
#include <ace/Get_Opt.h>
#include <iostream>
#include "pkserver/PKServerBase.h"
#include "pklog/pklog.h"
#include "pkcomm/pkcomm.h"
#include <ace/OS_NS_sys_stat.h>
#include "pkcrashdump/pkcrashdump.h"

CPKLog g_logger;
CDbTransferServer *g_DbTransferServer = NULL;

/**
 *  Constructor.
 *
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 */
CDbTransferServer::CDbTransferServer() : CPKServerBase(true)
{
}

CDbTransferServer::~CDbTransferServer()
{
}

/**
 *  Initialize Service,Access of Program;
 *
 *  @param  -[in,out]  int  argc: [comment]
 *  @param  -[in,out]  ACE_TCHAR*  args[]: [comment]
 *
 *  @version     05/19/2008  chenzhiquan  Initial Version.;
 �ڶ�������Ϊ�ͻ�����;
 */

int CDbTransferServer::OnStart( int argc, ACE_TCHAR* args[] )
{
	printf(("========== DbTransferServer Init =========\n"));

	//��ȡ����ʱ����Ŀ¼;
	string strRunTimePath = PKComm::GetRunTimeDataPath();
    // ���������������ݵĹ������;

	int nErr = MAIN_TASK->Start();
	return nErr;
}

int CDbTransferServer::OnStop()
{
	MAIN_TASK->Stop();
	return 0;
}

//#ifdef YCD
int main(int argc, char* args[])
{
	g_logger.SetLogFileName(PKComm::GetProcessName());
    g_DbTransferServer = new CDbTransferServer();
    g_logger.LogMessage(PK_LOGLEVEL_INFO, "===============DbTransferServer::Main(), argc(%d) start...===============", argc);

    int nRet = g_DbTransferServer->Main(argc, args);
    g_logger.LogMessage(PK_LOGLEVEL_INFO, "DbTransferServer::Main Return(%d)", nRet);
    if (NULL != g_DbTransferServer)
	{
        delete g_DbTransferServer;
        g_DbTransferServer = NULL;
	}
	return nRet;
}
//#endif