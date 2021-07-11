// XLVideoCommServer.cpp : Defines the entry point for the console application.
//

//包含应用头文件
#include "ace/ACE.h"
#include "SystemConfig.h"
#include "common/pkProcessController.h"
#include "MainTask.h"
#include "common/pklog.h"

//定义全局配置对象
CCVProcessController g_cvProcController("forwardservice");

long InitGlobals();
long ExitGlobals();
extern CPKLog PKLog;

void PrintCmdTips()
{
	printf("+=============================================+\n");
	printf("|  <<欢迎使用驱动转发服务>>                 |\n");
	printf("|  可以输入如下指令进行设置                   |\n");
	printf("|  ----q/Q:退出                               |\n");
	printf("+=============================================+\n");
}

void InteractiveLoop()
 {
	bool bAlive = true;
	while(bAlive)
	{
		if(g_cvProcController.CV_KbHit())
		{
			char chCmd = tolower(getchar());

			switch(chCmd)
			{
			case 'q':
				{
					bAlive = false;
				}
				break;

			default:
				break;
			}
		}
		
		if(g_cvProcController.IsProcessQuitSignaled())
		{
			bAlive = false;
			break;
		}

		ACE_Time_Value tvTimeOut(1,0);
		ACE_OS::sleep(tvTimeOut);
	}

}


void WaitToExit()
{
	printf("ready to exit, press any key to continue.....");
	getchar();
}

//主程序入口
int ACE_TMAIN(int argc, char* argv[])
{
	PrintCmdTips();	

	PKLog.SetLogFileName("forwardservice");
	//读取配置文件
	if (!SYSTEM_CONFIG->ReadConfig())
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "读取配置文件失败");
		WaitToExit();
		return -1;
	}
	
	if(SYSTEM_CONFIG->m_mapModbusPortToForwardInfo.size() <= 0)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "从配置文件中未读取到有效的转发配置(个数为0)");
		WaitToExit();
		return -1;
	}

	PKLog.LogMessage(PK_LOGLEVEL_ERROR, "读取配置文件成功");

	//初始化
	ACE::init();

	// 是否接收键盘输入
	g_cvProcController.SetConsoleParam(argc, argv);

	InitGlobals();

	int nListenPort = SYSTEM_CONFIG->m_mapModbusPortToForwardInfo.begin()->first;
	MAIN_TASK->SetListenPort(nListenPort);


	MAIN_TASK->Start();

	//开启主线程人机交互
	InteractiveLoop();

	//线程安全退出, send stop signal and wait for exit....
 	MAIN_TASK->Stop();

	ExitGlobals();
	ACE::fini();
	return 0;
}

long InitGlobals()
{

	return 0;
}

long ExitGlobals()
{
	return 0;
}