#include "DataAcquisition.h"
#include "pklog/pklog.h"
#include <cstdio>
#include <sstream>
#include <cstring>
#include <stdlib.h>

extern CPKLog g_logger;

#ifdef WIN32 

#define P_KILL_EXIT_CODE		1

//#define WIN32_LEAN_AND_MEAN	1
#define	C_DRIVERLETTER		0x43
#define	COMMANDLINE_LENGTH	2048

#include <cwchar>
//#include <tchar.h> 
#include <ctype.h>  
#include <Winsock2.h>
#include <WS2tcpip.h>
#include <Windows.h> 
#include <TlHelp32.h> 
#include <Pdh.h>
#pragma comment(lib, "Pdh.lib")
#include <Psapi.h> 
#pragma comment(lib, "Psapi.lib")
#include <IPHlpApi.h>
#pragma comment(lib, "Iphlpapi.lib")
#include <WinIoCtl.h>
#include <ShellAPI.h>
#include <devguid.h>
#include <Setupapi.h> 
#pragma comment(lib, "Setupapi.lib")
//#include <Winver.h> 
//#pragma comment(lib, "Mincore.lib")
#pragma comment(lib, "Version.lib")

PDH_HQUERY g_Query = NULL;
PDH_HCOUNTER *g_pCounters = NULL; 
PDH_FMT_COUNTERVALUE *g_pCounterValue = NULL;

#else
 
//#ifdef __GNUC__
//#include "x86intrin.h"
//#define __rdtsc() __builtin_ia32_rdtsc()
//#endif

#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sys/times.h>
#include <sys/vtimes.h>
#include <sys/socket.h> 
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <dirent.h>
#include <unistd.h> //sleep
#include <netdb.h>  
#include <arpa/inet.h> 
#include <signal.h>
#include <map>

#define HERTZ (sysconf(_SC_CLK_TCK) * 1.0) 
#define READ_CHARS_LENGTH	1024 

#endif

using namespace std;

DataAcquisition::DataAcquisition(void) : 
	m_nProcessorCores(0),
	m_nPhysicalMemory(0),
	m_nVirtualMemory(0)
{
} 
DataAcquisition::~DataAcquisition(void)
{
#ifdef WIN32 
	if(g_Query != NULL)
		PdhCloseQuery(g_Query); 
	if(g_pCounters != NULL || g_pCounterValue != NULL)
	{
		delete[] g_pCounters;
		delete[] g_pCounterValue;
	} 
#else

#endif
}

/*------Global Function Start------*/ 
#ifdef WIN32
bool AdjustTokenPrivilege(HANDLE hProcess, DWORD nDesiredAccess = TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY /* TOKEN_ALL_ACCESS */)
{
	HANDLE hToken = NULL;
	if(!OpenProcessToken(hProcess, nDesiredAccess, &hToken))
	{
		if(hToken != NULL)
			CloseHandle(hToken);
		if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) 
			 return true;
		else return false;
	}

	TOKEN_PRIVILEGES tokenPrivileges;  
	if(!LookupPrivilegeValue(NULL, SE_DEBUG_NAME/*SE_SECURITY_NAME*/, &tokenPrivileges.Privileges[0].Luid))
	{
		if(hToken != NULL)
			CloseHandle(hToken);
		return false;
	}

	tokenPrivileges.PrivilegeCount = 1;
	tokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	BOOL bReturn = AdjustTokenPrivileges(hToken, FALSE, &tokenPrivileges, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL);
	if(hToken != NULL)
		CloseHandle(hToken);

	return bReturn == TRUE;
}

int TheProcessIDByWindowName(const char* szWinName)
{
	DWORD ProcessID = -1;
		HWND hWnd = FindWindow(NULL, szWinName);
	if (hWnd == NULL) 
		return -1;
	GetWindowThreadProcessId(hWnd, &ProcessID);  
		CloseHandle(hWnd);
	return ProcessID;
}

HANDLE TheProcessByID(int nPid, unsigned long nDesiredAccess = PROCESS_ALL_ACCESS /* PROCESS_QUERY_INFORMATION | PROCESS_VM_READ */)
{
	//return OpenProcess(PROCESS_ALL_ACCESS, FALSE, nPid);
	return OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, nPid);

}

/**
* 提升进程权限。
*/
void EnableDebugPrivilege(HANDLE processHandle)
{
	HANDLE hToken;
	LUID sedebugnameValue;
	TOKEN_PRIVILEGES tkp;
	if (!OpenProcessToken(processHandle, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		long err = GetLastError();
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "OpenProcessToken is errno %d", err);
		return;
	}
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &sedebugnameValue))
	{	
		long err = GetLastError();
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "LookupPrivilegeValue is errno %d", err);
		CloseHandle(hToken);
		return;
	}
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Luid = sedebugnameValue;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof tkp, NULL, NULL))
	{
		long err = GetLastError();
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "AdjustTokenPrivileges is errno %d", err);
		CloseHandle(hToken);
	}
}
int TheCommandLineByProcessID(int nPid, char* &szCmdLine, SIZE_T &nLength)
{
	// 设置当前进程权限。
	//EnableDebugPrivilege(GetCurrentProcess());
	HANDLE hProcess = TheProcessByID(nPid, PROCESS_ALL_ACCESS);	
	if (hProcess == NULL)
		return -1;
	 //提升进程的权限
	if (!AdjustTokenPrivilege(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES |TOKEN_QUERY))
	{
		long err = GetLastError();
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "AdjustTokenPrivilege is errno %d", err);
		return -1;
	}
	DWORD dwThreadId = 0;
	HANDLE hThread = CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)GetCommandLine, NULL, 0, &dwThreadId);
	if (hThread == NULL)
	{
	    long err = GetLastError();
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "CreateRemoteThread is errno %d",err);
		CloseHandle(hProcess);
		return -1;
	}
	 
	WaitForSingleObject(hThread, 4000);
	DWORD dwExitCode = 0;
	GetExitCodeThread(hThread, &dwExitCode);
	if(0 == ReadProcessMemory(hProcess, (LPCVOID)dwExitCode,szCmdLine, nLength, &nLength))
	{
		CloseHandle(hThread);
		CloseHandle(hProcess);
		return -1;
	} 
	CloseHandle(hThread);
	CloseHandle(hProcess);
	return 0;
}
#else

void LinesFrom(FILE *pFile, vector<string> &vResult)
{
	vResult.clear();
	char szLine[READ_CHARS_LENGTH] = { 0 };
	while(fgets(szLine, READ_CHARS_LENGTH, pFile) != NULL)
	{
		int nLength = strlen(szLine);
		if(szLine[nLength - 1] == '\n')
			szLine[nLength - 1] = '\0';
		vResult.push_back(szLine);
	}
}

string trim(string &s)
{
    if(s.empty())
        return s;
    s.erase(0,s.find_first_not_of(" "));
    s.erase(s.find_last_not_of(" ")+1);
    return s;
}
void LinesTopCpu(FILE *pFile, vector<string> &vResult)
{
    vResult.clear();
    char szLine[READ_CHARS_LENGTH] = { 0 };
    //--add
    if(fgets(szLine, READ_CHARS_LENGTH, pFile) != NULL)
    {
        string strNotUsage(szLine);
        trim(strNotUsage);
        string strResult= strNotUsage.substr(0,strNotUsage.find_first_of("%"));
        double ff=atof(strResult.c_str());
        double useff=100.0-ff;
        char  szUse[24]={0};
        sprintf(szUse,"%lf",useff);
        vResult.push_back(string(szUse));
       // vResult.push_back(strResult);
    }
}

