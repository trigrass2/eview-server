////////////////////////////////////////////////////////////////
// MSDN Magazine -- August 2002
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
// Compiles with Visual Studio 6.0 on Windows XP.
//
#ifdef _WIN32
#include "EnumProc.h"
#include "psapi.h"
#include "assert.h"
#include <Tlhelp32.h>
#include "windows.h"
//#if(WINVER >= 0x0500)
#define SMTO_NOTIMEOUTIFNOTHUNG 0x0008
//#endif /* WINVER >= 0x0500 */

//////////////////
// Search for process whose module name matches parameter.
// Finds "foo" or "foo.exe"
// 仅仅能够找到32位程序。pdb是64位的所以无法找到！！
DWORD CFindKillProcess::GetProcessID(LPCTSTR szProcessName, BOOL bAddExeExt)
{
	char szProcessNameExt[MAX_PATH] = { 0 };
	strcpy(szProcessNameExt, szProcessName);
	if (bAddExeExt)
		strcat(szProcessNameExt, ".exe");

	PROCESSENTRY32 processEntry32;
	HANDLE toolHelp32Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (((int)toolHelp32Snapshot) != -1)
	{
		processEntry32.dwSize = sizeof(processEntry32);
		if (Process32First(toolHelp32Snapshot, &processEntry32))
		{
			do
			{
				if (stricmp(szProcessNameExt, processEntry32.szExeFile) == 0)
				{
					return processEntry32.th32ProcessID;
				}
			} while (Process32Next(toolHelp32Snapshot, &processEntry32));
		}
		CloseHandle(toolHelp32Snapshot);
	}

	return -1;
}


////////////////////////////////////////////////////////////////
// CProcessIterator - Iterates all processes
//
CProcessIterator::CProcessIterator()
{
	m_pids = NULL;
}

CProcessIterator::~CProcessIterator()
{
	delete[] m_pids;
}

//////////////////
// Get first process: Call EnumProcesses to init array. Return first one.
//
DWORD CProcessIterator::First()
{
	m_current = (DWORD)-1;
	m_count = 0;
	DWORD nalloc = 1024;
	do {
		delete[] m_pids;
		m_pids = new DWORD[nalloc];
		if (EnumProcesses(m_pids, nalloc*sizeof(DWORD), &m_count)) {
			m_count /= sizeof(DWORD);
			m_current = 1;						 // skip IDLE process
		}
	} while (nalloc <= m_count);

	return Next();
}

////////////////////////////////////////////////////////////////
// CProcessModuleIterator - Iterates all modules in a process
//
CProcessModuleIterator::CProcessModuleIterator(DWORD pid)
{
	m_hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
		FALSE, pid);
}

CProcessModuleIterator::~CProcessModuleIterator()
{
	CloseHandle(m_hProcess);
	delete[] m_hModules;
}

//////////////////
// Get first module in process. Call EnumProcesseModules to
// initialize entire array, then return first module.
//
HMODULE CProcessModuleIterator::First()
{
	m_count = 0;
	m_current = (DWORD)-1;
	m_hModules = NULL;
	if (m_hProcess) {
		DWORD nalloc = 1024;
		do {
			delete[] m_hModules;
			m_hModules = new HMODULE[nalloc];
			if (EnumProcessModules(m_hProcess, m_hModules,
				nalloc*sizeof(DWORD), &m_count)) {
				m_count /= sizeof(HMODULE);
				m_current = 0;
			}
		} while (nalloc <= m_count);
	}
	return Next();
}

////////////////////////////////////////////////////////////////
// CWindowIterator - Iterates all top-level windows (::EnumWindows)
//
CWindowIterator::CWindowIterator(DWORD nAlloc)
	: m_current(0), m_count(0)
{
	assert(nAlloc>0);
	m_hwnds = new HWND[nAlloc];
	m_nAlloc = nAlloc;
}

CWindowIterator::~CWindowIterator()
{
	delete[] m_hwnds;
}

// callback passes to virtual fn
BOOL CALLBACK CWindowIterator::EnumProc(HWND hwnd, LPARAM lp)
{
	return ((CWindowIterator*)lp)->OnEnumProc(hwnd);
}

//////////////////
// Virtual enumerator proc: add HWND to array if OnWindows is TRUE.
// You can override OnWindow to filter windows however you like.
//
BOOL CWindowIterator::OnEnumProc(HWND hwnd)
{
	if (OnWindow(hwnd)) {
		if (m_count < m_nAlloc)
			m_hwnds[m_count++] = hwnd;
	}
	return TRUE; // keep looking
}

////////////////////////////////////////////////////////////////
// CMainWindowIterator - Iterates the main windows of a process.
// Overrides CWindowIterator::OnWindow to filter.
//
CMainWindowIterator::CMainWindowIterator(DWORD pid, BOOL bVis,
	DWORD nAlloc) : CWindowIterator(nAlloc), m_pid(pid), m_bVisible(bVis)
{
}

CMainWindowIterator::~CMainWindowIterator()
{
}

