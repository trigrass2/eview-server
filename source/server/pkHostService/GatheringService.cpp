#include "pkserver/PKServerBase.h"
#include "pklog/pklog.h"
#include <iostream>
#include "TimerTask.h" 
#include "CommunicateServiceTask.h"
#include "pkcrashdump/pkcrashdump.h"
extern CPKLog g_logger;
using namespace std;

class GatheringService : public CPKServerBase
{
public: 
	GatheringService();
	virtual ~GatheringService(){ };

public:	
	virtual int	OnStart(int argc, char* args[]);

	virtual int OnStop();

	virtual void PrintStartUpScreen();

	virtual void PrintHelpScreen();
	//设置日志文件名称，保证打印ServiceBase里的日志到期望的文件中
	virtual void SetLogFileName()
	{
		g_logger.SetLogFileName("pkHostService_Log");
	} 
};

GatheringService::GatheringService() : CPKServerBase(false)
{

}

int GatheringService::OnStart(int argc, char* args[])
{	   
	TIMERTASK->Start(CommunicateServiceTask::OnTimeOutDo);
	//for gather initialize data done 

	COMMUNICATESERVICE_TASK->StartTask();

	return 0;
}

int GatheringService::OnStop()
{
	COMMUNICATESERVICE_TASK->StopTask();
	TIMERTASK->Stop();
	return 0;
}

void GatheringService::PrintStartUpScreen()
{
	printf("+=================================================================+\n");
	printf("  <<Welcome to pkHostService!>>\n");
	printf("  You can configure the service by enter the following key \n");
	printf("  Q/q:Quit	\n");
	printf("  Others:Print tips\n");
	printf("+=================================================================+\n");
}

void GatheringService::PrintHelpScreen()
{
	PrintStartUpScreen();
}
 

/**
 *  程序入口函数.
 *
 *  @param		-[in]  int argc: [参数]
 *  @param		-[in,out]  char* args[]: [参数]
 *
 *  @return		0. 
 *
 *  @version	09/10/2016	wanghongwei   Initial Version
*/
int main(int argc, char* args[])
{
	GatheringService *g_pServer = new GatheringService();
    g_pServer->SetLogFileName();
    g_logger.LogMessage(PK_LOGLEVEL_INFO, "pkHostService::DoService(), argc(%d).", argc);
    int nRet = g_pServer->Main(argc, args);
    g_logger.LogMessage(PK_LOGLEVEL_INFO, "pkHostService::DoService Return(%d)", nRet);
	if (NULL != g_pServer)
	{
		delete g_pServer;
		g_pServer = NULL;
	}
    return nRet;
}