int ExecuteCommand(const char *szCommand, vector<string> &vResult)
{	
	FILE *pCmd = popen(szCommand, "r");
	if(pCmd == NULL) 
		return -1; 
    if(strncmp((char*)szCommand,"top",3)==0)
           LinesTopCpu(pCmd, vResult);
    else
           LinesFrom(pCmd, vResult);
    //LinesFrom(pCmd, vResult);
	return pclose(pCmd);
}

int OpenTheFileWithMode(const char *szFile, vector<string> &vResult, const char *szMode = "r")
{
	FILE *pFile = fopen(szFile, szMode);
	if(pFile == NULL) 
		return -1; 
	LinesFrom(pFile, vResult);
	return fclose(pFile);
}

int CPUCurrentUsageCalculator(vector<ULLong> &vltTotal, vector<ULLong> &vltIdle,vector<string> &vResult)
{
	vltTotal.clear();
	vltIdle.clear();	
	// /proc/stat | grep cpu | awk -F ' ' '{ print $2 + $3 + $4 + $5 + $6 + $7 + $8 + $9, $5 }'
    //ExecuteCommand("cat /proc/stat | grep cpu | awk -F ' ' '{ print $2 + $3 + $4 + $5 + $6 + $7 + $8 + $9, $5 }'", vResult);
     ExecuteCommand("top -n 1 -b | grep  Cpu | cut -d ',' -f 4",vResult);
	int nCores = vResult.size();
#if 0
    for(int i = 0; i < nCores; i++)
	{
		ULLong ltTotal, ltIdle;
		sscanf(vResult[i].c_str(), "%llu %llu", &ltTotal, &ltIdle);
		vltTotal.push_back(ltTotal);
		vltIdle.push_back(ltIdle);
	}
#endif
	return nCores;
}
#endif
/*------Global Function End------*/ 


/*------CPU Start------*/ 
int DataAcquisition::ProcessorCores()
{
	if(m_nProcessorCores == 0)
	{
#ifdef WIN32
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		m_nProcessorCores = sysInfo.dwNumberOfProcessors;
#else 
		m_nProcessorCores = sysconf(_SC_NPROCESSORS_ONLN);
#endif
	}
	return m_nProcessorCores;
}

int DataAcquisition::CpuUsageOfCores(vector<double> &vUtilization)
{
	vUtilization.clear();
	int nCores = ProcessorCores();
	vUtilization.assign(nCores + 1, 0);
	
#ifdef WIN32
	/*also by FILETIME  GetSystemTimes()
		utilization = (1 - idle / (kernel + user)) * 100;
	*/
	if(g_Query == NULL || g_pCounters == NULL)	
	{
		g_pCounters = new PDH_HCOUNTER[nCores + 1];
		g_pCounterValue = new PDH_FMT_COUNTERVALUE[nCores + 1];

		PdhOpenQuery(NULL, NULL, &g_Query);
		PdhAddCounter(g_Query, "\\Processor(_Total)\\% Processor Time", NULL, &g_pCounters[0]);

		char szCounterPath[64] = { 0 };
		for(int i = 1; i <= nCores; i++)
		{
			sprintf(szCounterPath, "\\Processor(%d)\\%% Processor Time", i);
			PdhAddCounter(g_Query, szCounterPath, NULL, &g_pCounters[i]);
		}
		PdhCollectQueryData(g_Query);
		for(int i = 0; i <= nCores; i++)
		{
			PdhGetFormattedCounterValue(g_pCounters[i], PDH_FMT_DOUBLE, NULL, &g_pCounterValue[i]);
		}
		Sleep(QUERY_SLEEP_TIMESTAMP_MS / 8);
	}	
	PdhCollectQueryData(g_Query);
	for(int i = 0; i <= nCores; i++)
	{
		PdhGetFormattedCounterValue(g_pCounters[i], PDH_FMT_DOUBLE, NULL, &g_pCounterValue[i]);
		vUtilization[i] = g_pCounterValue[i].doubleValue;
	}
	//Sleep(QUERY_SLEEP_TIMESTAMP_MS / 16);
#else
	vector<ULLong> vltTotalFirst, vltIdleFirst;  
    vector<string> nResult;
    nCores = CPUCurrentUsageCalculator(vltTotalFirst, vltIdleFirst,nResult);
     vUtilization[0]=atof(nResult[0].c_str());
	sleep(1); 
#if 0
	vector<ULLong> vltTotalSecond, vltIdleSecond;
    nCores = CPUCurrentUsageCalculator(vltTotalSecond, vltIdleSecond,nResult);

	for(int i = 0; i < nCores; i++)
	{
		double divisor = (vltIdleSecond[i] - vltIdleFirst[i]) * 1.0;
		double dividend = (vltTotalSecond[i] - vltTotalFirst[i]) * 1.0;
		double utilizate = (dividend - divisor) <= 0 ? 0 : (dividend - divisor) / dividend;
		vUtilization[i] = utilizate >= 1 ? 100 : utilizate * 100; 
	}  
#endif
#endif
    return 0; 	 
}

