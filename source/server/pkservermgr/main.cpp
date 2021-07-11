/**************************************************************
 *  Filename:    CCAgent.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 服务端程序的入口.
 *
 *  @author:     fengjuanyong
 *  @version     06/18/2008  fengjuanyong  Initial Version
**************************************************************/
#include <ace/OS_NS_unistd.h>
#include <iostream>
#include <ace/ACE.h>
#include <ace/Get_Opt.h>
#include <ace/Process_Mutex.h>
#include <ace/Event_Handler.h>


#define CRYPTOPP_DEFAULT_NO_DLL
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1

#include "pklog/pklog.h"
#include "common/SigPipeHandler.h"
#include "pkcomm/pkcomm.h"
#include "MainTask.h"
#include "common/eviewdefine.h"
#ifdef _WIN32
#include "conio.h"
#else
#include<stdio.h>
#endif
//#include "cryptopp/dll.h"
#include "cryptopp/cryptlib.h"
#include "pkprocesscontroller/pkprocesscontroller.h"

#if !(defined _WIN32) || (_MSC_VER >= 1400)
#include <ace/Sig_Handler.h>
#endif

#ifdef _WIN32
#ifndef NO_EXCEPTION_ATTACHER
#include "common/RegularDllExceptionAttacher.h"
#pragma comment(lib, "ExceptionReport.lib")
#pragma comment(lib, "dbghelp.lib")
// global member to capture exception so as to analyze 
static CRegularDllExceptionAttacher	g_RegDllExceptAttacher(1);
#endif//#ifndef NO_EXCEPTION_ATTACHER

extern int ProcessManualCmd(int argc, char* argvs[]);
extern int StartServiceByManager();
#endif

#define		_(STRING) STRING
#define		SWITCH_CONTROLLER_MUTEXNAME			"Global\\PROCMGR_SWITCH_ALREADY"
#define		SLEEPSECONDS_BEFORE_UNCOMMONEXIT	2	// wait seconds before exceptional exit
#define		PROCESS_ALIVE_MUTEXNAME				"Global\\PROCMGR_ALIVE_ALREADY"	// 使用CPKProcessController的全局mutex，检查管理器是否关闭

using namespace std;

extern string		g_strServiceName;
#define		PKSERVERMGR_EXE_NAME	"pkservermgr"
CPKLog PKLog;
CPKProcessController g_pkProcController(PKSERVERMGR_EXE_NAME);// 进程启停控制器
bool				g_bAlive = true;
bool				g_bRestart = false;
ACE_Process_Mutex *	g_pSwitchMutex = NULL;
bool				g_bSwitch = false;
char				g_chPreCmd = 0;		// 可以切换服务管理同时进行管理控制

void PrintCmdTips();
bool SetConsoleParam(int argc, char* argvs[]);
void InteractiveLoop();

#ifdef _WIN32
/**
 *  @brief    (CtrlHandler ONLY works in WIN32). 
 *  (!!! IMPORTANT !!! SetConsoleCtrlHandler would kill all child-processes before CtrlHandler is called).
 */
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType) 
{ 
    switch (fdwCtrlType) 
    { 
        // Handle the CTRL+C signal. 
        case CTRL_C_EVENT: 
            return true; 
 
        // CTRL+CLOSE
        case CTRL_CLOSE_EVENT: 
			//SignalToExit();
			PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "serice will stop......");
			MAIN_TASK->Stop();
			Sleep(5);
			g_bAlive = false;
			g_bRestart = false;


            return true; 
 
        // Pass other signals to the next handler. 
        case CTRL_BREAK_EVENT: 
        case CTRL_LOGOFF_EVENT: 
        case CTRL_SHUTDOWN_EVENT: 
        default: 
            return false; 
    } 
}
#endif

