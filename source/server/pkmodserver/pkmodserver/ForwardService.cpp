/**************************************************************
 *  Filename:    CCService.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description:  Defines the entry point for the console application.
 *
 *  @author:     shijunpu
 *  @version     05/23/2008  shijunpu  Initial Version
**************************************************************/

#include "ace/ACE.h"
#include "pkserver/PKServerBase.h"
#include "pkcomm/pkcomm.h"
#include "SystemConfig.h"
#include "MainTask.h"
#include "pklog/pklog.h"
#include "pkcrashdump/pkcrashdump.h"

CPKLog g_logger;

class CPKModbusServer : public CPKServerBase
{
public:
	CPKModbusServer() : CPKServerBase (true)
	{
	}
	
	virtual int OnStart(int argc, ACE_TCHAR* args[]);
	virtual int OnStop();

	void PrintHelpScreen();
	//设置日志文件名称，保证打印ServiceBase里的日志到期望的文件中
	virtual void SetLogFileName()
	{
		g_logger.SetLogFileName("pkmodserver");
	}
};

void WaitToExit()
{
	printf("ready to exit, press any key to continue.....");
	getchar();
}

int CPKModbusServer::OnStart( int argc, char* args[] )
{
	chdir(PKComm::GetBinPath());
	
	//读取配置文件
	if (0 != SYSTEM_CONFIG->ReadConfig())
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "加载配置文件失败");
		WaitToExit();
		return -1;
	}
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "加载配置文件成功");

	MAIN_TASK->Start();	


	//初始化
	// ACE::init();
	return 0;
}

int CPKModbusServer::OnStop()
{
	//线程安全退出, send stop signal and wait for exit....
	MAIN_TASK->Stop();

	//ACE::fini();

	return PK_SUCCESS;
}

void CPKModbusServer::PrintHelpScreen()
{
	printf("\n");
	printf("+=============================================+\n");
	printf("|  <<转发为modbustcp & modbus rtu 协议>>      |\n");
	printf("|  ----q/Q:退出                               |\n");
	printf("+=============================================+\n");
}

int main(int argc, char* args[])
{
	g_logger.SetLogFileName(PKComm::GetProcessName());
	CPKModbusServer *pModServer = new CPKModbusServer();
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "===============PKModbusServer::Main(), argc(%d) start..===============", argc);
	int nRet = pModServer->Main(argc, args);
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "PKModbusServer::Main Return(%d)", nRet);
	if (NULL != pModServer)
	{
		delete pModServer;
		pModServer = NULL;
	}
	return nRet;
}