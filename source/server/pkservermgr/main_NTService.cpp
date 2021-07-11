#include <string>
using namespace std;

#define		PKSERVERMGR_SERVICE_NAME	"pkservermgr"
string		g_strServiceName = PKSERVERMGR_SERVICE_NAME;
extern bool			g_bAlive;
extern bool			g_bRestart;

#ifdef _WIN32

#include <stdio.h>
#include <windows.h>
#include <winbase.h>
#include <winsvc.h>
#include <process.h>
#include <string>
#include <tchar.h>
#include "time.h"
#include <vector>
#include <map>
#include <string>
#include "pklog/pklog.h"
extern CPKLog PKLog;

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType);

// NT Service 需要的
SERVICE_STATUS          g_serviceStatus; 
SERVICE_STATUS_HANDLE   g_hServiceStatusHandle; 
CRITICAL_SECTION myCS;

bool DirExist(const TCHAR *pszDirName);
bool CreateDir(const TCHAR *pszDirName);
VOID WINAPI ServiceControlMain( DWORD dwArgc, LPTSTR *lpszArgv );
VOID WINAPI ServiceControlHandler( DWORD fdwControl );

SERVICE_TABLE_ENTRY   DispatchTable[] = 
{ 
	{(char *)g_strServiceName.c_str(), ServiceControlMain}, 
	{NULL, NULL}
};
#define		PKSERVERMGR_EXE_NAME		"pkservermgr"

BOOL KillService(char* szServiceName) 
{ 
	SC_HANDLE schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS); 
	if (schSCManager==0) 
	{
		long nError = GetLastError();
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "KillService..., OpenSCManager failed, error code = %d", nError);
	}
	else
	{
		SC_HANDLE schService = OpenService( schSCManager, szServiceName, SERVICE_ALL_ACCESS);
		if (schService==0) 
		{
			long nError = GetLastError();
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "KillService..., OpenService failed, error code = %d", nError);
		}
		else
		{
			SERVICE_STATUS status;
			if(ControlService(schService,SERVICE_CONTROL_STOP,&status))
			{
				CloseServiceHandle(schService); 
				CloseServiceHandle(schSCManager); 
				PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "KillService..., successfully");
				return TRUE;
			}
			else
			{
				long nError = GetLastError();
				PKLog.LogMessage(PK_LOGLEVEL_ERROR, "KillService..., ControlService failed, error code = %d", nError);
			}
			CloseServiceHandle(schService); 
		}
		CloseServiceHandle(schSCManager); 
	}
	return FALSE;
}

BOOL RunService(char* szServiceName, int nArg, char** pArg) 
{ 
	SC_HANDLE schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS); 
	if (schSCManager==0) 
	{
		long nError = GetLastError();
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "RunService..., OpenSCManager failed, error code = %d", nError);
	}
	else
	{
		SC_HANDLE schService = OpenService( schSCManager, szServiceName, SERVICE_ALL_ACCESS);
		if (schService==0) 
		{
			long nError = GetLastError();
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "RunService..., OpenService failed, error code = %d", nError);
			CloseServiceHandle(schSCManager); 
			return FALSE;
		}
		

		if(StartService(schService,nArg,(const char**)pArg))
		{
			CloseServiceHandle(schService); 
			CloseServiceHandle(schSCManager); 
			PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "RunService..., successfully");
			return TRUE;
		}

		long nError = GetLastError();
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "RunService..., StartService failed, error code = %d", nError);

		CloseServiceHandle(schService); 
		CloseServiceHandle(schSCManager); 
	}
	return FALSE;
}


