/**************************************************************
 *  Filename:    CRService.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description:  Defines the entry point for the console application.
 *
 *  @author:     shijunpu
 *  @version     05/23/2008  shijunpu  Initial Version
**************************************************************/

#include "MainTask.h"
#include "pklog/pklog.h"
#include "pkcomm/pkcomm.h"
#include "PKHisDataServer.h"

#ifdef _WIN32
#include "common/RegularDllExceptionAttacher.h"
CRegularDllExceptionAttacher g_ExceptionAttacher;

#pragma comment(lib,"dbghelp.lib")
#pragma comment(lib,"ExceptionReport.lib")
#endif

CPKLog PKLog;
#define _x(x)	x
#define		SLEEPSECONDS_BEFORE_UNCOMMONEXIT		2	// wait seconds before exceptional exit

CPKHisDataServer::CPKHisDataServer() : CPKServerBase("pkhisdataserver", true)
{

}

int CPKHisDataServer::OnStart( int argc, ACE_TCHAR* args[] )
{
	printf("========== PKHisDataServer Init =========\n");

	int nErr = MAIN_TASK->StartTask();

	return nErr;
}

int CPKHisDataServer::OnStop()
{
	MAIN_TASK->StopTask();
	
	return 0;
}

void CPKHisDataServer::PrintStartUpScreen()
{
	printf("\n");
	printf("------------Welcome to PKHisDataServer-----------\n");
	printf("\tsupport the following commands	             \n");
	printf("\tq/Q:Quit                                       \n");
	printf("\tOthers:Print tips                              \n");
	printf("-------------------------------------------------\n");
}
void CPKHisDataServer::PrintHelpScreen()
{
	PrintStartUpScreen();
}

int main(int argc, char* args[])
{
	PKLog.SetLogFileName("pkhisdataserver");
	CPKHisDataServer *pServer =new CPKHisDataServer();
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "start PKHisDataServer::main(argc(%d)).", argc);
	int nRet = pServer->Main(argc, args);
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "PKHisDataServer::main stopped return(%d)", nRet);
	if (NULL != pServer)
	{
		delete pServer;
		pServer = NULL;
	}
	return nRet;
}