int StopServerMgr()
{
	char szExitMutexName[PK_LONGFILENAME_MAXLEN];
	sprintf(szExitMutexName, "Global\\PROC_%s_NOTIFY_QUIT", PKSERVERMGR_EXE_NAME);
	ACE_Process_Mutex mutexQuitSignal(szExitMutexName); 
	int nRet = mutexQuitSignal.tryacquire(); // 触发已经运行的服务管理器的退出信号
	if (nRet != 0)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "cannot signal quit pkservermgr, because cannot acquire mutex(mutex:%s)!", szExitMutexName);
		return -1;
	}
	else
	{
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "quit pkservermgr signaled for 3 seconds(mutex:%s)!", szExitMutexName);
	}

	PKComm::Sleep(3000); // 持有信号量3秒, 防止对方收不到消息
	mutexQuitSignal.release();
	mutexQuitSignal.remove(); // 删除信号量 
	return 0;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argvs[])
{
	printf("----usage: %s without parameter to start all managed server. OR %s parameter to control manager. (when registered as service, service name: %s)\n", PKSERVERMGR_EXE_NAME, PKSERVERMGR_EXE_NAME, g_strServiceName.c_str());
	printf("----%s, to start all managed server %s\n", PKSERVERMGR_EXE_NAME, g_strServiceName.c_str());
#ifdef _WIN32
	printf("----%s install, to install service %s\n", PKSERVERMGR_EXE_NAME, g_strServiceName.c_str());
	printf("----%s uninstall, to uninstall service %s\n", PKSERVERMGR_EXE_NAME, g_strServiceName.c_str());
	//printf("%s restart, to restart service %s\n", PKSERVERMGR_EXE_NAME, g_strServiceName.c_str());
#else
	printf("----%s, need to add to /etc/init.d or system service manually,service name: %s\n", PKSERVERMGR_EXE_NAME, g_strServiceName.c_str());
#endif
	printf("----%s start, to start service %s\n", PKSERVERMGR_EXE_NAME, g_strServiceName.c_str());
	printf("----%s stop, to kill service %s\n", PKSERVERMGR_EXE_NAME, g_strServiceName.c_str());
	printf("\n\n");

	if (argc >= 2) // 1个参数，是注销、注册、启动、停止等控制命令
	{
		char *szParam1 = argvs[1];
#ifdef _WIN32
		if (PKStringHelper::StriCmp("install", szParam1) == 0 || PKStringHelper::StriCmp("-install", szParam1) == 0 
			|| PKStringHelper::StriCmp("uninstall", szParam1) == 0 || PKStringHelper::StriCmp("-uninstall", szParam1) == 0)
		{ // 无论是否已经运行. 按照已经注册了服务来考虑
			ProcessManualCmd(argc, argvs);
			return 0;
		}
#endif

		if (PKStringHelper::StriCmp("kill", szParam1) == 0 || PKStringHelper::StriCmp("-k", szParam1) == 0 || PKStringHelper::StriCmp("/k", szParam1) == 0 || PKStringHelper::StriCmp("-kill", szParam1) == 0 || PKStringHelper::StriCmp("/kill", szParam1) == 0
			|| PKStringHelper::StriCmp("stop", szParam1) == 0 || PKStringHelper::StriCmp("-stop", szParam1) == 0 || PKStringHelper::StriCmp("/stop", szParam1) == 0 || PKStringHelper::StriCmp("-stop", szParam1) == 0 || PKStringHelper::StriCmp("/kill", szParam1) == 0)
		{
			StopServerMgr();
			return 0;
		}
	}

	// UNIX系统里，可能控制台已关闭，需要重启一个cvprocessmgr进行管理
	// 首先新建一个接受切换服务管理请求的信号
	g_pSwitchMutex = new ACE_Process_Mutex(SWITCH_CONTROLLER_MUTEXNAME);
	// 输入参数的功能与InteractiveLoop里一样，是为了接管进程后进一步控制
	SetConsoleParam(argc,argvs);
	PKLog.SetLogFileName(PKSERVERMGR_EXE_NAME);

	// assure only one process is running
	if(g_pkProcController.SetProcessAliveFlag() != PK_SUCCESS)
	{
		PKLog.LogMessage(PK_LOGLEVEL_CRITICAL, "another process manager has been started! exit...");
		return -1;
	}
	//PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "main()");
	// 如果有一个参数，则认为是注册服务、注销服务、启动服务等操作
#ifdef _WIN32
	// 无参数或3个参数时, 继续向后面走
    if (! SetConsoleCtrlHandler( (PHANDLER_ROUTINE) CtrlHandler, TRUE) ) 
		ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T: Could not set control handler \n")));
#endif 
	// 是否接收键盘输入
	g_pkProcController.SetConsoleParam(argc,argvs);

	ACE_Sig_Handlers handler;
	CSigPipeHandler SigPipeH;
	handler.register_handler(SIGPIPE, &SigPipeH);
	
	//PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "maintask start...");
	MAIN_TASK->Start();
	//PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "maintask started");

	// 有2个参数认为是通过nt启动 d:\Work\eview\bin\pkservermgr.exe runas ntservice
	if(argc >=3 )
	{
#ifdef _WIN32
		StartServiceByManager();  // 这里会一直阻塞......直到服务为stopped
#endif
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "arc=%d>=3,after ProcessNTCmd",argc);
		for(int i = 1; i < argc; i ++)
		{
			PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "arg%d:,value:%s", i, argvs[i]);
		}
	}
	else  // 一个参数,则认为是手工双击程序启动, 进入处理命令过程
	{
		do 
		{
			InteractiveLoop();
		}
		while(g_bRestart);
	}

	PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "service will stop......");
	MAIN_TASK->Stop();
	//删除服务切换mutex
	if (g_pSwitchMutex)
	{
		delete g_pSwitchMutex;
		g_pSwitchMutex = NULL;
	}
	PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "service stop ok, now exit(0)");
	return PK_SUCCESS;
}

