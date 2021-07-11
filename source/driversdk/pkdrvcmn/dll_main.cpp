/**************************************************************
*  Filename:    Server.cpp
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: ������.
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

#define PROCESSMGR_RUNTIME_DIR_NAME			ACE_TEXT("ProcessMgr") // ֪ͨ���ж�Ӧ�Ĺ����ڴ�����Ŀ¼ǰ׺
#define PROCESSMGR_CMDTYPE_REFRESH			1			// ֪ͨˢ����������
#define PROCESSMGR_CMDTYPE_STOP				2			// ֹͣ������������
#define SAFE_DELETE(x)						{ if (x) delete x; x = NULL; }
#define MAXBUFSIZE							1024
 
extern CPKProcessController *	g_pProcController;	// ���̿���������
extern CProcessQueue *			g_pQueFromProcMgr;				// �ͽ��̹���������ͨ�ŵĹ����ڴ����

extern bool		g_bDriverAlive;
extern ACE_DLL		g_hDrvDll;	// ������̬��
extern string		g_strDrvName;	// ������׺��������
extern string		g_strDrvExtName; // ��׺������ .pyo,.dll, .so, .pyc
extern string		g_strDrvPath; // �õ�û����չ����·������ /gateway/bin/drivers/modbus,����������.dll
extern int			g_nDrvLang; // ��������
bool				g_bWorkingThreadExit = false; // �Ƿ����й����̶߳��˳��ˣ����̺߳Ϳ���̨�����߳���δ�˳���SetConsoleCtrlHandler���̣߳�
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
	g_pQueFromProcMgr->Open(true, PROCESSQUEUE_PROCESSMGR_CONTROL_AREASIZE); // ���ﴴ������ʾ���̹�����������
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
	// ������־�ļ�
	g_logger.SetLogFileName(szDrvName);
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "drvMain called:%s, %s", szDrvName, szDrvPath);

	int nRet = 0;
    // �������ƾ���argv[0]
    g_strDrvName = szDrvName;

    g_strDrvPath = szDrvPath;
	// ��ȡ��������ļ�������Ϊ��������
	int nPos = string::npos;
	if ((nPos = g_strDrvName.find_last_of(ACE_DIRECTORY_SEPARATOR_CHAR_A)) != string::npos)
		g_strDrvName = g_strDrvName.substr(nPos + 1); // �õ� modbus.exe

	if ((nPos = g_strDrvName.find_last_of('.')) != string::npos)
		g_strDrvName = g_strDrvName.substr(0, nPos); // �õ� modbus
	// transform(g_strDrvName.begin(), g_strDrvName.end(), g_strDrvName.begin(), (int(*)(int))tolower); // linux�¶���Сд


	// C���Ե��õ�ʱ�򣬲�ȷ������
	g_nDrvLang = DetectDriverLanguage();
	if (g_nDrvLang == PK_LANG_UNKNOWN)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "û���ҵ�������,name=%s", g_strDrvName.c_str());
		WaitAndExit();
		return -1;
	}

    g_pProcController = new CPKProcessController((char *)g_strDrvName.c_str());
    // assure only one process is running
    if (g_pProcController->SetProcessAliveFlag() != PK_SUCCESS)
    {
		g_logger.LogMessage(PK_LOGLEVEL_CRITICAL, "����%s����ʧ��! �����Ѿ�����, ���˳�!", g_strDrvName.c_str());

        WaitAndExit();
        return -1;
    }

    InitProcMgrQues(); // ��ʼ����procmanagerͨ�ŵĶ���

    // ��¼��������״̬--ʱ��
    //char szTime[PK_HOSTNAMESTRING_MAXLEN] = {'\0'};
    //PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));

    //long lRet = InitLua();
    //if(lRet != 0)
    //{
    //	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "lua��ʼ��ʧ�ܣ�����:%d", lRet);
    //}

    long lRet = LoadDrvDllFuncs();
    if (lRet != PK_SUCCESS)
    {
		g_logger.LogMessage(PK_LOGLEVEL_CRITICAL, "fail to start driver: %s! load dll or python in path:%s failed ��return:%d��, will exit!!!!!!!!!", g_strDrvName.c_str(),
            g_strDrvPath.c_str(), lRet);

        WaitAndExit();
        return -1;
    }

#ifdef _WIN32
	// �޲�����3������ʱ, �����������
	if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE))
		ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T: Could not set control handler \n")));
#endif 

    //REMOTELOG_TASK->Start(); // ������־��¼����
    MAIN_TASK->Start();
    // ������ѭ����
    RunInteractiveLoop();
    // �û�ѡ���˳������˳�
    MAIN_TASK->Stop();
    //REMOTELOG_TASK->Stop(); // �ر���־��¼����

    if (g_pQueFromProcMgr)
        delete g_pQueFromProcMgr;
    g_pQueFromProcMgr = NULL;

    // ���������ķ���ʼ������
    if (g_nDrvLang == PK_LANG_CPP)
    {
        if (g_pfnUnInitDriver)
            g_pfnUnInitDriver(MAIN_TASK->m_driverInfo.m_pOutDriverInfo);
        // �ر�������̬��
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
        if (g_pProcController->KbHit()) // �յ�����1���ַ��ˡ�����յ��ַ��������������գ����û���յ��ַ�������ȴ�300ms���������
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
                MAIN_TASK->TriggerRefreshConfig();	// �ֹ�����һ����������
                break;

                // filter out ox0D0A, �س����в�����
			case 0x0a:
			case 0x0D:
				break;
            default:
                ACE_OS::printf(">>%d>>", chCmd); // ��linux�£������pkservermgr������sh�ű��е��ã�drivers/dbdrv/dbdrv & �����ں�̨���ᵼ��CPU�ܸߣ���Ϊ���ڴ�ӡ>>˵�������յ�һЩδ֪���ַ�
                break;
            }
            continue; // ����յ��ַ��������������գ�
        }

		// �������û���յ��κ��ַ���sleep 1000ms
        ACE_Time_Value tmWait(0, 1000000); // ÿ�ν���֮��ȴ�1000ms���ᵼ�½������ݱ�������������δ����pkservermgrʱ���������᲻�ϴ�ӡ������Ϣ
        ACE_OS::sleep(tmWait);

        // ���Դ����������������Ϣ
        if (g_pQueFromProcMgr)
        {
            char szRecBuffer[PROCESSQUEUE_PROCESSMGR_CONTROL_RECORD_LENGTH] = { 0 };
            unsigned int nRecLen = 0;
            bool bFoundRecord = false;
            // ��ȡ�����һ���������ݣ�ǰ��ļ�¼ȫ�����׵�
            int nResult = g_pQueFromProcMgr->DeQueue(szRecBuffer, sizeof(szRecBuffer), &nRecLen);
            if (nResult == PK_SUCCESS)
                bFoundRecord = true;

            // ����м�¼�����ȡ���һ����¼
            while (bFoundRecord)
            {
                nResult = g_pQueFromProcMgr->DeQueue(szRecBuffer, sizeof(szRecBuffer), &nRecLen);
                if (nResult != PK_SUCCESS)
                    break;
            }

            if (bFoundRecord)
            {
                // ǰ4���ֽڱ�ʾ��������
                int nType = 0;
                memcpy(&nType, szRecBuffer, sizeof(int));

                // ���ŵ�4���ֽڱ�ʾ����ʱ��
                time_t tmNow;
                memcpy(&tmNow, szRecBuffer + sizeof(int), sizeof(time_t));
                char szTime[PK_HOSTNAMESTRING_MAXLEN] = { 0 };
                ACE_OS::strftime(szTime, sizeof(szTime), "%Y-%m-%d %H:%M:%S", localtime(&tmNow));

                if (nType == PROCESSMGR_CMDTYPE_REFRESH)
                {
                    g_logger.LogMessage(PK_LOGLEVEL_WARN, "===Recv command: CONFIG REFRESH from pkservermgr, time:%s==", szTime);
                    MAIN_TASK->TriggerRefreshConfig();	// �ֹ�����һ����������
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

// ȷ�������Ŀ������ԣ���C����python�����ݺ�׺������
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

	// ���û������������ӣ�bin/drivers/drivername;bin;python/binĿ¼
	string strDriverDir = strCurDir + PK_OS_DIR_SEPARATOR + "drivers" + PK_OS_DIR_SEPARATOR + g_strDrvName;
	string strPythonDir = strCurDir + PK_OS_DIR_SEPARATOR + "python"; // ��python27.dll��������Ҫ�ģ�

	
	//int nRet = putenv(strEnv.c_str());
#ifdef _WIN32
	char *szPathEnv = getenv("PATH");
	if (szPathEnv)
	{
		string strSysPath = getenv("PATH");
		strSysPath = strDriverDir + ";" + strCurDir + ";" + strPythonDir + ";" + strSysPath;
		string strEnv = "PATH=";// c:\\windows";
		strEnv = strEnv + strSysPath;

		int nRet = putenv(strEnv.c_str()); // �����������Ч�ģ���֪���ǲ�����Ϊ����Ա��ԭ��GetLastError����0
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

		int nRet = setenv("LD_LIBRARY_PATH", strSysPath.c_str(), 1); // �����������Ч�ģ���֪���ǲ�����Ϊ����Ա��ԭ��GetLastError����0
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
        long nErr = g_hDrvDll.open(strDllPath.c_str()); // C++����
        if (nErr != PK_SUCCESS)
        {
#ifdef WIN32
            g_logger.LogMessage(PK_LOGLEVEL_CRITICAL, "���ܼ�������DLL %s, GetLastError()=:%d, WorkdingDir:%s", strDllPath.c_str(), GetLastError(), szWorkingDir);
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
    // ��Ϣ5���Ա��û��ܿ�����ʾ
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

		// �ȴ��˳�
		time_t tmBegin = time(NULL);
		while (!g_bWorkingThreadExit)
		{
			PKComm::Sleep(300);
			time_t tmNow = time(NULL);
			if (labs(tmNow - tmBegin) > 10) // ���ȴ�10��
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