int DataAcquisition::CpuUsageByProcess(double &nUtilize, int nPid)
{
	nUtilize = 0; 

#ifdef WIN32 
	/*also by "\\Process(%s)\\% Processor Time"  
		%s=The Process Name;
	*/
	ULARGE_INTEGER uiPFistIdle, uiPFistKernel, uiPFistUser;
	FILETIME ftPFistIdle, ftPFistKernel, ftPFistUser;
	
	GetSystemTimeAsFileTime(&ftPFistIdle);
	memcpy(&uiPFistIdle, &ftPFistIdle, sizeof(FILETIME));
	 
	HANDLE hProcess = nPid == -1 ? GetCurrentProcess() : TheProcessByID(nPid);
	AdjustTokenPrivilege(hProcess);

	if(FALSE == GetProcessTimes(hProcess, &ftPFistIdle, &ftPFistIdle, &ftPFistKernel, &ftPFistUser))
	{
		CloseHandle(hProcess);
		return -1;
	}
	memcpy(&uiPFistUser, &ftPFistUser, sizeof(FILETIME));	  
	memcpy(&uiPFistKernel, &ftPFistKernel, sizeof(FILETIME));
		 
	Sleep(QUERY_SLEEP_TIMESTAMP_MS / 16);
	   
	DWORD eCode = 0;
	GetExitCodeProcess(hProcess, &eCode);    
    if(eCode != STILL_ACTIVE)  
	{
		CloseHandle(hProcess);
        return -1;
	}

	ULARGE_INTEGER uiPSecondIdle, uiPSecondKernel, uiPSecondUser; 
	FILETIME ftPSecondIdle, ftPSecondKernel, ftPSecondUser;

	GetSystemTimeAsFileTime(&ftPSecondIdle);
	memcpy(&uiPSecondIdle, &ftPSecondIdle, sizeof(FILETIME));
	  
	if(FALSE == GetProcessTimes(hProcess, &ftPSecondIdle, &ftPSecondIdle, &ftPSecondKernel, &ftPSecondUser))
	{
		CloseHandle(hProcess);
		return -1;
	}
	memcpy(&uiPSecondUser, &ftPSecondUser, sizeof(FILETIME));	  
	memcpy(&uiPSecondKernel, &ftPSecondKernel, sizeof(FILETIME));

	//(kernel + user) / (idle * cors) * 100; 
	double divisor = (uiPSecondKernel.QuadPart - uiPFistKernel.QuadPart + uiPSecondUser.QuadPart - uiPFistUser.QuadPart) * 1.0;
	double dividend = (uiPSecondIdle.QuadPart - uiPFistIdle.QuadPart) * 1.0; 
	double utilization = (dividend <= 0 || divisor <= 0) ? 0 : divisor / dividend * ProcessorCores();	
	nUtilize = utilization >= 1 ? 100 : utilization * 100; 
#else 
	// also "ps -p {pid} h -o %cpu"
	// cat /proc/{pid}/stat | awk -F ' ' '{	 print $14 + $15 + $16 + $17 }'
	if(nPid == 0)
		return -1;
	if(nPid <= -1)
		nPid = getpid();
		
	char cmd[128] = { 0 };
	sprintf(cmd, "cat /proc/%i/stat | awk -F ' ' '{ print $14 + $15 + $16 + $17 }'", nPid);

	vector<string> vResult;	  
	ExecuteCommand(cmd, vResult);  
	if(vResult.size() == 0)
		return -1;  
		
	ULLong nTotalFirst;
	sscanf(vResult[0].c_str(), "%llu", &nTotalFirst);
	vector<ULLong> vltTotalFirst, vltIdleFirst;  
    vector<string> nResult;
    CPUCurrentUsageCalculator(vltTotalFirst, vltIdleFirst,nResult);

	sleep(1); 
	  
	ExecuteCommand(cmd, vResult);  
	if(vResult.size() == 0)
		return -1;  
	
	ULLong nTotalSecond;
	sscanf(vResult[0].c_str(), "%llu", &nTotalSecond);
	vector<ULLong> vltTotalSecond, vltIdleSecond;
    CPUCurrentUsageCalculator(vltTotalSecond, vltIdleSecond,nResult);
	  		 
	double divisor = (nTotalSecond - nTotalFirst) * 1.0;
	double dividend = (vltTotalSecond[0] - vltTotalFirst[0]) * 1.0;
	double utilization = (dividend <= 0 || divisor <= 0) ? 0 : divisor / dividend * ProcessorCores();
	nUtilize = utilization >= 1 ? 100 : utilization * 100; 
#endif
	return 0;
}	 
/*------ CPU End ------*/


/*------Memory Start------*/
int	DataAcquisition::PhysicalMemory(ULLong &nTotal)
{
	if(m_nPhysicalMemory == 0)
	{
#ifdef WIN32
		MEMORYSTATUSEX vMemory;
		vMemory.dwLength = sizeof(MEMORYSTATUSEX);
		GlobalMemoryStatusEx(&vMemory);

		m_nPhysicalMemory = vMemory.ullTotalPhys / 1024;
#else
		struct sysinfo pMemory;
		sysinfo(&pMemory); 
	
		m_nPhysicalMemory = pMemory.totalram * pMemory.mem_unit / 1024;
#endif
	}
	nTotal = m_nPhysicalMemory;
	return 0;
}

int DataAcquisition::PhysicalMemoryFree(ULLong &nFree)
{
	nFree = 0;

#ifdef WIN32
	/*also by "\\Memory\\Available Bytes";
	*/
	MEMORYSTATUSEX vMemory;
	vMemory.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&vMemory);

	if(m_nPhysicalMemory == 0)
		m_nPhysicalMemory = vMemory.ullTotalPhys / 1024;
	nFree = vMemory.ullAvailPhys / 1024; 
#else
	/*also /proc/meminfo
	*/
	struct sysinfo pMemory;
	sysinfo(&pMemory); 

	if(m_nPhysicalMemory == 0)
		m_nPhysicalMemory = pMemory.totalram * pMemory.mem_unit / 1024;
	nFree = pMemory.freeram * pMemory.mem_unit / 1024;
#endif
	return 0;
}

int DataAcquisition::PhysicalMemroyUsageByProcess(ULLong &nUsage, int nPid)
{
	nUsage = 0;	

#ifdef WIN32
	PROCESS_MEMORY_COUNTERS pMemCounter; 
	HANDLE hProcess = nPid == -1 ? GetCurrentProcess() : TheProcessByID(nPid);
	GetProcessMemoryInfo(hProcess, &pMemCounter, sizeof(PROCESS_MEMORY_COUNTERS)); 
	CloseHandle(hProcess);

	nUsage = pMemCounter.WorkingSetSize / 1024;
#else
	/*also "ps -p {pid} h -o %mem"
	*/
	if(nPid == 0)
		return -1;
	if(nPid <= -1)
		nPid = getpid();

	//			grep VmRSS /proc/{nPid}/status
	//			RssAnon :system-monitor value
	char buf[32] = { 0 };
	sprintf(buf, "grep VmRSS /proc/%i/status", nPid); 
	vector<string> vResult;
	ExecuteCommand(buf, vResult);
	
	if(vResult.size() == 0)
		return -1; 
	 
	sscanf(vResult[0].c_str(), "VmRSS:%llukB", &nUsage); 
#endif
	return 0;
}

int	DataAcquisition::VirtualMemory(ULLong &nTotal)
{
	if(m_nVirtualMemory == 0)
	{
#ifdef WIN32
		MEMORYSTATUSEX vMemory;
		vMemory.dwLength = sizeof(MEMORYSTATUSEX);
		GlobalMemoryStatusEx(&vMemory);

		m_nVirtualMemory = vMemory.ullTotalVirtual / 1024;
#else
		struct sysinfo vMemory;
		sysinfo(&vMemory);
		
		m_nVirtualMemory = vMemory.totalswap * vMemory.mem_unit / 1024;   
#endif
	}
	nTotal = m_nVirtualMemory;
	return 0;
}

int DataAcquisition::VirtualMemoryFree(ULLong &nFree)
{
	nFree = 0;	

#ifdef WIN32

	MEMORYSTATUSEX vMemory;
	vMemory.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&vMemory);

	if(m_nVirtualMemory == 0)
		m_nVirtualMemory = vMemory.ullTotalVirtual / 1024;
	nFree = vMemory.ullAvailVirtual / 1024; 

#else

	struct sysinfo vMemory;
	sysinfo(&vMemory); 

	if(m_nVirtualMemory == 0)
		m_nVirtualMemory = vMemory.totalswap * vMemory.mem_unit / 1024; 
	nFree = vMemory.freeswap * vMemory.mem_unit / 1024; 

#endif

	return 0;
}

int DataAcquisition::VirtualMemroyUsageByProcess(ULLong &nUsage, int nPid)
{
	nUsage = 0; 

#ifdef WIN32
	PROCESS_MEMORY_COUNTERS pMemCounter; 
	HANDLE hProcess = nPid == -1 ? GetCurrentProcess() : TheProcessByID(nPid);
	GetProcessMemoryInfo(hProcess, &pMemCounter, sizeof(PROCESS_MEMORY_COUNTERS)); 
	CloseHandle(hProcess); 

	nUsage = pMemCounter.PagefileUsage / 1024;
#else  
	if(nPid == 0)
		return -1;
	if(nPid <= -1)
		nPid = getpid(); 

	char buf[32] = { 0 };
	sprintf(buf, "ps p %i -o vsz h", nPid); 
	vector<string> vResult;
	ExecuteCommand(buf, vResult);
	
	if(vResult.size() == 0)
		return -1; 
	 
	sscanf(vResult[0].c_str(), "%llu", &nUsage); 
#endif
	return 0;
}
/*------ Memory End ------*/