//////////////////
// OnWindow:: Is window's process ID the one i'm looking for?
// Set m_bVisible=FALSE to find invisible windows too.
//
BOOL CMainWindowIterator::OnWindow(HWND hwnd)
{
	if (!m_bVisible || (GetWindowLong(hwnd, GWL_STYLE) & WS_VISIBLE)) {
		DWORD pidwin;
		GetWindowThreadProcessId(hwnd, &pidwin);
		if (pidwin == m_pid)
			return TRUE;
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////
// CFindKillProcess - to find/kill a process by module name.
//
CFindKillProcess::CFindKillProcess()
{
	m_bOnlyWaitOnceOnKillProcess = FALSE;//关闭多个进程时，在关闭进程之前仅仅等待一次
	m_bFirstWaitKillProcess = TRUE;//是否第一次关闭进程

}

CFindKillProcess::~CFindKillProcess()
{
}

void RaisePrivilege()
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	//
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken))
	{
		if (LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid))
		{
			AdjustTokenPrivileges(hToken, FALSE, &tp, NULL, NULL, 0);
		}
	}

	if (hToken)
		CloseHandle(hToken);
}

//////////////////
// Search for process whose module name matches parameter.
// Finds "foo" or "foo.exe"
// 仅仅能够找到32位程序。pdb是64位的所以无法找到！！
DWORD CFindKillProcess::GetProcessIDOnlyWin32(LPCTSTR modname, BOOL bAddExe)
{
	RaisePrivilege();
	CProcessIterator itp;
	for (DWORD pid = itp.First(); pid; pid = itp.Next()) {
		TCHAR name[_MAX_PATH];
		CProcessModuleIterator itm(pid);
		HMODULE hModule = itm.First(); // .EXE
		if (hModule) {
			GetModuleBaseName(itm.GetProcessHandle(),
				hModule, name, _MAX_PATH);

			CString strDestModName = modname;
			if (strDestModName.Find(".exe") <= 0)
				strDestModName = strDestModName + ".exe";
			CString sModName = GetFileName(strDestModName);
			if (strcmpi(sModName.GetBuffer(0), name) == 0)
				return pid;
			sModName += ".exe";
			if (bAddExe && strcmpi(sModName.GetBuffer(0), name) == 0)
				return pid;
		}
	}
	return 0;
}

//////////////////
// Kill a process cleanly: Close main windows and wait.
// bZap=TRUE to force kill.
//
BOOL CFindKillProcess::KillProcess(DWORD pid, BOOL bZap, DWORD dwWaitTime)//dwWaitTime强制关闭时的等待时间
{
	CMainWindowIterator itw(pid, FALSE);
	for (HWND hwnd = itw.First(); hwnd; hwnd = itw.Next()) {
		DWORD_PTR bOKToKill = FALSE;
		SendMessageTimeout(hwnd, WM_QUERYENDSESSION, 0, 0,
			SMTO_ABORTIFHUNG | SMTO_NOTIMEOUTIFNOTHUNG, 100, &bOKToKill);
		if (!bOKToKill)
			return FALSE;  // window doesn't want to die: abort

		//////////////////////////////////////////////////////////////////////////
		// fengjuanyong: only close top-level window

		HWND hwndParent = (HWND)GetWindowLong(hwnd, GWLP_HWNDPARENT);
		if (!IsWindow(hwndParent))
		{
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			//long nThreadID = GetWindowThreadProcessId(hwnd, NULL);
			//PostThreadMessage(nThreadID, WM_CLOSE, 0, 0);
		}
	}

	//如果仅仅允许等待一次，且是第一次等待杀线程，则利用等待时间；否则等待时间为0
	if (!(m_bOnlyWaitOnceOnKillProcess && m_bFirstWaitKillProcess))
		dwWaitTime = 0;

	// I've closed the main windows; now wait for process to die. 
	BOOL bKilled = TRUE;
	HANDLE hp = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, pid);
	if (hp) {
		if (WaitForSingleObject(hp, dwWaitTime) != WAIT_OBJECT_0)
		{
			if (bZap)
			{
				m_bFirstWaitKillProcess = FALSE;//非正常关闭
				// didn't die: force kill it if zap requested
				TerminateProcess(hp, 0);
			}
			else
			{
				bKilled = FALSE;
			}
		}
		CloseHandle(hp);
	}
	return bKilled;
}


BOOL CFindKillProcess::ShowProcessWindow(DWORD pid, int nShowFlag)
{
	BOOL bRetVal = FALSE;
	CMainWindowIterator itw(pid, FALSE);
	for (HWND hwnd = itw.First(); hwnd; hwnd = itw.Next()) {
		bRetVal = ShowWindow(hwnd, nShowFlag);
	}
	return bRetVal;
}


/************************************************************************/
/*  将路径中的文件名称提炼出来  added by fanyun 2004-9-28               */
/************************************************************************/
CString CFindKillProcess::GetFileName(CString strPath)
{
	CString strFileName = _T("");

	int nLastIndex = -1;
	int nPathLength = strPath.GetLength();
	for (int i = 0; i<nPathLength; i++)
	{
		CString p = strPath.GetAt(i);
		if (strPath.GetAt(i) == '\\')
		{
			nLastIndex = i;
		}
	}

	strFileName = strPath.Right(nPathLength - nLastIndex - 1);//得到文件名称

	return strFileName;
}

#endif // _WIN32