////////////////////////////////////////////////////////////////////// 
//
// This routine gets used to start your service
//
VOID WINAPI ServiceControlMain( DWORD dwArgc, LPTSTR *lpszArgv )
{
	PKLog.SetLogFileName(PKSERVERMGR_EXE_NAME);

	//MessageBox(NULL, "ServiceControlMain", "ss",MB_OK);
	PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "enter ServiceControlMain...");
	char szTip[1024] = {0};
	//WriteLog("service started");
	//MessageBox(NULL, "aa", "aa",MB_OK);
	DWORD   status = 0; 
	DWORD   specificError = 0xfffffff; 

	g_serviceStatus.dwServiceType        = SERVICE_WIN32; 
	g_serviceStatus.dwCurrentState       = SERVICE_START_PENDING; 
	g_serviceStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_PAUSE_CONTINUE; 
	g_serviceStatus.dwWin32ExitCode      = 0; 
	g_serviceStatus.dwServiceSpecificExitCode = 0; 
	g_serviceStatus.dwCheckPoint         = 0; 
	g_serviceStatus.dwWaitHint           = 0; 

	g_hServiceStatusHandle = RegisterServiceCtrlHandler(g_strServiceName.c_str(), ServiceControlHandler); 
	if (g_hServiceStatusHandle==0) 
	{
		long nError = GetLastError();
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "ServiceControlMain::RegisterServiceCtrlHandler failed, error code = %d", nError);
		return; 
	} 
	PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "ServiceControlMain::RegisterServiceCtrlHandler success");
	// Handle error condition 
	/*
	status = GetLastError(); 
	if (status!=NO_ERROR) 
	{ 
		g_serviceStatus.dwCurrentState       = SERVICE_STOPPED; 
		g_serviceStatus.dwCheckPoint         = 0; 
		g_serviceStatus.dwWaitHint           = 0; 
		g_serviceStatus.dwWin32ExitCode      = status; 
		g_serviceStatus.dwServiceSpecificExitCode = specificError; 
		SetServiceStatus(g_hServiceStatusHandle, &g_serviceStatus); 
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "RegisterServiceCtrlHandler(%s) status(%d)!=NO_ERROR", g_strServiceName.c_str(), status);
		return; 
	} 
	*/

	// Initialization complete - report running status 
	g_serviceStatus.dwCurrentState       = SERVICE_RUNNING; 
	g_serviceStatus.dwCheckPoint         = 0; 
	g_serviceStatus.dwWaitHint           = 0;  
	if(!SetServiceStatus(g_hServiceStatusHandle, &g_serviceStatus)) 
	{ 
		long nError = GetLastError();
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "ServiceControlMain::SetServiceStatus(RUNNING) failed, error code = %d\n", nError);
	} 
	else
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "ServiceControlMain::SetServiceStatus(RUNNING) success, leave ServiceControlMain");
}

////////////////////////////////////////////////////////////////////// 
//
// This routine responds to events concerning your service, like start/stop
//
VOID WINAPI ServiceControlHandler(DWORD fdwControl)
{
	bool bNeedSetStatus = false;
	switch(fdwControl) 
	{
	case SERVICE_CONTROL_STOP:
	case SERVICE_CONTROL_SHUTDOWN:
		g_serviceStatus.dwWin32ExitCode = 0; 
		g_serviceStatus.dwCurrentState  = SERVICE_STOPPED; 
		g_serviceStatus.dwCheckPoint    = 0; 
		g_serviceStatus.dwWaitHint      = 0;
		{
			g_bAlive = false;
			g_bRestart = false;
		}
		bNeedSetStatus = true;
		break; 
	case SERVICE_CONTROL_PAUSE:
		g_serviceStatus.dwCurrentState = SERVICE_PAUSED; 
		bNeedSetStatus = true;
		break;
	case SERVICE_CONTROL_CONTINUE:
		g_serviceStatus.dwCurrentState = SERVICE_RUNNING; 
		bNeedSetStatus = true;
		break;
	case SERVICE_CONTROL_INTERROGATE:
		break;
	default:  // 可能是重启？
		g_bAlive = false;
		g_bRestart = true;
	}

	if(bNeedSetStatus)
	{
		if (!SetServiceStatus(g_hServiceStatusHandle,  &g_serviceStatus)) 
		{ 
			long nError = GetLastError();
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "ServiceControlHandler::SetServiceStatus(%d),control=%d failed, error code = %d", g_serviceStatus.dwCurrentState, fdwControl, nError);
		} 
		else
			PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "ServiceControlHandler::SetServiceStatus(%d),control=%d success", g_serviceStatus.dwCurrentState, fdwControl);
	}
	else
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "ServiceControlHandler::Unknown control(%d), do nothing", fdwControl);
}