/*------Disk Start------*/
int	DataAcquisition::DiskCount()
{
	int nCount = 1;//At Least 1

#ifdef WIN32
	HDEVINFO hDeviceInfoSet = SetupDiGetClassDevs(&GUID_DEVCLASS_DISKDRIVE, NULL, NULL, DIGCF_PRESENT);
	if (hDeviceInfoSet == INVALID_HANDLE_VALUE)   
		return nCount; 
   
	SP_DEVINFO_DATA deviceInfoData;
	deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA); 
	DWORD nDeviceIndex  = 0;   
	while(SetupDiEnumDeviceInfo(hDeviceInfoSet, nDeviceIndex, &deviceInfoData)) 
		nDeviceIndex++;   
	if(hDeviceInfoSet)
		SetupDiDestroyDeviceInfoList(hDeviceInfoSet); 
	nCount = nDeviceIndex;
#else
	//lsblk -nd | grep -w disk
	vector<string> vResult;
	ExecuteCommand("lsblk -dn | grep -w disk", vResult);
	int nSize = vResult.size();
	if(nSize > 0)
		return nSize;
#endif
	return nCount;
}

int	DataAcquisition::DriveLetterCount()
{ 
	int nCount = 0;

#ifdef WIN32 
	//Bit position 0 (the least-significant bit) is drive A, bit position 1 is drive B, bit position 2 is drive C, and so on.
	DWORD nDrive = GetLogicalDrives();
	while(nDrive)
	{
		if(nDrive & 1) 
			++nCount;
		nDrive >>= 1;
	} 
#else
	//df --output=target
	//df -a
	vector<string> vResult;
	//Fedora
	//ExecuteCommand("df --output=target", vResult);
	//Redhat
	ExecuteCommand("df -a", vResult);
	int nSize = vResult.size();
	if(nSize > 0)
		return nSize - 1;
#endif
	return nCount;
}	

int DataAcquisition::TheDiskCapacityInfo(vector<CapacityInfo*> &vCapacityInfo)
{
	vCapacityInfo.clear();	

#ifdef WIN32
	int nPhsicalDriveCount = DiskCount();
	while(nPhsicalDriveCount-- > 0)
	{
		char szPhysicalDrive[32] = { 0 };
		sprintf(szPhysicalDrive, "\\\\.\\PhysicalDrive%d", nPhsicalDriveCount);  

		HANDLE hDevice = CreateFile(szPhysicalDrive, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		if (hDevice == INVALID_HANDLE_VALUE)
		{
			CloseHandle(hDevice);
			return -1;
		}
 
		DWORD nReturn = 0;
		DISK_GEOMETRY outBuffer; 
		if(DeviceIoControl(hDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &outBuffer, sizeof(DISK_GEOMETRY), &nReturn, (LPOVERLAPPED)NULL)) 
		{
			CapacityInfo *pCI = new CapacityInfo();
			pCI->m_nTotal = outBuffer.Cylinders.QuadPart * (ULONG)outBuffer.TracksPerCylinder * (ULONG)outBuffer.SectorsPerTrack * (ULONG)outBuffer.BytesPerSector / 1024; 
			vCapacityInfo.push_back(pCI);
		}

		CloseHandle(hDevice);   
	}	
#else
	//lsblk -ndbo name,size,type | grep -w disk | awk -F ' ' '{print $1, $2 / 1024}'
	vector<string> vResult;
	ExecuteCommand("lsblk -ndbo name,size,type | grep -w disk | awk -F ' ' '{print $1, $2 / 1024}'", vResult);

	if(vResult.size() == 0)
		return -1;

	for(vector<string>::iterator item = vResult.begin(); item != vResult.end(); item++)
	{
		CapacityInfo *pCI = new CapacityInfo();
		sscanf(item->c_str(), "%s %llu", pCI->m_szName, &pCI->m_nTotal); 
		vCapacityInfo.push_back(pCI);
	}
#endif
	return 0;
}
	
int DataAcquisition::TheDriveLetterCapacityInfo(vector<CapacityInfo*> &vCapacityInfo, const char* szDriveLetter)
{
	vCapacityInfo.clear();	

#ifdef WIN32  
	int nDriveCount = 0, nIndex = 0;
	if(szDriveLetter == NULL) 
		nDriveCount = DriveLetterCount();

	char szDriveLetterFormat[4] = { 0 };
	ULARGE_INTEGER atc, nTotal, nFree; 
	do
	{
		if(szDriveLetter == NULL) 
			sprintf(szDriveLetterFormat, "%c:\\", C_DRIVERLETTER + nIndex++);

		UINT nType = GetDriveType(szDriveLetter == NULL ? szDriveLetterFormat : szDriveLetter);
		if(nType != DRIVE_REMOVABLE && nType != DRIVE_FIXED) 
			continue; 

		if(GetDiskFreeSpaceEx(szDriveLetter == NULL ? szDriveLetterFormat : szDriveLetter, &atc, &nTotal, &nFree))
		{ 
			CapacityInfo *pCI = new CapacityInfo();
			pCI->m_nTotal = nTotal.QuadPart / 1024;
			pCI->m_nFree = nFree.QuadPart / 1024;
			PKStringHelper::Safe_StrNCpy(pCI->m_szName, szDriveLetter == NULL ? szDriveLetterFormat : szDriveLetter, sizeof(pCI->m_szName));
			vCapacityInfo.push_back(pCI);
		} 

		if(szDriveLetter != NULL) 
			break;

	}while(nIndex < nDriveCount);
#else
	vector<string> vResult;
	//all of mounted partitions
	if(szDriveLetter == NULL)
	{
		//df -l | grep -E '^/|/mnt/'
        ExecuteCommand("df -l | grep -E '^/|/mnt/'", vResult);
       // ExecuteCommand("df -l | grep -w /", vResult);
	}
	else//just the special one
	{
		//df -l | grep -w {szPartition}
		if(szDriveLetter[0] != '/')
			return -1;
		string cmd("df -l | grep -w ");
		cmd.append(szDriveLetter);
        ExecuteCommand(cmd.c_str(), vResult);
	}
	
	if(vResult.size() == 0)
		return -1;
		
	for(vector<string>::iterator item = vResult.begin(); item != vResult.end(); item++)
	{
		CapacityInfo *pCI = new CapacityInfo();
       // sscanf(item->c_str(), "%*s %llu %*llu %llu %*i%% %s", &pCI->m_nTotal, &pCI->m_nFree, pCI->m_szName);
        sscanf(item->c_str(), " %llu %*llu %llu %*i%% %s", &pCI->m_nTotal, &pCI->m_nFree, pCI->m_szName);
        vCapacityInfo.push_back(pCI);
	}  
#endif
	return 0;
}	
/*------ Disk End ------*/


/*------Process Start------*/
int DataAcquisition::Kill(int nPid)
{
#ifdef WIN32  
    HANDLE hProcess = nPid == -1 ? GetCurrentProcess() : TheProcessByID(nPid, PROCESS_TERMINATE);
    if (hProcess == NULL)
    {
        g_logger.LogMessage(PK_LOGLEVEL_ERROR,"Cannot find process when killing process(pid=%d)", nPid);
        return -1;
    }
	AdjustTokenPrivilege(hProcess);
    if(TerminateProcess(hProcess, P_KILL_EXIT_CODE))
	{
		CloseHandle(hProcess);
        g_logger.LogMessage(PK_LOGLEVEL_NOTICE,"successful to kill process(pid=%d)", nPid);
		return 0;
	}
	else
	{
        g_logger.LogMessage(PK_LOGLEVEL_ERROR,"fail to kill process(pid=%d),TerminateProcess return false", nPid);
		CloseHandle(hProcess);
		return -1;
	}
#else 
	if(nPid == 0)
    {
        g_logger.LogMessage(PK_LOGLEVEL_ERROR,"cannot kill process(pid=%d),pid cannot be 0", nPid);
        return -1;
    }
    int nPidToKill = nPid <= -1 ? getpid() : nPid;
    int nRet = kill(nPidToKill, SIGTERM);
    if(nRet == 0)
    {
        g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "successful to kill process(pid=%d)", nPidToKill);
        return nRet;
    }
    else
    {
         g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "fail to kill process(pid=%d), kill return:%d", nPidToKill, nRet);
        return nRet;
    }
	//return killpg(nPid <= -1 ? getpid() : nPid, SIGTERM);