/**
 *  process the console command interacting with user.
 *
 *  @return $(no return).
 *
 *  @version     05/23/2008  shijunpu  Initial Version.
 */
void InteractiveLoop()
{
	PrintCmdTips();
	g_bAlive = true;
	g_bRestart = false;
	int nCount = 0;
	
 	while(g_bAlive)
    {
        if(g_pkProcController.KbHit() || g_chPreCmd)
		{
			char chCmd = 0;
			if (g_chPreCmd)
			{
				chCmd = tolower(g_chPreCmd);
				g_chPreCmd = 0;
			}
			else
			{
#ifdef _WIN32
				char chTmp = getch();
#else
                char chTmp = getchar();
#endif
				//char chTmp2 = getch();
				chCmd = tolower(chTmp);
			}
            //printf("recv char:---%c---\n", chCmd);
            /*
			switch(chCmd)
			{
			case 'q':
				g_bAlive = false;
				g_bRestart = false;
				// 这里不需要调用，因为马上就会退出InteractiveLoop，退出时会调用下面语句退出
				// processMgr.NotifyAllProcessStop(false);
				break;

			case 'r':
				//g_bAlive = false;
				//g_bRestart = true;
				MAIN_TASK->Signal2StopAllProcess(true);
				break;			
			//case 'f':
				//g_bAlive = false;
				//g_bRestart = true;
			//	MAIN_TASK->Signal2NotifyAllProcessRefresh();
			//	break;			
			case 'v':
				MAIN_TASK->Signal2PrintAllProcessState();
				break;
			case 'l':
				MAIN_TASK->Signal2PrintLicenseState();
				break;
			case 10: // 回车
				PrintCmdTips();
				break;
			default:
				int nProcNum = MAIN_TASK->m_cvProcMgr.m_processList.size();
				int nProcNo = chCmd - '0';
				if(nProcNo > 1 && nProcNo <= nProcNum)
				{
					MAIN_TASK->Signal2ToggleProcessShowHide(nProcNo);
				}
                printf("recv cmd unknown! ----%c\n", chCmd);
                //else
                //	PrintCmdTips();
				break;
			}
            */
		} //if(g_pkProcController.CV_KbHit() || g_chPreCmd)

		// 判断是否切换信号量被触发
		if (g_pSwitchMutex->tryacquire() != 0)
		{
			g_bSwitch = true;
			g_bAlive = false;
			g_bRestart = false;
			break;
		}
		g_pSwitchMutex->release();

		// 判断是否退出信号量被触发
		if(g_pkProcController.IsProcessQuitSignaled())
		{
			g_bAlive = false;
			break;
		}

		ACE_Time_Value tvSleep;
		tvSleep.msec(20); // 100ms
		ACE_OS::sleep(tvSleep);	
		// Sleep(20);
		//ACE_OS::sleep(1);
	} // while(g_bAlive)
}

bool SetConsoleParam(int argc, char* argvs[])	
{
	ACE_Get_Opt cmd_opts(argc, const_cast<char **>(argvs), "");
	cmd_opts.long_option("switch", 's', ACE_Get_Opt::NO_ARG);
	cmd_opts.long_option("restart", 'r', ACE_Get_Opt::NO_ARG);

	long nOption = 0;
	bool bOptShot = false;

	while( (nOption = cmd_opts()) != EOF)
	{
		switch(nOption)
		{
		case 's': // switch process controller
			printf(_("switch to current process manager\n"));
			bOptShot = true;
			break;		
		case 'r':
			printf(_("switch process manager and restart processes\n"));
			g_chPreCmd = 'r';
			bOptShot = true;
		default:
			break;
		}
	}

	if (!bOptShot)
		return false;

	//服务管理切换到当前进程
	ACE_Time_Value tvSleep(0, 100*1000);	// 每次休息100ms	
	ACE_Time_Value tvTimeout = ACE_OS::gettimeofday() + ACE_Time_Value(1,0); // 处理超时时间1s
	while(1)
	{
		//向另外的进程发送切换服务管理信号，并等待退出
		if (g_pSwitchMutex->tryacquire() == 0)
		{
			//检查是否已退出
			char szAliveMutexName[PK_LONGFILENAME_MAXLEN];
			strcpy(szAliveMutexName, PROCESS_ALIVE_MUTEXNAME);	
			ACE_Process_Mutex *pMutexProcAlive = new ACE_Process_Mutex(szAliveMutexName);
			// 由于服务管理器可能需要等待其它进程退出，这里等待超时设为30s
			tvTimeout = ACE_OS::gettimeofday() + ACE_Time_Value(30,0); // 处理超时时间1s
			while(1)
			{
				if (pMutexProcAlive->tryacquire() == 0)
				{
					pMutexProcAlive->release();
					break;
				}
				//取不到说明另一个进程还未退出，等待后再次尝试
				ACE_OS::sleep(tvSleep);
				//如果超时，不再尝试
				if (tvTimeout < ACE_OS::gettimeofday())
				{
					printf(_("Failed to switch process manager due to another instance of cvprocessmgr already running, and then quit!\n"));
					break;
				}
			}
			delete pMutexProcAlive;
			g_pSwitchMutex->release();
			break;
		} // if (g_pSwitchMutex->tryacquire() == 0)

		//取不到说明另一个进程正在检查，等待后再次尝试
		ACE_OS::sleep(tvSleep);
		//如果超时，不再尝试
		if (tvTimeout < ACE_OS::gettimeofday())
		{
			break;
		}
	} // while(1)
	return bOptShot;
}

