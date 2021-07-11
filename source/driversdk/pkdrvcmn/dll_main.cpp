/**************************************************************
*  Filename:    Server.cpp
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: 主程序.
*
*  @author:     shijunpu
*  @version     08/11/2016  shijunpu  Initial Version
**************************************************************/
#include <ace/Process_Mutex.h>
#include <ace/OS_NS_unistd.h>
#include <ace/OS.h>
#include <iostream>
#include "MainTask.h"
#include "pkdriver/pkdrvcmn.h"
#include "drvframework.h"
#include "MainTask.h"
#include "ace/Signal.h"
#include "pklog/pklog.h"
#include "pkcomm/pkcomm.h"
#include "errcode/error_code.h"
#include "shmqueue/ProcessQueue.h"
#include "common/eviewcomm_internal.h"
#include "common/RMAPI.h"
#include "pkprocesscontroller/pkprocesscontroller.h"
#include "PythonHelper.h"
#include "stdlib.h"

#include "pkcrashdump/pkcrashdump.h"

extern CPKLog g_logger;

#define LH_DEBUG(X)		g_logger.LogMessage((PK_LOGLEVEL_DEBUG,X))
#define LH_ERROR(X)		g_logger.LogMessage((PK_LOGLEVEL_ERROR,X))
#define LH_INFO(X)		g_logger.LogMessage((PK_LOGLEVEL_INFO,X))
#define LH_WARN(X)		g_logger.LogMessage((PK_LOGLEVEL_WARN,X))
#define LH_CRITICAL(X)	g_logger.LogMessage((PK_LOGLEVEL_CRITICAL,X))

using namespace std;

#define PROCESSMGR_RUNTIME_DIR_NAME			ACE_TEXT("ProcessMgr") // 通知队列对应的共享内存的相对目录前缀
#define PROCESSMGR_CMDTYPE_REFRESH			1			// 通知刷新命令类型
#define PROCESSMGR_CMDTYPE_STOP				2			// 停止运行命令类型
#define SAFE_DELETE(x)						{ if (x) delete x; x = NULL; }
#define MAXBUFSIZE							1024
 
extern CPKProcessController *	g_pProcController;	// 进程控制器辅助
extern CProcessQueue *			g_pQueFromProcMgr;				// 和进程管理器进行通信的共享内存队列

extern bool		g_bDriverAlive;
extern ACE_DLL		g_hDrvDll;	// 驱动动态库
extern string		g_strDrvName;	// 不带后缀名的驱动
extern string		g_strDrvExtName; // 后缀名，如 .pyo,.dll, .so, .pyc
extern string		g_strDrvPath; // 得到没有扩展名的路径，如 /gateway/bin/drivers/modbus,不包含驱动.dll
extern int			g_nDrvLang; // 驱动语言
bool				g_bWorkingThreadExit = false; // 是否所有工作线程都退出了？主线程和控制台控制线程尚未退出（SetConsoleCtrlHandler的线程）
bool IsActiveHost()
{
    int lStatus = 0;
    GetRMStatus(&lStatus);
    return lStatus != 0;
}

void WaitAndExit();
int DetectDriverLanguage();
long LoadDrvDllFuncs();
void RunInteractiveLoop();

extern long InitLua();
extern long UnInitLua();
#ifdef _WIN32
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType);
#endif

int InitProcMgrQues()
{
    int lRet = PK_SUCCESS;
    string strRunTimePath = PKComm::GetRunTimeDataPath();

    string strQueMgrPath = strRunTimePath + ACE_DIRECTORY_SEPARATOR_STR + PROCESSMGR_RUNTIME_DIR_NAME;
    PKFileHelper::CreateDir(strQueMgrPath.c_str());
    g_pQueFromProcMgr = new CProcessQueue((char *)g_strDrvName.c_str(), (char *)strQueMgrPath.c_str());
	g_pQueFromProcMgr->Open(true, PROCESSQUEUE_PROCESSMGR_CONTROL_AREASIZE); // 这里创建，表示进程管理器创建！
    return 0;
}

int unInitProcQues()
{
    if (g_pQueFromProcMgr)
        delete g_pQueFromProcMgr;
    g_pQueFromProcMgr = NULL;
    return 0;
}