#endif

	return 0;
}

int DataAcquisition::OpenTheFile(const char* szFile, const char* szParameters)
{
	const char* szSplitCharFind = strchr(szFile, '|');
    if(szSplitCharFind == NULL)
    {
        return -1;
    }

    //split by '|'
    int nLenProcessPath = szSplitCharFind - szFile;
    char* szProcessPath = new char[nLenProcessPath + 1]();
	PKStringHelper::Safe_StrNCpy(szProcessPath, szFile, nLenProcessPath + 1);
    szSplitCharFind++;
    int nLenProcessCmdLine = strlen(szSplitCharFind);
    char* szProcessCmdLine = new char[nLenProcessCmdLine + 1]();
	PKStringHelper::Safe_StrNCpy(szProcessCmdLine, szSplitCharFind, nLenProcessCmdLine + 1);
#ifdef WIN32
	ShellExecute(NULL, "open", szProcessPath, szProcessCmdLine, NULL, SW_SHOWNORMAL);
	return 0;
#else 
	//execve(szFile, argv, envp);
  /*  string cmd(szFile);
	if(szParameters != NULL)
	{
		cmd.append(" ");
		cmd.append(szParameters);
    }*/
    //add code----------------
#if 1
    
    //--------------------------

  //  cmd.append(" &");
  //  char szCmd[1024]={0};
  //  char szPath[512]={0};
   // sscanf(cmd.c_str(),"%s|%s",szPath,szCmd);
   // char szP[1024]={0};
   // sprintf(szP,"cd %s",szPath);
   // chdir(szPath);

    //-------------------


    //---------------------
    chdir(szProcessPath);
    g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "cd dir to %s",getcwd(NULL,NULL));
#endif

    char szCd[1024]={0};
    sprintf(szCd,"%s &",szProcessCmdLine);
    g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "system %s",szCd);

    return system(szCd);

    //return system(cmd.c_str());
#endif
}

int DataAcquisition::Processes(vector<ProcessInfo*> &vProcessInfo, int nPid)
{
	vProcessInfo.clear();

#ifdef WIN32
	HANDLE hProcess = GetCurrentProcess();
	AdjustTokenPrivilege(hProcess);

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);  
	if (hSnapshot == INVALID_HANDLE_VALUE) 
	{
		CloseHandle(hProcess);
		return -1; 
	}
	 
	PROCESSENTRY32 proEntry32; 
	proEntry32.dwSize = sizeof(PROCESSENTRY32);
	if(!Process32First(hSnapshot, &proEntry32))
	{
		CloseHandle(hSnapshot);
		CloseHandle(hProcess);
		return -1;
	} 

	while(Process32Next(hSnapshot, &proEntry32))
	{    
		if(nPid != -1)
		{
			if(proEntry32.th32ProcessID != nPid)
				continue;
		}
		ProcessInfo *pi = new ProcessInfo(); 
        pi->m_nPid = (unsigned int)proEntry32.th32ProcessID;
		PKStringHelper::Safe_StrNCpy(pi->m_szName, proEntry32.szExeFile, sizeof(pi->m_szName));
		//WideCharToMultiByte(CP_ACP, 0, proEntry32.szExeFile, -1, pi.m_szName, sizeof(pi.m_szName), NULL, NULL);

		HANDLE theProcess = TheProcessByID(proEntry32.th32ProcessID);
		DWORD dwSize = sizeof(pi->m_szFileName);
		QueryFullProcessImageName(theProcess, 0/*PROCESS_NAME_NATIVE or 0*/, pi->m_szFileName, &dwSize); 
		if(dwSize > 0)
		{
				/*
					Comments  InternalName  ProductName  CompanyName  LegalCopyright  ProductVersion
					FileDescription  LegalTrademarks  PrivateBuild  FileVersion  OriginalFilename  SpecialBuild
				*/
			DWORD nHandle = 0;
			DWORD nSize = GetFileVersionInfoSize(pi->m_szFileName, &nHandle); 
			if(nSize > 0)
			{
				char *szVersionInfo = new char[nSize + 1]();
				if(GetFileVersionInfo(pi->m_szFileName, 0, nSize + 1, szVersionInfo))
				{
					DWORD *nLangCode = NULL;
					UINT nLen = 0;
					VerQueryValue(szVersionInfo, "\\VarFileInfo\\Translation", (LPVOID*)&nLangCode, &nLen); 
		
					char szDes[64] = { 0 }; 
					sprintf(szDes, "\\StringFileInfo\\%04x%04x\\FileDescription", LOWORD(nLangCode[0]), HIWORD(nLangCode[0]));

					UINT nLenFileDes = 0;
					char *szFileDes = NULL;
					VerQueryValue(szVersionInfo, szDes, (LPVOID*)&szFileDes, &nLenFileDes); 
					memcpy(pi->m_szDescribe, szFileDes, nLenFileDes);
						//WideCharToMultiByte(CP_ACP, 0, proEntry32.szExeFile, -1, pi.m_szName, sizeof(pi.m_szName), NULL, NULL); 

						//char szPN[64] = { 0 }; 
						//sprintf(szPN, "\\StringFileInfo\\%04x%04x\\ProductName", LOWORD(nLangCode[0]), HIWORD(nLangCode[0]));

						//UINT nLenProName = 0;
						//char *szProName = NULL;
						//VerQueryValue(szVersionInfo, szPN, (LPVOID*)&szProName, &nLenProName);  
						//pi->m_pProductName = new char[nLenProName + 1]();
						////WideCharToMultiByte(CP_ACP, 0, proEntry32.szExeFile, -1, pi.m_szName, sizeof(pi.m_szName), NULL, NULL); 
						//memcpy(pi->m_pProductName, szProName, nLenProName);
				}
				delete[] szVersionInfo;
			}
		} 
		FILETIME ftCreationTime, ftExitTime, ftKernelTime, ftUserTime; 
		if(GetProcessTimes(theProcess, &ftCreationTime, &ftExitTime, &ftKernelTime, &ftUserTime))
		{
		    //or ULARGE_INTEGER uiStart;
		    //	 memcpy(&uiStart, &creationTime, sizeof(ULARGE_INTEGER)); 
			FILETIME ftNowTime;
			GetSystemTimeAsFileTime(&ftNowTime);
			pi->m_nRunTime = (reinterpret_cast<ULARGE_INTEGER*>(&ftNowTime)->QuadPart - reinterpret_cast<ULARGE_INTEGER*>(&ftCreationTime)->QuadPart) / ELAPSEDSECONDS;

			FILETIME ftLocalCreationTime;
			FileTimeToLocalFileTime(&ftCreationTime, &ftLocalCreationTime);
			SYSTEMTIME stLocalUTC8CreationTime;
			FileTimeToSystemTime(&ftLocalCreationTime, &stLocalUTC8CreationTime);
			pi->m_nStartTime_Year = stLocalUTC8CreationTime.wYear;
			pi->m_nStartTime_Month = stLocalUTC8CreationTime.wMonth;
			pi->m_nStartTime_Day = stLocalUTC8CreationTime.wDay;
			pi->m_nStartTime_Hour = stLocalUTC8CreationTime.wHour;
			pi->m_nStartTime_Minute = stLocalUTC8CreationTime.wMinute;
			pi->m_nStartTime_Second = stLocalUTC8CreationTime.wSecond;			
		}
		vProcessInfo.push_back(pi); 
		CloseHandle(theProcess);

		if(nPid != -1) 
			break; 
	}
	CloseHandle(hSnapshot);
	CloseHandle(hProcess);