void PrintCmdTips()
{
	printf("+============================================================+\n");
#ifdef _WIN32
	printf("  %-60.60s\n", _("<<Process Manager! >>"));
	printf("  %-60.60s\n", _("You can conigure by entering the following commands"));
	printf("  %-60.60s\n", _("q/Q:Quit all processes and sub-processes"));
	printf("  %-60.60s\n", _("R/r:Restart all processes"));
	printf("  %-60.60s\n", _("V/v:Print status of all running sub-processes"));
	printf("  %-60.60s\n", _("L/l:Print status of this service"));
	//printf("  %-60.60s\n","<<进程管理器>>");
	//printf("  %-60.60s\n","支持命令如下:");
	//printf("  %-60.60s\n","q/Q:退出进程管理器本身, 并退出所管理的所有进程"));
	//printf("  %-60.60s\n","R/r:重启所有进程");
	//printf("  %-60.60s\n", "V/v:打印被管理的所有进程状态");
	//printf("  %-60.60s\n","L/l:打印本服务状态");
#else // linux下可能不支持中文
	printf("  %-60.60s\n",_("<<Shanghai Peakinfo Co.Ltd. Process Manager! >>"));
	printf("  %-60.60s\n",_("You can conigure by entering the following commands"));
	printf("  %-60.60s\n",_("q/Q:Quit all processes and sub-processes"));
	printf("  %-60.60s\n",_("R/r:Restart all processes"));
	printf("  %-60.60s\n",_("V/v:Print status of all running sub-processes"));
	printf("  %-60.60s\n",_("L/l:Print status of this service"));
#endif
	printf("+============================================================+\n");
}


int PushInt32ToBuffer(char *&pBufer, int &nLeftBufLen, int nValue)
{
	if(nLeftBufLen < 4)
		return -1;

	memcpy(pBufer, &nValue, 4);
	pBufer +=4;
	nLeftBufLen -= 4;

	return 0;
}

// 将字符串推到缓冲区中去
int PushStringToBuffer(char *&pSendBuf, int &nLeftBufLen, const char *szStr, int nStrLen)
{
	if(nLeftBufLen < nStrLen + 4)
		return -1;

	memcpy(pSendBuf, &nStrLen, 4);
	pSendBuf +=4;
	nLeftBufLen -= 4;

	if(nStrLen > 0)
	{
		memcpy(pSendBuf, (char *)szStr, nStrLen);
		pSendBuf += nStrLen;
		nLeftBufLen -= nStrLen;
	}

	return 0;
}

// 从缓冲区字符串出，字符串先存放4个字节长度，再存放实际内容（不包含0结束符）
// nLeftBufLen 长度也会被改变
int PopStringFromBuffer(char *&pBufer, int &nLeftBufLen, string &strValue)
{
	strValue = "";
	if(nLeftBufLen < 4)
		return -1;

	int nStrLen = 0;
	memcpy(&nStrLen, pBufer, 4);
	nLeftBufLen -= 4;
	pBufer += 4;

	if(nStrLen <= 0) // 字符串长度为空
		return 0;

	if(nStrLen > nLeftBufLen) // 剩余的缓冲区长度不够
		return -2;

	char *pszValue = new char[nStrLen + 1];
	memcpy(pszValue, pBufer, nStrLen);
	nLeftBufLen -= nStrLen;
	pBufer += nStrLen;

	pszValue[nStrLen] = '\0';
	strValue = pszValue;
	return 0;
}

// 从缓冲区字符串出，字符串先存放4个字节长度，再存放实际内容（不包含0结束符）
// nLeftBufLen 长度也会被改变
int PopInt32FromBuffer(char *&pBufer, int &nLeftBufLen, int &nValue)
{
	nValue = 0;
	if(nLeftBufLen < 4)
		return -1;

	memcpy(&nValue, pBufer, 4);
	nLeftBufLen -= 4;
	pBufer += 4;

	return 0;
}
