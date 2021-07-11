#include "pkserver/PKServerBase.h"

#include "pklog/pklog.h"
#include "pkcomm/pkcomm.h"
#include <stdio.h>
CPKLog PKLog;

#define	SLEEPSECONDS_BEFORE_UNCOMMONEXIT		2	// wait seconds before exceptional exit
#ifndef PK_SUCCESS
#define PK_SUCCESS							0								// iCV系统中所有成功消息的返回码
#endif

class CPKSampleServer:public CPKServerBase
{
public:
	CPKSampleServer();
	virtual ~CPKSampleServer(){};

	virtual int	OnStart( int argc, char* args[] );

	virtual int OnStop();

	virtual void PrintStartUpScreen();

	virtual void PrintHelpScreen();
	//设置日志文件名称，保证打印ServiceBase里的日志到期望的文件中
	virtual void SetLogFileName()
	{
		PKLog.SetLogFileName("PKSampleServer");
	}

	void StartAllTasks();
};


CPKSampleServer::CPKSampleServer() : CPKServerBase("PKSampleServer", true)
{

}

int CPKSampleServer::OnStart(int argc, char* args[])
{
	printf("========== PKSampleServer Init =========\n");

	int nErr = 0;//MAIN_TASK->StartTask();

	return nErr;
}

int CPKSampleServer::OnStop()
{
	//MAIN_TASK->StopTask();

	return 0;
}

void CPKSampleServer::PrintStartUpScreen()
{
	printf("+=================================================================+\n");
	printf("  <<Welcome to PKSampleServer!>>\n");
	printf("  You can configure the service by enter the following key \n");
	printf("  Q/q:Quit	\n");
	printf("  Others:Print tips\n");
	printf("+=================================================================+\n");
}
void CPKSampleServer::PrintHelpScreen()
{
	PrintStartUpScreen();
}

CPKSampleServer *g_pServer = NULL;

int main(int argc, char* args[])
{
	PKLog.SetLogFileName("PKSampleServer");
	g_pServer = new CPKSampleServer();
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "PKNodeServer::DoService(), argc(%d).", argc);
	int nRet = g_pServer->Main(argc, args);
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "PKNodeServer::DoService Return(%d)", nRet);
	if (NULL != g_pServer)
	{
		delete g_pServer;
		g_pServer = NULL;
	}
	return nRet;
}