#else 
	static map<string, int> Month = map<string, int>();
	if(Month.size() == 0)
	{
		Month["Jan"] = 1;Month["Feb"] = 2; Month["Mar"] = 3; Month["Apr"] = 4; Month["May"] = 5; Month["Jun"] = 6; 
		Month["Jul"] = 7; Month["Aug"] = 8; Month["Sep"] = 9; Month["Oct"] = 10; Month["Nov"] = 11; Month["Dec"] = 12; 
	}

	DIR *dir = opendir("/proc");
	if(dir == NULL)
		return -1;

	char cmd[128] = { 0 };
    char month[4] = { 0 };
    vector<string> vResult;
	struct dirent *dirEnt;
	while(dirEnt = readdir(dir))
	{
		if(dirEnt->d_type != DT_DIR)
			continue;

		pid_t pid = (pid_t)atoi(dirEnt->d_name);
		if(pid <= 0)
			continue;

        if(nPid != -1)
        {
            if(pid != nPid)
                continue;
        }
		//ps -p 950 -o pid,etimes,lstart h
		//sprintf(cmd, "ps -p %i -o etimes,lstart h", pid);
		sprintf(cmd, "ps -p %i -o lstart h", pid);
		ExecuteCommand(cmd, vResult);

		ProcessInfo* pi = new ProcessInfo();
		pi->m_nRunTime = 0;
		if(vResult.size() > 0)
		{
			//sscanf(vResult[0].c_str(), "%llu %*s %s %hu %hu:%hu:%hu %hu",
			//        &pi->m_nRunTime, month, &pi->m_nStartTime_Day, &pi->m_nStartTime_Hour, &pi->m_nStartTime_Minute, &pi->m_nStartTime_Second, &pi->m_nStartTime_Year);
			sscanf(vResult[0].c_str(), "%*s %s %hu %hu:%hu:%hu %hu",
				month, &pi->m_nStartTime_Day, &pi->m_nStartTime_Hour, &pi->m_nStartTime_Minute, &pi->m_nStartTime_Second, &pi->m_nStartTime_Year);

			pi->m_nStartTime_Month = Month[string(month)];
		} 
		sprintf(cmd, "/proc/%i/exe", pid);
		readlink(cmd, pi->m_szFileName, sizeof(pi->m_szFileName));
		int nLength = strlen(pi->m_szFileName);
		if(nLength > 0)   
			PKStringHelper::Safe_StrNCpy(pi->m_szName, basename(pi->m_szFileName), sizeof(pi->m_szName)); 
		else
		{
			sprintf(cmd, "cat /proc/%i/comm", pid);
			ExecuteCommand(cmd, vResult);
			if(vResult.size() > 0) 
				PKStringHelper::Safe_StrNCpy(pi->m_szName, vResult[0].c_str(), sizeof(pi->m_szName));  
			else
				printf("%d %s\n", pid, "No Result.");
		}
        pi->m_nPid = static_cast<unsigned int>(pid);
		vProcessInfo.push_back(pi); 

        if(nPid != -1)
            break;
	} 
	closedir(dir);
#endif
	return 0;
}

int DataAcquisition::TheProcessIDsByName(const char* szProcessName, vector<int> &vPids)
{
	vPids.clear();

#ifdef WIN32
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); 
	if (hSnapshot == INVALID_HANDLE_VALUE) 
		return -1; 

	PROCESSENTRY32 process; 
	process.dwSize = sizeof(PROCESSENTRY32); 
	if(!Process32First(hSnapshot, &process))
	{
		CloseHandle(hSnapshot);
		return -1;
	} 
	while(Process32Next(hSnapshot, &process))
	{  
		if (strcmp(process.szExeFile, szProcessName) == 0)
		{ 
			vPids.push_back(process.th32ProcessID);
		}
	} 
	CloseHandle(hSnapshot); 
#else  
	//not work for process-name like [name1/name2]
	//pidof -s [pname]
    vector<string> vResult;
	string cmdPath = string("pidof ") + string(szProcessName);	
	ExecuteCommand(cmdPath.c_str(), vResult);  
		
	if(vResult.size() == 0)
		return -1;	 
	 
	char* szToken = strtok(const_cast<char*>(vResult[0].c_str()), " ");
	while (szToken != NULL) 
	{ 
		int nPid = 0;
		sscanf(szToken, "%i", &nPid);
		vPids.push_back(nPid);
		szToken = strtok(NULL, " ");
	}
#endif
	return 0;
}