BOOL IsServiceInstalled()
{
	SC_HANDLE schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS); 
	if (schSCManager==0) 
	{
		long nError = GetLastError();
		return TRUE;
	}
	
	SC_HANDLE schService = OpenService( schSCManager, g_strServiceName.c_str(), SERVICE_ALL_ACCESS);
	if (schService==0) 
	{
		long nError = GetLastError();
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "OpenService failed, error code = %d\n", nError);
		CloseServiceHandle(schSCManager);	
		return FALSE;
	}

	CloseServiceHandle(schService); 
	CloseServiceHandle(schSCManager);	
	return TRUE;

}
////////////////////////////////////////////////////////////////////// 
//
// Uninstall
//
VOID UnInstall(char* szServiceName)
{
	printf("uninstall %s...\n", g_strServiceName.c_str());
	SC_HANDLE schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS); 
	if (schSCManager==0) 
	{
		long nError = GetLastError();
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "OpenSCManager failed, error code = %d\n", nError);
	}
	else
	{
		SC_HANDLE schService = OpenService( schSCManager, szServiceName, SERVICE_ALL_ACCESS);
		if (schService==0) 
		{
			long nError = GetLastError();
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "OpenService failed, error code = %d\n", nError);
		}
		else
		{
			if(!DeleteService(schService)) 
			{
				PKLog.LogMessage(PK_LOGLEVEL_ERROR, "Failed to delete service %s\n", szServiceName);
			}
			else 
			{
				PKLog.LogMessage(PK_LOGLEVEL_ERROR, "Service %s removed",szServiceName);
			}
			CloseServiceHandle(schService); 
		}
		CloseServiceHandle(schSCManager);	
	}
	PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "uninstall %s, success!", g_strServiceName.c_str());
}

////////////////////////////////////////////////////////////////////// 
//
// Install
//
VOID Install(char* szExePath, char* szServiceName) 
{  
	printf("install service %s with exepath(%s)...\n", g_strServiceName.c_str(), szExePath);

	char szPathWithParam[1024] = {0};
	sprintf(szPathWithParam, "%s runas ntservice", szExePath);
	SC_HANDLE schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS); 
	if (schSCManager==0) 
	{
		long nError = GetLastError();
		PKLog.LogMessage(PK_LOGLEVEL_ERROR,"OpenSCManager failed, error code = %d", nError);
	}
	else
	{
		SC_HANDLE schService = CreateService
			( 
			schSCManager,	/* SCManager database      */ 
			szServiceName,			/* name of service         */ 
			szServiceName,			/* service name to display */ 
			SERVICE_ALL_ACCESS,        /* desired access          */ 
			SERVICE_WIN32_OWN_PROCESS,//|SERVICE_INTERACTIVE_PROCESS , /* service type            */ 
			SERVICE_AUTO_START,      /* start type              */ 
			SERVICE_ERROR_NORMAL,      /* error control type      */ 
			szPathWithParam,			/* service's binary        */ 
			NULL,                      /* no load ordering group  */ 
			NULL,                      /* no tag identifier       */ 
			NULL,                      /* no dependencies         */ 
			NULL,                      /* LocalSystem account     */ 
			NULL
			);                     /* no password             */ 
		if (schService==0) 
		{
			long nError =  GetLastError();
			PKLog.LogMessage(PK_LOGLEVEL_ERROR,"Failed to create service %s, error code = %d\n", szServiceName, nError);
		}
		else
		{
			//设置服务描述
			SERVICE_DESCRIPTION   ServiceDesc;   
			static   TCHAR   szDescription[MAX_PATH];   

			_tcscpy(szDescription,   _T("peak eView service control"));   

			ServiceDesc.lpDescription   =   szDescription;   

			if(!(::ChangeServiceConfig2(schService,   SERVICE_CONFIG_DESCRIPTION,   &ServiceDesc)))
			{
				PKLog.LogMessage(PK_LOGLEVEL_ERROR,  "Service %s set service description failed", szServiceName);
			}

			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "Service %s installed", szServiceName);
			CloseServiceHandle(schService); 
			//printf("Service %s installed\n", szServiceName);
		}
		CloseServiceHandle(schSCManager);
	}	
	PKLog.LogMessage(PK_LOGLEVEL_NOTICE,  "install service %s with exepath(%s) success!", g_strServiceName.c_str(), szPathWithParam);
	//printf("install service %s with exepath(%s) success!\n", g_strServiceName.c_str(), szExePath);
}