PKDRIVER_EXPORTS int drvMain(char *szDrvName, char *szDrvPath, int nDrvLang)
{
	// 设置日志文件
	g_logger.SetLogFileName(szDrvName);
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "drvMain called:%s, %s", szDrvName, szDrvPath);

	int nRet = 0;
    // 驱动名称就是argv[0]
    g_strDrvName = szDrvName;

    g_strDrvPath = szDrvPath;
	// 截取到纯粹的文件名称作为驱动名称
	int nPos = string::npos;
	if ((nPos = g_strDrvName.find_last_of(ACE_DIRECTORY_SEPARATOR_CHAR_A)) != string::npos)
		g_strDrvName = g_strDrvName.substr(nPos + 1); // 得到 modbus.exe

	if ((nPos = g_strDrvName.find_last_of('.')) != string::npos)
		g_strDrvName = g_strDrvName.substr(0, nPos); // 得到 modbus
	// transform(g_strDrvName.begin(), g_strDrvName.end(), g_strDrvName.begin(), (int(*)(int))tolower); // linux下都用小写


	// C语言调用的时候，不确定语言
	g_nDrvLang = DetectDriverLanguage();
	if (g_nDrvLang == PK_LANG_UNKNOWN)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "没有找到驱动库,name=%s", g_strDrvName.c_str());
		WaitAndExit();
		return -1;
	}

    g_pProcController = new CPKProcessController((char *)g_strDrvName.c_str());
    // assure only one process is running
    if (g_pProcController->SetProcessAliveFlag() != PK_SUCCESS)
    {
		g_logger.LogMessage(PK_LOGLEVEL_CRITICAL, "驱动%s启动失败! 进程已经存在, 将退出!", g_strDrvName.c_str());

        WaitAndExit();
        return -1;
    }

    InitProcMgrQues(); // 初始化和procmanager通信的队列

    // 记录驱动启动状态--时间
    //char szTime[PK_HOSTNAMESTRING_MAXLEN] = {'\0'};
    //PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));

    //long lRet = InitLua();
    //if(lRet != 0)
    //{
    //	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "lua初始化失败，返回:%d", lRet);
    //}

    long lRet = LoadDrvDllFuncs();
    if (lRet != PK_SUCCESS)
    {
		g_logger.LogMessage(PK_LOGLEVEL_CRITICAL, "fail to start driver: %s! load dll or python in path:%s failed （return:%d）, will exit!!!!!!!!!", g_strDrvName.c_str(),
            g_strDrvPath.c_str(), lRet);

        WaitAndExit();
        return -1;
    }

#ifdef _WIN32
	// 无参数或3个参数时, 继续向后面走
	if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE))
		ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T: Could not set control handler \n")));
#endif 

    //REMOTELOG_TASK->Start(); // 开启日志记录任务
    MAIN_TASK->Start();
    // 进入主循环体
    RunInteractiveLoop();
    // 用户选择退出程序退出
    MAIN_TASK->Stop();
    //REMOTELOG_TASK->Stop(); // 关闭日志记录任务

    if (g_pQueFromProcMgr)
        delete g_pQueFromProcMgr;
    g_pQueFromProcMgr = NULL;

    // 调用驱动的反初始化函数
    if (g_nDrvLang == PK_LANG_CPP)
    {
        if (g_pfnUnInitDriver)
            g_pfnUnInitDriver(MAIN_TASK->m_driverInfo.m_pOutDriverInfo);
        // 关闭驱动动态库
        g_hDrvDll.close();
    }

	Py_UnInitPython();

    //UnInitLua();


    unInitProcQues();
    g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "driver %s main exit...", g_strDrvName.c_str());
	g_bWorkingThreadExit = true;
    return 0;
}