int DataAcquisition::TheProcessIDByNameAndCmdLine(const char* szProcessNameAndCmdLine, int &nPid)
{
	nPid = -1;

	int nLenProcessNameAndCmdLine = strlen(szProcessNameAndCmdLine);
	const char* szSplitCharFind = strchr(szProcessNameAndCmdLine, '|');
	if(szSplitCharFind == NULL || szProcessNameAndCmdLine[nLenProcessNameAndCmdLine - 1] == '|')
	{
		vector<int> vPids;
		int nRet = TheProcessIDsByName(szProcessNameAndCmdLine, vPids);
		if (vPids.size() > 0)
			nPid = vPids[0];
		return nRet;
	}

	//split by '|'
	int nLenProcessName = szSplitCharFind - szProcessNameAndCmdLine;
	char* szProcessName = new char[nLenProcessName + 1]();
	PKStringHelper::Safe_StrNCpy(szProcessName, szProcessNameAndCmdLine, nLenProcessName + 1);
	szSplitCharFind++;
	int nLenProcessCmdLine = strlen(szSplitCharFind);
	char* szProcessCmdLine = new char[nLenProcessCmdLine + 1]();
	PKStringHelper::Safe_StrNCpy(szProcessCmdLine, szSplitCharFind, nLenProcessCmdLine + 1);

#ifdef WIN32

	//user			
	int nLenProcessCommandLineWChars = (strlen(szProcessCmdLine) + 1) * 4;
	wchar_t* szProcessCommandLineWChars = new wchar_t[nLenProcessCommandLineWChars]();
	mbstowcs(szProcessCommandLineWChars, szProcessCmdLine, nLenProcessCommandLineWChars);
	int nProcessCommandLineArgs = 0;
	LPWSTR* szProcessCommandLineWCharsArgList = CommandLineToArgvW(szProcessCommandLineWChars, &nProcessCommandLineArgs);
	if(szProcessCommandLineWCharsArgList == NULL || nProcessCommandLineArgs == 0)
	{
		LocalFree(szProcessCommandLineWCharsArgList);
		delete[] szProcessCommandLineWChars;

		delete[] szProcessName;
		delete[] szProcessCmdLine; 

		return -1;
	}
		
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) 
	{
		LocalFree(szProcessCommandLineWCharsArgList);
		delete[] szProcessCommandLineWChars;

		delete[] szProcessName;
		delete[] szProcessCmdLine; 
		return -1; 
	}

	PROCESSENTRY32 process; 
	process.dwSize = sizeof(PROCESSENTRY32); 
	if(!Process32First(hSnapshot, &process))
	{ 
		CloseHandle(hSnapshot);

		LocalFree(szProcessCommandLineWCharsArgList);
		delete[] szProcessCommandLineWChars;

		delete[] szProcessName;
		delete[] szProcessCmdLine; 
		return -1;
	}

	while(Process32Next(hSnapshot, &process))
	{  
		if (strcmp(process.szExeFile, szProcessName) != 0)
			continue;
			
		char* szTheCommandLine = new char[COMMANDLINE_LENGTH]();
		SIZE_T nTheCommandLineLength = COMMANDLINE_LENGTH;
		if(TheCommandLineByProcessID(process.th32ProcessID, szTheCommandLine, nTheCommandLineLength) == -1)
		{
			delete[] szTheCommandLine;
			continue; 
		}
		//process
		int nLenTheCommandLineWChars = (strlen(szTheCommandLine) + 1) * 4;
		wchar_t* szTheCommandLineWChars = new wchar_t[nLenTheCommandLineWChars]();
		mbstowcs(szTheCommandLineWChars, szTheCommandLine, nLenTheCommandLineWChars);
		int nTheCommandLineArgs = 0;
		LPWSTR* szTheCommandLineWCharsArgList = CommandLineToArgvW(szTheCommandLineWChars, &nTheCommandLineArgs);
		if(szTheCommandLineWCharsArgList == NULL || nTheCommandLineArgs == 0)
		{
			LocalFree(szTheCommandLineWCharsArgList);

			delete[] szTheCommandLineWChars;

			delete[] szTheCommandLine;
			continue;
		}
		//analysis first args
		int nLenArgs0WChars = (wcslen(szTheCommandLineWCharsArgList[0]) + 1) * 4; 
		char* szArgs0Chars = new char[nLenArgs0WChars]();
		wcstombs(szArgs0Chars, szTheCommandLineWCharsArgList[0], nLenArgs0WChars);
		string strArgs0Chars(szArgs0Chars);
		//with .exe
		bool bArgs0CharsIsPathOrProcessName = strArgs0Chars.find(szProcessName) == string::npos ? false : true;
		//just 1
		if(bArgs0CharsIsPathOrProcessName && nTheCommandLineArgs == 1)
		{				
			delete[] szArgs0Chars;

			LocalFree(szTheCommandLineWCharsArgList);

			delete[] szTheCommandLineWChars;

			delete[] szTheCommandLine;
			continue;
		}
		//calc&compare args count
		if(nProcessCommandLineArgs != (bArgs0CharsIsPathOrProcessName ? (nTheCommandLineArgs - 1) : nTheCommandLineArgs))
		{
			delete[] szArgs0Chars;

			LocalFree(szTheCommandLineWCharsArgList);

			delete[] szTheCommandLineWChars;

			delete[] szTheCommandLine;
			continue;
		}
		//compare each one
		bool bIsMatched = true;
		for(int i = 0; i < nProcessCommandLineArgs; i++)
		{
			if(wcscmp(szProcessCommandLineWCharsArgList[i], szTheCommandLineWCharsArgList[(bArgs0CharsIsPathOrProcessName ? (i + 1) : i)]) == 0)
				continue;
			bIsMatched = false;
			break;
		}
		if(!bIsMatched)
		{
			delete[] szArgs0Chars;

			LocalFree(szTheCommandLineWCharsArgList);

			delete[] szTheCommandLineWChars;

			delete[] szTheCommandLine;
			continue;
		}
		//done
		nPid = process.th32ProcessID;
		break;
	}

	CloseHandle(hSnapshot);

	LocalFree(szProcessCommandLineWCharsArgList);
	delete[] szProcessCommandLineWChars;

	delete[] szProcessName;
	delete[] szProcessCmdLine; 

#else  
    vector<string> vResult;
	string cmdPath = string("ps -eo pid,cmd h | grep ") + string(szProcessName);	
    ExecuteCommand(cmdPath.c_str(), vResult);
		  
	if(vResult.size() == 1)
		return -1;	  
		 
	int nSizeProcess = vResult.size();
    for(int i = 0; i <= (nSizeProcess - 1); i++)
	{
		const char* szLine = vResult[i].c_str();

		int pid = 0;
		char szSecondBlank[1024] = { 0 };
		sscanf(szLine, "%i %s", &pid, szSecondBlank);

		if(szSecondBlank[0] == '[')//just process-name, goto:TheProcessIDsByName
		{
			continue;
		}
		else if(szSecondBlank[0] == '/')//path [cmdline], note:not work for the path which contains BLANK
		{
			string strSecondBlank = string(szSecondBlank);
			string::size_type posLastSplash = strSecondBlank.find_last_of('/');
			string tmpProcessName = strSecondBlank.substr(posLastSplash + 1);
			if(strcmp(szProcessName, tmpProcessName.c_str()) != 0)
				continue;
		}
		else//name [cmdline]
		{
			if(strcmp(szProcessName, szSecondBlank) != 0)
				continue;
		}
		string strJustCmd = vResult[i].substr(vResult[i].find_first_of(' ') + 1);//remove pid 
        //add-----
        int posLastSplash = strJustCmd.find_last_of('/');
        if(posLastSplash >= 0)
        {
            string strTmpCmd=strJustCmd.substr(posLastSplash+1+nLenProcessName+1);
          /*  int nIndex= strJustCmd.find_first_of(' ') + 1;
            string  pathcmd=strJustCmd.substr(nIndex);
            int ntmpI=pathcmd.find_first_of(' ') + 1;
            string cmd=pathcmd.substr(ntmpI);
            int nTmpc=cmd.find_first_of(' ')+1;
            string procmd=cmd.substr(nTmpc);*/
            g_logger.LogMessage(PK_LOGLEVEL_NOTICE,"szProcessCmdLine: %s,strTmpCmd :%s", szProcessCmdLine,strTmpCmd.c_str());
            if(strcmp(szProcessCmdLine,strTmpCmd.c_str())!=0)
                continue;
        }
#if 0
        else if(posLastSplash==0)
        {
            int nIndex= strJustCmd.find_first_of(' ') + 1;
            string  pathcmd=strJustCmd.substr(nIndex);
            if(strcmp(szProcessCmdLine,pathcmd.c_str())!=0)
                 continue;
        }
#endif
        //-----
        else
        {
             string::size_type posLastSplash = vResult[i].find_first_of(' ');
             if(posLastSplash==0)
             {
                   int nIndex= strJustCmd.find_first_of(' ') + 1;
                   string  pathcmd=strJustCmd.substr(nIndex);
                   int ntmpI=pathcmd.find_first_of(' ') + 1;
                   string cmd=pathcmd.substr(ntmpI);
                 //  int nTmpc=cmd.find_first_of(' ')+1;
                   //string procmd=cmd.substr(nTmpc);
                   g_logger.LogMessage(PK_LOGLEVEL_NOTICE,"first is null,szProcessCmdLine: %s,cmd :%s", szProcessCmdLine,cmd.c_str());
                   if(strcmp(szProcessCmdLine,cmd.c_str())!=0)
                       continue;
             }
             else
             {
               g_logger.LogMessage(PK_LOGLEVEL_NOTICE,"szProcessCmdLine: %s,strJustCmd:%s", szProcessCmdLine,strJustCmd.c_str());
               if(strcmp(szProcessCmdLine, strJustCmd.substr(strJustCmd.find_first_of(' ') + 1).c_str()) != 0)
                   continue;
             }
        }

       // if(strcmp(szProcessNameAndCmdLine, strJustCmd.substr(strJustCmd.find_first_of(' ') + 1).c_str()) != 0)
       // if(strcmp(szProcessCmdLine,strTmp2.c_str())!=0)

		nPid = pid;
		break;
	}
	
	delete[] szProcessCmdLine;
	delete[] szProcessName;