int ProcessManualCmd(int argc, char* argvs[])
{
	if (argc != 2)
		return -1;

	char szExeFilePath[MAX_PATH] = {0};
	DWORD dwSize = GetModuleFileName(NULL,szExeFilePath, sizeof(szExeFilePath)); 
		
	char *szParam1 = argvs[1];
	if (_stricmp("uninstall", szParam1) == 0 || _stricmp("-u", szParam1) == 0 || _stricmp("/u", szParam1) == 0 || _stricmp("-uninstall", szParam1) == 0 || _stricmp("/uninstall", szParam1) == 0)
	{
		UnInstall((char *)g_strServiceName.c_str());
	}
	else if (_stricmp("install", szParam1) == 0 || _stricmp("-i", szParam1) == 0 || _stricmp("/i", szParam1) == 0 || _stricmp("-install", szParam1) == 0 || _stricmp("/install", szParam1) == 0)
	{			
		if(IsServiceInstalled()) // 如果已经安装了，则删除之
			UnInstall((char *)g_strServiceName.c_str());

		Install((char *)szExeFilePath, (char *)g_strServiceName.c_str());
	}
	else if (_stricmp("restart", szParam1) == 0 || _stricmp("-restart", szParam1) == 0 || _stricmp("-r", szParam1) == 0 || _stricmp("/restart", szParam1) == 0 || _stricmp("/r", szParam1) == 0)
	{			
		KillService((char *)g_strServiceName.c_str());
		RunService((char *)g_strServiceName.c_str(),0,NULL);
	}
	else if (_stricmp("start", szParam1) == 0 || _stricmp("-s", szParam1) == 0 || _stricmp("-start", szParam1) == 0 || _stricmp("/s", szParam1) == 0 || _stricmp("/start", szParam1) == 0)
	{	
		RunService((char *)g_strServiceName.c_str(),0,NULL);
	}
	else if (_stricmp("kill", szParam1) == 0 || _stricmp("-k", szParam1) == 0 || _stricmp("/k", szParam1) == 0 || _stricmp("-kill", szParam1) == 0 || _stricmp("/kill", szParam1) == 0
		|| _stricmp("stop", szParam1) == 0 || _stricmp("-stop", szParam1) == 0 || _stricmp("/stop", szParam1) == 0 || _stricmp("-stop", szParam1) == 0 || _stricmp("/kill", szParam1) == 0)
	{
		if(KillService((char *)g_strServiceName.c_str()))
		{
			PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "Stop service %s successfully.", g_strServiceName.c_str());
		}
		else
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "Failed to stop service %s.", g_strServiceName.c_str());
		}
	}
	else
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "%s, unknown command: %s.", g_strServiceName.c_str(), szParam1);
	
	//MessageBox(NULL, "ServiceControlMain", "bb",MB_OK);
	return 0;
}

int StartServiceByManager()
{
	// 启动NT服务。通过服务启动该函数后，该函数将会在DispatchTable表中所有的服务都停止后，才退出！！！所以这应该是最后一个函数
	if(!StartServiceCtrlDispatcher(DispatchTable))
	{
		long nError = GetLastError();
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "StartServiceByManager::StartServiceCtrlDispatcher failed, error code = %d\n", nError);
	}
	else
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "StartServiceByManager::StartServiceCtrlDispatcher return success,  means exe will exit now...");

	return 0;
}

#endif