void RunInteractiveLoop()
{
    // whislter to quit
    ACE_OS::printf(">>");

    while (g_bDriverAlive)
    {
        if (g_pProcController->KbHit()) // 收到大于1个字符了。如果收到字符输入则会继续接收；如果没有收到字符输入则等待300ms后接收命令
        {
            char chCmd = tolower(getchar());
            switch (chCmd)
            {
            case 'q':
            case 'Q':
                g_bDriverAlive = false;
                break;
            case 'r':
            case 'R':
                MAIN_TASK->TriggerRefreshConfig();	// 手工触发一次在线配置
                break;

                // filter out ox0D0A, 回车换行不处理
			case 0x0a:
			case 0x0D:
				break;
            default:
                ACE_OS::printf(">>%d>>", chCmd); // 在linux下，如果用pkservermgr，或者sh脚本中调用：drivers/dbdrv/dbdrv & 启动在后台，会导致CPU很高，因为总在打印>>说明总是收到一些未知的字符
                break;
            }
            continue; // 如果收到字符输入则会继续接收；
        }

		// 如果该轮没有收到任何字符，sleep 1000ms
        ACE_Time_Value tmWait(0, 1000000); // 每次接收之间等待1000ms，会导致接收数据变慢，但可以在未开启pkservermgr时，驱动不会不断打印错误信息
        ACE_OS::sleep(tmWait);

        // 尝试从任务管理器接收消息
        if (g_pQueFromProcMgr)
        {
            char szRecBuffer[PROCESSQUEUE_PROCESSMGR_CONTROL_RECORD_LENGTH] = { 0 };
            unsigned int nRecLen = 0;
            bool bFoundRecord = false;
            // 获取到最后一个队列内容，前面的记录全部都抛掉
            int nResult = g_pQueFromProcMgr->DeQueue(szRecBuffer, sizeof(szRecBuffer), &nRecLen);
            if (nResult == PK_SUCCESS)
                bFoundRecord = true;

            // 如果有记录则继续取最后一条记录
            while (bFoundRecord)
            {
                nResult = g_pQueFromProcMgr->DeQueue(szRecBuffer, sizeof(szRecBuffer), &nRecLen);
                if (nResult != PK_SUCCESS)
                    break;
            }

            if (bFoundRecord)
            {
                // 前4个字节表示命令类型
                int nType = 0;
                memcpy(&nType, szRecBuffer, sizeof(int));

                // 接着的4个字节表示命令时间
                time_t tmNow;
                memcpy(&tmNow, szRecBuffer + sizeof(int), sizeof(time_t));
                char szTime[PK_HOSTNAMESTRING_MAXLEN] = { 0 };
                ACE_OS::strftime(szTime, sizeof(szTime), "%Y-%m-%d %H:%M:%S", localtime(&tmNow));

                if (nType == PROCESSMGR_CMDTYPE_REFRESH)
                {
                    g_logger.LogMessage(PK_LOGLEVEL_WARN, "===Recv command: CONFIG REFRESH from pkservermgr, time:%s==", szTime);
                    MAIN_TASK->TriggerRefreshConfig();	// 手工触发一次在线配置
                }
                else if (nType == PROCESSMGR_CMDTYPE_STOP)
                {
                    g_logger.LogMessage(PK_LOGLEVEL_WARN, "===Recv command: STOP from pkservermgr, time:%s==", szTime);
                    g_bDriverAlive = false;
                }
                else
                {
                    g_logger.LogMessage(PK_LOGLEVEL_ERROR, "===Recv command: %d, NOT STOP OR REFRESH, INVALID! time:%s", szTime);
                }
            }
        } // if(g_pProcQueMgr)
    } // while(g_bDriverAlive)
}

// 确定驱动的开发语言，是C还是python，根据后缀名区分
int DetectDriverLanguage()
{
	if (IsPythonDriver())
		g_nDrvLang = PK_LANG_PYTHON;
	else
		g_nDrvLang = PK_LANG_CPP;

	return g_nDrvLang;
}

