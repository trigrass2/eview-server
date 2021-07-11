// CVProcess.h: interface for the CCVProcess class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CVPROCESS_H__CBC0E21E_C7B1_48EC_AE9B_50D60309B2B4__INCLUDED_)
#define AFX_CVPROCESS_H__CBC0E21E_C7B1_48EC_AE9B_50D60309B2B4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <ace/Process.h>
#include <ace/Singleton.h>
#include <ace/Process_Mutex.h>
#include <ace/Time_Value.h>
#include "common/eviewdefine.h"
#include "errcode/ErrCode_iCV_Common.hxx"
#include <ace/Pipe.h>
#include "shmqueue/ProcessQueue.h"

#define MODULE_RUNAT_PLATFORM_WINDOWS				1	// 在windows运行
#define MODULE_RUNAT_PLATFORM_UNIX					2	// 在非windows运行，包括solaris和linux
#define MODULE_RUNAT_PLATFORM_ALL					0	// 在所有平台运行

#define EC_ICV_PROCMGR_FAILTOSTRTPROCESS			0241
#define EC_ICV_PROCMGR_PROCESSSTARTEDALAREADY		0242
//#define EC_ICV_PROCMGR_FAILTOSTOPROCESSS			0243
#define EC_ICV_PROCMGR_FAILTOOPENOCFGFILE			0244
#define EC_ICV_PROCMGR_XMLREQUESTERROR				0245
#define EC_ICV_PROCMGR_PROCESS_STILL_RUNNING		0246

#include <string>
#include <list>
using namespace std;

class CPKProcessMgr;
typedef list<string> stringlist;
class CPKProcess  : public ACE_Process
{
public:
	char m_szExeName[PK_LONGFILENAME_MAXLEN];
	char m_szExeDir[PK_LONGFILENAME_MAXLEN];
	char m_szStartCmd[PK_LONGFILENAME_MAXLEN]; // 可以为空
	char m_szStopCmd[PK_LONGFILENAME_MAXLEN];	// 可以为空
	char m_szDispName[PK_LONGFILENAME_MAXLEN];
	char m_szWorkingDir[PK_LONGFILENAME_MAXLEN];	// 可以为空

	// 绝对路径
	char	m_szExeAbsPath[PK_LONGFILENAME_MAXLEN]; // exe绝对路径,如：d:\peak\bin\drivers\modbus\modbus.exe
	char	m_szStartCmdAbsPath[PK_LONGFILENAME_MAXLEN * 3]; // exe绝对路径,如：d:\peak\bin\drivers\modbus\start.bat
	char	m_szStopCmdAbsPath[PK_LONGFILENAME_MAXLEN * 3]; // exe绝对路径,如：d:\peak\bin\drivers\modbus\stop.bat
	char	m_szWorkingDirAbsPath[PK_LONGFILENAME_MAXLEN]; // exe绝对路径,如：d:\peak\bin\drivers\modbus\

	bool m_bShowConsole;
	bool m_bAlive; // 结合Exist，状态：alive、notexist、stopped
	bool m_bExeExist;	// 进程文件exe是否存在
	pid_t m_pid;
	bool m_bIsDriverExe;	// 是否是驱动
public:
	CPKProcess(char *szExeName, char *szExeDir, char *szDispName, char *szStartCmd, char *szStopCmd, char *szRunAtPlatForm, long lSleepTime, CPKProcessMgr *pProcessMgr);
	virtual ~CPKProcess();

public:
	// void CheckOutBuffer();

	int		m_nRestartCount; // 重启次数
	string	m_strLastRestartTime; // 上次尝试重启的时间
	ACE_Pipe   m_pipeOut;
	CPKProcessMgr *m_pProcessMgr;
	time_t	m_tmLastFailTime;
public:
	int SetAliveStatus();
	long StartProcess();
	long NotifyProcessToStop(bool bDisableRestart, bool bDisplayLog);
	long NotifyProcessToRefresh();
	long WaitProcessToStop(ACE_Time_Value &tvTimeout, bool bForceStop);
	bool IsProcessAlive();
	void DumpProcessInfo();
	bool IsMemDB();
	void RestartProcess();

	virtual int prepare(ACE_Process_Options &options);
	long CV_ProcessMutex_Acquire(ACE_Process_Mutex *pMutex, ACE_Time_Value tvTimeout); // 由于Solaris下超时获取锁函数不能正常工作，因此自己提供一个

	long GetPlatformName(char *szPlatform, long lBuffLen);

	const char *GetModuleName() const {return m_szExeName;};

	void ToggleConsoleStatus();

	void ManualStopProcess(int nWaitSecondsBeforeForceExit);

	bool IsShowConsole();

	void GetProcessState(string &strStatus);
	int UpdateStatusInPdb(bool bAlive);
	pid_t GetProcessId();
	int DelManageTagInPdb();
	int InitLogConfTagInPdb();
	string GetAbsFilePath(string strExeOrBatFile, bool bIsDriver);
	int GenerateAbsPaths(bool bIsDriver); // 产生exe全路径、StartCmd全路径、StopCmd全路径、WorkdingDir全路径
	bool IsBatchFile(); // StartCmd是否是bat file
	bool IsWebServer();
public:
	char m_szArguments[PK_LONGFILENAME_MAXLEN];
    int m_lRunAtPlatform;
    int m_lSleepSeconds;
	ACE_Process_Mutex	*m_pMutexProcessAlive;	// 本子进程是否还活着的信号量。如果获取不到说明还活着
	ACE_Process_Mutex	*m_pMutexNotifyQuit;		// 本子进程是否需要退出的信号量。如果本子进程获取不到说明需要退出

	bool m_bRestartEnable;
	bool m_bRestartEnableInit;			// 初始配置的是否运行重启选项			
	//bool m_bManualStopped;
	bool m_bShouldUpdateStatusInPdb;	// 是否需要在pdb更新状态。pdb自己显然是不需要的，但其他几个进程需要
	bool m_bBatchFile;
private:
	CProcessQueue * m_pProcessQue;
	//int _UpdatePdbInfo();
    long StartBatchProcess();
	long StopBatProcess(bool bDisplayLog);
	int UpdateTagsOfDriverToBad();
};


#endif // !defined(AFX_CVPROCESS_H__CBC0E21E_C7B1_48EC_AE9B_50D60309B2B4__INCLUDED_)
