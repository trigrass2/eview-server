/**************************************************************
 *  Filename:    CRService.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description:  Defines the entry point for the console application.
 *
 *  @author:     shijunpu
 *  @version     05/23/2008  shijunpu  Initial Version
**************************************************************/

#include "LinkServer.h"
#include "pkcomm/pkcomm.h"
#include "pkserver/PKServerBase.h"
#include "pkcrashdump/pkcrashdump.h"

#include "pklog/pklog.h"
#include "pkcomm/pkcomm.h"
#include "ActionTask.h"
#include "MainTask.h"

CPKLog g_logger;

#define		SLEEPSECONDS_BEFORE_UNCOMMONEXIT		2	// wait seconds before exceptional exit

class CLinkServer:public CPKServerBase
{
public:
	CLinkServer();
	virtual ~CLinkServer(){};

	virtual int	OnStart( int argc, ACE_TCHAR* args[] );
	
	virtual int OnStop();
	
	virtual void PrintStartUpScreen();

	virtual void PrintHelpScreen();
	//设置日志文件名称，保证打印ServiceBase里的日志到期望的文件中
	virtual void SetLogFileName()
	{
		g_logger.SetLogFileName("pklinkserver");
	}

	void StartAllTasks();
};


CLinkServer::CLinkServer() : CPKServerBase(true)
{

}

int CLinkServer::OnStart( int argc, ACE_TCHAR* args[] )
{
	printf(_("========== pklinkserver Init =========\n"));

	int nErr = MAIN_TASK->StartTask();

	return nErr;
}

int CLinkServer::OnStop()
{
	MAIN_TASK->StopTask();
	
	return 0;
}

void CLinkServer::PrintStartUpScreen()
{
	printf("+=================================================================+\n");
	printf("|						<<Welcome to pklinkserver!>>			      |\n");
	printf("|  You can configure the service by enter the following commands  |\n");
	printf("|  q/Q:Quit														  |\n");
	printf("|  Others:Print tips											  |\n");
	printf("+=================================================================+\n");
}
void CLinkServer::PrintHelpScreen()
{
	PrintStartUpScreen();
}

CLinkServer *g_pLinkServer = NULL;

int main(int argc, char* args[])
{
	g_logger.SetLogFileName(PKComm::GetProcessName());
	g_pLinkServer = new CLinkServer();
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "===============PKLinkServer::Main(), argc(%d) start..===============", argc);
	int nRet = g_pLinkServer->Main(argc, args);
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "PKLinkServer::Main Return(%d)", nRet);
	if (NULL != g_pLinkServer)
	{
		delete g_pLinkServer;
		g_pLinkServer = NULL;
	}
	return nRet;
}