long LoadDrvDllFuncs()
{
    //char szDrvPath[MAXBUFSIZE] = {0};
    //GetDriverPath(szDrvPath, sizeof(szDrvPath)); // /peak/bin/drivers/modbus
    const char *szDrvPath = g_strDrvPath.c_str();
	string strDirPath = g_strDrvPath;
	string strDllPath = g_strDrvPath;
    strDllPath += ACE_DIRECTORY_SEPARATOR_CHAR_A;
    strDllPath += g_strDrvName; // drivers/modbus/modbus

    string strCurDir = PKComm::GetBinPath();
    PKFileHelper::SetWorkingDirectory(strCurDir.c_str());
    char szWorkingDir[1024] = { 0 };
    PKFileHelper::GetWorkingDirectory(szWorkingDir, sizeof(szWorkingDir));

	// 设置环境变量，增加：bin/drivers/drivername;bin;python/bin目录
	string strDriverDir = strCurDir + PK_OS_DIR_SEPARATOR + "drivers" + PK_OS_DIR_SEPARATOR + g_strDrvName;
	string strPythonDir = strCurDir + PK_OS_DIR_SEPARATOR + "python"; // 有python27.dll是驱动需要的！

	
	//int nRet = putenv(strEnv.c_str());
#ifdef _WIN32
	char *szPathEnv = getenv("PATH");
	if (szPathEnv)
	{
		string strSysPath = getenv("PATH");
		strSysPath = strDriverDir + ";" + strCurDir + ";" + strPythonDir + ";" + strSysPath;
		string strEnv = "PATH=";// c:\\windows";
		strEnv = strEnv + strSysPath;

		int nRet = putenv(strEnv.c_str()); // 这个设置是无效的，不知道是不是因为管理员的原因。GetLastError返回0
		if (nRet != 0)
		{
			g_logger.LogMessage(PK_LOGLEVEL_CRITICAL, "Set Enviroment Variable path failed, ret:%d, %s", nRet, strEnv.c_str());
		}
	}
#else
	char *szLibPath = getenv("LD_LIBRARY_PATH"); // DO NOT EXIST OF THIS LD_LIBRARY_PATH, return NULL!
	if (szLibPath)
	{
		string strSysPath = szLibPath;
		strSysPath = strDriverDir + ":" + strCurDir + ":" + strPythonDir + ":" + strSysPath;

		int nRet = setenv("LD_LIBRARY_PATH", strSysPath.c_str(), 1); // 这个设置是无效的，不知道是不是因为管理员的原因。GetLastError返回0
		if (nRet != 0)
		{
			g_logger.LogMessage(PK_LOGLEVEL_CRITICAL, "Set Enviroment Variable LD_LIBRARY_PATH failed, ret:%d, %s", nRet, strSysPath.c_str());
		}
	}
#endif

    if (g_nDrvLang == PK_LANG_PYTHON)
	{
		int nRet = Py_InitPython();
		if (nRet != 0)
			return nRet;
	}
	else
    {
        long nErr = g_hDrvDll.open(strDllPath.c_str()); // C++语言
        if (nErr != PK_SUCCESS)
        {
#ifdef WIN32
            g_logger.LogMessage(PK_LOGLEVEL_CRITICAL, "不能加载驱动DLL %s, GetLastError()=:%d, WorkdingDir:%s", strDllPath.c_str(), GetLastError(), szWorkingDir);
#else
            g_logger.LogMessage(PK_LOGLEVEL_CRITICAL, "can not load DLL %s, WorkdingDir:%s", strDllPath.c_str(), szWorkingDir);
#endif

            return EC_ICV_DA_IO_DLL_NOT_FOUND;
        }

        g_pfnInitDriver = (PFN_InitDriver)g_hDrvDll.symbol("InitDriver");
        g_pfnUnInitDriver = (PFN_UnInitDriver)g_hDrvDll.symbol("UnInitDriver");
        g_pfnInitDevice = (PFN_InitDevice)g_hDrvDll.symbol("InitDevice");
        g_pfnUnInitDevice = (PFN_UnInitDevice)g_hDrvDll.symbol("UnInitDevice");
        g_pfnOnDeviceConnStateChanged = (PFN_OnDeviceConnStateChanged)g_hDrvDll.symbol("OnDeviceConnStateChanged");
        g_pfnOnTimer = (PFN_OnTimer)g_hDrvDll.symbol("OnTimer");
        if (!g_pfnOnTimer)
            g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "driver %s must implement OnTimer functions!!!!!!", strDllPath.c_str());
        g_pfnOnControl = (PFN_OnControl)g_hDrvDll.symbol("OnControl");
        g_pfnGetVersion = (PFN_GetVersion)g_hDrvDll.symbol("GetVersion");
        if (g_pfnInitDevice == NULL)
        {
            g_logger.LogMessage(PK_LOGLEVEL_CRITICAL, "driver %s must implement InitDevice function: %s!!!!!!!", strDllPath.c_str(), "InitDevice");
            return EC_ICV_DA_IO_DLL_NOT_FOUND;
        }
    }
    return PK_SUCCESS;
}

void WaitAndExit()
{
    //CDriver::UpdateDriverStatus(&m_redisRW, -1,"WaitAndExit",NULL);
    // 休息5秒以便用户能看到提示
	PKComm::Sleep(5000);
}


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
	{
		//SignalToExit();
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "recv closewindow action, Driver:%s will stop......", g_strDrvName.c_str());
		g_bDriverAlive = false;

		// 等待退出
		time_t tmBegin = time(NULL);
		while (!g_bWorkingThreadExit)
		{
			PKComm::Sleep(300);
			time_t tmNow = time(NULL);
			if (labs(tmNow - tmBegin) > 10) // 最多等待10秒
				break;
		}
		return true;
	}
		// Pass other signals to the next handler. 
	case CTRL_BREAK_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
	default:
		return false;
	}
}
#endif
