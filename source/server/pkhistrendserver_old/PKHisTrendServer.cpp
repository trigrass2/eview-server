/**************************************************************
*  Filename:    PKHisTrendServer.cpp
*  Copyright:   Shanghai Peakinfo Software Co., Ltd.
*
*  Description:	历史趋势服务程序
*
*  @author:     xingxing
*  @version     04/05/2017 xingxing  Initial Version
**************************************************************/
#include "PKHisTrendServer.h"
#include "pkserver/PKServerBase.h"
#include "pklog/pklog.h"
#include "common/pkcomm.h"
#include <iostream>


CPKHisTrendServer::CPKHisTrendServer() : CPKServerBase("CPKHisTrendServer", true)
{

}

int CPKHisTrendServer::OnStart( int argc, ACE_TCHAR* args[] )
{
	printf("========== CPKHisTrendServer Init =========\n");
	int lRet = PKEviewDbApi.Init();
	if(lRet < 0)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "Init Datebase failed!");
		return PK_FAILURE;
	}
	MAIN_TASK->Start();
	return PK_SUCCESS;
}

int CPKHisTrendServer::OnStop()
{
	int lRet = PKEviewDbApi.Exit();
	if(lRet <0)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "Uninit Datebase failed!");
		return PK_FAILURE;
	}
	MAIN_TASK->Stop();
	return PK_SUCCESS;
}

void CPKHisTrendServer::PrintStartUpScreen()
{
	printf("\n+=================================================================+\n");
	printf("  <<Welcome to CPKHisTrendServer!>>\n");
	printf("  You can configure the service by enter the following key \n");
	printf("  Q/q:Quit	\n");
	printf("  Others:Print tips\n");
	printf("+=================================================================+\n");
}
void CPKHisTrendServer::PrintHelpScreen()
{
	PrintStartUpScreen();
}

int main(int argc, char* args[])
{
	PKLog.SetLogFileName("CPKHisTrendServer");
	g_pServer = new CPKHisTrendServer();
	int nRet = g_pServer->Main(argc, args);
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "CPKHisTrendServer::DoService(), argc(%d).", argc);
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "CPKHisTrendServer::DoService Return(%d)", nRet);
	if (NULL != g_pServer)
	{
		delete g_pServer;
		g_pServer = NULL;
	}
	return PK_SUCCESS;
}
