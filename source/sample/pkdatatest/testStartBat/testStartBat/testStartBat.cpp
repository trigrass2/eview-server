// testStartBat.cpp : 定义控制台应用程序的入口点。
//

//#include "stdafx.h"
#include<iostream>
#include<stdlib.h>
#include<windows.h>

int ShellProcess() 
{ 
	char m_szWorkingDirAbsPath[256];
	strcpy(m_szWorkingDirAbsPath, "d:\\Work\\eview\\web\\");

	char m_szStartCmdAbsPath[256];
	strcpy(m_szStartCmdAbsPath, m_szWorkingDirAbsPath);
	strcat(m_szStartCmdAbsPath, "startWeb.bat");
	
	HINSTANCE hInst = ShellExecute(NULL, "open",m_szStartCmdAbsPath,"",m_szWorkingDirAbsPath,SW_SHOWMINIMIZED );

	if(hInst != NULL)
	{
		printf("启动进程成功: %s [pid = %d]", m_szStartCmdAbsPath, hInst);
		return (int)hInst;
	}
	else
	{
		long nError = GetLastError();
		printf("启动进程失败: '%s', 错误码: %d", m_szStartCmdAbsPath, nError);
		return -1;
	}
}

int StartBatchProcess() 
{ 
	char m_szWorkingDirAbsPath[256];
	strcpy(m_szWorkingDirAbsPath, "d:\\Work\\eview\\web\\");

	char m_szStartCmdAbsPath[256];
	strcpy(m_szStartCmdAbsPath, m_szWorkingDirAbsPath);
	strcat(m_szStartCmdAbsPath, "startWeb.bat");
	STARTUPINFO startUpInfo = { sizeof(STARTUPINFO),NULL,NULL,NULL,0,0,0,0,0,0,0,STARTF_USESHOWWINDOW,0,0,NULL,0,0,0};  

	if(true)
	{
		startUpInfo.wShowWindow = SW_HIDE;
		startUpInfo.lpDesktop = NULL;
	}
	else
	{
		startUpInfo.wShowWindow = SW_HIDE;
		startUpInfo.lpDesktop = NULL;
	}

	PROCESS_INFORMATION processInfo;
	processInfo.hThread = 0;
	processInfo.hProcess = 0;
	char *pWorkdingDir = NULL;
	if(strlen(m_szWorkingDirAbsPath) > 0)
		pWorkdingDir = m_szWorkingDirAbsPath;
	if(CreateProcess(NULL,m_szStartCmdAbsPath,NULL,NULL,TRUE,NORMAL_PRIORITY_CLASS,NULL, pWorkdingDir,&startUpInfo,&processInfo))
	{
		printf("启动进程成功: %s [pid = %d]", m_szStartCmdAbsPath, processInfo.dwProcessId);
		return processInfo.dwProcessId;
	}
	else
	{
		long nError = GetLastError();
		printf("启动进程失败: '%s', 错误码: %d", m_szStartCmdAbsPath, nError);
		return -1;
	}
}


int main(int argc, char* argv[])
{
	printf("begin\n");

	StartBatchProcess();
	// ShellProcess();
	printf("end\n");
	getchar();
	return 0;
}