#endif
	return 0;
}
/*------ Process End ------*/


/*------PC Start------*/
bool DataAcquisition::Ping(const char *szIPAddress)
{
#ifdef WIN32	
	string cmd("cmd /C ping -n 3 -w 1024 ");
	cmd.append(szIPAddress); 
	//WinExec(cmd.c_str(), SW_HIDE); 
	return system(cmd.c_str()) == 0;
#else
	string cmd("ping -c 3 -W 1 ");
	cmd.append(szIPAddress);
	cmd += " | grep received";
	cmd += " | awk -F , '{ print $2 }' | awk '{ print $1 }'";
	vector<string> vResult;
	ExecuteCommand(cmd.c_str(), vResult);
	if(vResult.size() == 0)
		return false;
	if(vResult[0] == "0")
		return false;
	return true; 
#endif 
}

int	DataAcquisition::TheHostName(char* szHostName, unsigned long &nLength)
{
#ifdef WIN32  
	if(!GetComputerName(szHostName, &nLength)) //char*,DWORD*
		return -1; 
#else    
	if(gethostname(szHostName, nLength) == 0)
		nLength = strlen(szHostName);
	else return -1;
#endif
	return 0;
}

int DataAcquisition::TheUserName(char* szUserName, unsigned long &nLength)
{
#ifdef WIN32
	if(!GetUserName(szUserName, &nLength))//(w)char*,DWORD*
		return -1;
#else
	/* also getlogin()
	*/
	if(getlogin_r(szUserName, nLength) == 0)
		nLength = strlen(szUserName);
	else return -1;
#endif
	return 0;
}

int DataAcquisition::TheHostIPAddress(vector<string> &vIPAddress, unsigned short nFamily)
{
	vIPAddress.clear();

#ifdef WIN32 
	DWORD nSize = 0; 
	IP_ADAPTER_ADDRESSES *pAdapterAddress = NULL;
	GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, pAdapterAddress, &nSize); 
	pAdapterAddress = (IP_ADAPTER_ADDRESSES*)malloc(nSize); 
	if(GetAdaptersAddresses(nFamily, GAA_FLAG_INCLUDE_PREFIX, NULL, pAdapterAddress, &nSize) != ERROR_SUCCESS) 
	{
		free(pAdapterAddress);
		return -1;
	} 

	IP_ADAPTER_ADDRESSES *pAdapter = pAdapterAddress; 
	for(; pAdapter; pAdapter = pAdapter->Next)
	{
		// Skip loopback adapters
		if (IF_TYPE_SOFTWARE_LOOPBACK == pAdapter->IfType) 
		  continue;  
		// Parse all IPv4 and IPv6 addresses
		for(IP_ADAPTER_UNICAST_ADDRESS* pAddress = pAdapter->FirstUnicastAddress; pAddress; pAddress = pAddress->Next)
		{
			auto family = pAddress->Address.lpSockaddr->sa_family;
			if (AF_INET == family)
			{
				// IPv4
				SOCKADDR_IN* Ipv4 = reinterpret_cast<SOCKADDR_IN*>(pAddress->Address.lpSockaddr);
				vIPAddress.push_back(inet_ntoa(Ipv4->sin_addr));
				//char v4Buf[16] = { 0 };
				//InetNtop(AF_INET, &(Ipv4->sin_addr), v4Buf, sizeof(v4Buf));  
				//vIPAddress.push_back(string(v4Buf));
			}
			else if(AF_INET6 == family)
			{
				// IPv6
				SOCKADDR_IN6* Ipv6 = reinterpret_cast<SOCKADDR_IN6*>(pAddress->Address.lpSockaddr); 
				char v6Buf[64] = { 0 }; 
				DWORD nAddrSize = sizeof(sockaddr_storage);
				WSAAddressToString((LPSOCKADDR)Ipv6, nAddrSize, NULL, v6Buf, &nAddrSize);
				//WCHAR  v6Buf[64] = { 0 }; 
				//InetNtop(AF_INET6, &(Ipv6->sin6_addr), v6Buf, sizeof(v6Buf));  
				vIPAddress.push_back(string(v6Buf));
			}
		}
	}
	free(pAdapterAddress);
#else 
  	/* also https://linux.die.net/man/3/getnameinfo
	*/
	addrinfo hints, *result, *next; 
  	memset(&hints, 0, sizeof(hints));
  	hints.ai_family = nFamily;//PF_UNSPEC (IPv4 r IPv6)
  	hints.ai_flags |= AI_CANONNAME;
  	hints.ai_socktype = SOCK_STREAM;//SOCK_DGRAM
 
	char szHostName[256] = { 0 };
	unsigned long length = sizeof(szHostName);
	TheHostName(szHostName, length);
	
  	if(getaddrinfo(szHostName, NULL, &hints, &result) != 0) 
      	return -1; 
      	
  	char szAddress[128] = { 0 };
	for(next = result; next != NULL; next = next->ai_next)
	{
		void *addr;
		switch(next->ai_family)
		{
			case AF_INET:
	  			addr = &(((sockaddr_in*)next->ai_addr)->sin_addr);
	  		break;
			case AF_INET6:
	  			addr = &(((sockaddr_in6*)next->ai_addr)->sin6_addr);
	  		break;
		}		
		//convert ip address to human-readable form
		inet_ntop(next->ai_family, addr, szAddress, sizeof(szAddress));
		vIPAddress.push_back(string(szAddress));
	}
   	freeaddrinfo(result);
	return vIPAddress.size(); 
#endif
	return 0;
}
/*------ PC End ------*/


#ifdef WIN32

#else

#endif
