/**************************************************************
*  Filename:    Server.cpp
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: ÿ��������ͨ���ÿǼ���executableĿ¼�µ�cvdrivercommon.dll,ͨ��cvdrivercommon.dll��������dll����.
*
*  @author:     xingxing
*  @version     07/26/2017  xingxing  Initial Version
**************************************************************/

#ifdef WIN32
#include <windows.h>
#else
// Ϊ�˱���windows������ACE.dll��windows�²���
#include "ace/ACE.h"
#include "ace/DLL.h"
#include "ace/Time_Value.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string>

using namespace std;

#define LOAD_LIBRARY_FAILED               -1
#define EXPORT_SYMBOL_FROM_LIBRARY_FAILED -2
#define MAX_FILE_PATH		1024
#define CVDRIVER_COMMON_DLL_NAME		"pkdrvcmn"

#ifdef WIN32
#define OS_DIR_SEP_STR		"\\"
#define OS_DIR_SEP_CHAR		'\\'
#else
#define OS_DIR_SEP_STR		"/"
#define OS_DIR_SEP_CHAR		'/'
#endif

extern "C" int drvMain(char *szDrvName, char *szDrvPath, int nDrvLang);
typedef int(*PFN_Main)(const char* szDrvName, const char *szDrvDir, int nLanguage);

//extern "C" void initpydriver();

int exitWithWait(int nCode)
{
#ifdef WIN32
	Sleep(5000);
#else
	ACE_Time_Value tvSleep;
	tvSleep.msec(5000);
	ACE_OS::sleep(tvSleep);
#endif

	return nCode;
}

// �õ������ļ�·����ȫ��·��
int getDrvFullPath(string &strDrvFullPath)
{
	char szDrvExePath[MAX_FILE_PATH] = { 0 };
#ifdef _WIN32
	::GetModuleFileName(NULL, szDrvExePath, sizeof(szDrvExePath));
#else
	// ����ȡ���̵�ȫ·������
	int  nExePathCount = ACE_OS::readlink("/proc/self/exe", szDrvExePath, sizeof(szDrvExePath));
	if (nExePathCount < 0 || nExePathCount >= sizeof(szDrvExePath))
	{
		printf("Failed to get DrvExePath by readlink(/proc/self/exe)\n");
	}
	else
	{
		szDrvExePath[nExePathCount] = '\0'; // like /icv/Executable/Drivers/modbus, because driverdll same name as exe,
	}
#endif // _WIN32
	strDrvFullPath = szDrvExePath;
	return 0;
}

// �õ�����DrvCmn.dll��·��,���������һ��б�ܺ�
int GetDrvCmnDir(string strDrvFullPath, string &strDrvCmdDir)
{
	strDrvCmdDir = strDrvFullPath;
	int nPos = strDrvCmdDir.find_last_of(OS_DIR_SEP_CHAR);
	if (nPos >= 0)
		strDrvCmdDir = strDrvCmdDir.substr(0, nPos); // /icv/Executable/drivers/modbus
	strDrvCmdDir += OS_DIR_SEP_STR;
	strDrvCmdDir += "..";
	strDrvCmdDir += OS_DIR_SEP_STR;
	strDrvCmdDir += "..";
	return 0;
}

// �õ�û����չ����·������ /gateway/bin/drivers/modbus,����������.dll
int GetDriverDir(string strDrvExeFullPath, string & strDriverDir)
{
	// �ҵ���չ�ļ�����ȥ��.��
	strDriverDir = strDrvExeFullPath;
	// ����binĿ¼���ҵ���binĿ¼֮ǰ�ľ���homepath
	std::size_t pos = strDriverDir.rfind(OS_DIR_SEP_STR);
	if (pos != string::npos)
		strDriverDir = strDriverDir.substr(0, pos);

	return 0;
}


// �õ�����DrvCmn.dll��·��
int GetDrvName(string strDrvFullPath, string &strDrvName)
{
	strDrvName = strDrvFullPath;
	// ��ȡ��������ļ�������Ϊ��������
	int nPos = string::npos;
	if ((nPos = strDrvName.find_last_of(OS_DIR_SEP_CHAR)) != string::npos)
		strDrvName = strDrvName.substr(nPos + 1); // �õ� modbus.exe

	if ((nPos = strDrvName.find_last_of('.')) != string::npos)
		strDrvName = strDrvName.substr(0, nPos); // �õ� modbus

	return 0;
}


int main(int argc, char** argv)
{
	string strDrvFullPath, strDrvDir, strDrvName, strDrvCmnDir, strDrvCmnPath;
	getDrvFullPath(strDrvFullPath); // full path of exe. if is symbol link, then to final file
	GetDriverDir(strDrvFullPath, strDrvDir);		// driver path
	GetDrvCmnDir(strDrvFullPath, strDrvCmnDir);	// drvcmn.dll path, binĿ¼
	GetDrvName(strDrvFullPath, strDrvName);
	strDrvCmnPath = strDrvCmnDir + OS_DIR_SEP_STR + CVDRIVER_COMMON_DLL_NAME;

	string strPythonDir = strDrvCmnDir + OS_DIR_SEP_STR + ".." + OS_DIR_SEP_STR + "python"; // ��python27.dll��������Ҫ�ģ�
	//// ���û������������ӣ�bin/drivers/drivername;bin;python/binĿ¼
	//string strDriverDir = strCurDir + PK_OS_DIR_SEPARATOR + "drivers" + PK_OS_DIR_SEPARATOR + g_strDrvName;
	//string strPythonDir = strCurDir + PK_OS_DIR_SEPARATOR + "python"; // ��python27.dll��������Ҫ�ģ�

	//string strSysPath = getenv("PATH");
	//strSysPath = strDriverDir + ";" + strCurDir + ";" + strPythonDir + ";" + strSysPath;
	//string strEnv = "PATH=";// c:\\windows";
	//strEnv = strEnv + strSysPath;

	////int nRet = putenv(strEnv.c_str());
	//int nRet = putenv(strEnv.c_str()); // �����������Ч�ģ���֪���ǲ�����Ϊ����Ա��ԭ��GetLastError����0
	//if (nRet != 0)
	//{
	//	g_logger.LogMessage(PK_LOGLEVEL_CRITICAL, "Set Enviroment Variable path failed, ret:%d, %s", nRet, strEnv.c_str());
	//}
	//string strNewPath = getenv("path");
#ifdef WIN32

	if (!::SetCurrentDirectory(strDrvCmnDir.c_str()))
		printf("fail to set current dir��%s!\n", strDrvCmnDir.c_str());
	else
		printf("succeed to set current dir��%s!\n", strDrvCmnDir.c_str());

	string strEnvPathValue = getenv("PATH");
	strEnvPathValue = strDrvDir + ";" + strDrvCmnDir + ";" + strPythonDir + ";" + strEnvPathValue;
	string strEnvPath = "PATH=";
	strEnvPath = strEnvPath + strEnvPathValue;
	int nRet = putenv(strEnvPath.c_str()); // �����������Ч�ģ���֪���ǲ�����Ϊ����Ա��ԭ��GetLastError����0
	if (nRet != 0)
	{
		printf("Set Enviroment Variable path failed, ret:%d, ENV: %s\n", nRet, strEnvPath.c_str());
	}
	strEnvPathValue = getenv("PATH");
	printf("path=%s", strEnvPathValue.c_str());

	HMODULE hModule = LoadLibrary(strDrvCmnPath.c_str());
	if (NULL == hModule)
	{
		printf("CANNOT LOAD DRIVERBASE DLL %s, Error Code:%d\n", strDrvCmnPath.c_str(), GetLastError());
		return exitWithWait(LOAD_LIBRARY_FAILED);
	}
	else
		printf("Succeed to Load Driver DBase DLL %s\n", strDrvCmnPath.c_str());

	//::SetCurrentDirectory(szOldCurDir);
	PFN_Main pfnMain = (PFN_Main)GetProcAddress(hModule, "drvMain");
	if (pfnMain == NULL)
	{
		printf("Failt to Get Function:%s from DriverBase DLL, Error Code:%d\n", strDrvCmnPath.c_str(), GetLastError());
		return exitWithWait(EXPORT_SYMBOL_FROM_LIBRARY_FAILED);
	}

	//pfnMain(argc, argv);
	pfnMain(strDrvName.c_str(), strDrvDir.c_str(), 0); // �������ʹ�������ȥȷ��
	FreeLibrary(hModule);
#else
	char szOldCurDir[1024] = { 0 };
	ACE_OS::getcwd(szOldCurDir, sizeof(szOldCurDir));
	printf("CurrentWorkDir: %s \n", szOldCurDir);
	ACE_OS::chdir(strDrvCmnDir.c_str());

	//initpydriver();
	//int nRet = drvMain((char *)strDrvName.c_str(),(char *)strDrvDir.c_str(), 0);
	//printf("drvMain(%s,%s) return %d\n",strDrvName.c_str(),strDrvDir.c_str(),nRet);

	ACE_DLL hDrvComonDll;
	long nErr = hDrvComonDll.open(strDrvCmnPath.c_str());
	if (nErr != 0)
	{
		printf("Load %s failed!ret:%d\n", strDrvCmnPath.c_str(), nErr);
		return exitWithWait(LOAD_LIBRARY_FAILED);
	}
	printf("Load %s successfull!\n", strDrvCmnPath.c_str());

	//ACE_OS::chdir(szOldCurDir);
	PFN_Main pfnMain = (PFN_Main)hDrvComonDll.symbol("drvMain");
	if (pfnMain == NULL)
	{
		printf("Driver base: %s, failed to Get Function:drvMain\n", strDrvCmnPath.c_str());
		return exitWithWait(EXPORT_SYMBOL_FROM_LIBRARY_FAILED);
	}

	pfnMain(strDrvName.c_str(), strDrvDir.c_str(), 0);
	hDrvComonDll.close();
#endif

	return 0;
};
