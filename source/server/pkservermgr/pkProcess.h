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

#define MODULE_RUNAT_PLATFORM_WINDOWS				1	// ��windows����
#define MODULE_RUNAT_PLATFORM_UNIX					2	// �ڷ�windows���У�����solaris��linux
#define MODULE_RUNAT_PLATFORM_ALL					0	// ������ƽ̨����

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
	char m_szStartCmd[PK_LONGFILENAME_MAXLEN]; // ����Ϊ��
	char m_szStopCmd[PK_LONGFILENAME_MAXLEN];	// ����Ϊ��
	char m_szDispName[PK_LONGFILENAME_MAXLEN];
	char m_szWorkingDir[PK_LONGFILENAME_MAXLEN];	// ����Ϊ��

	// ����·��
	char	m_szExeAbsPath[PK_LONGFILENAME_MAXLEN]; // exe����·��,�磺d:\peak\bin\drivers\modbus\modbus.exe
	char	m_szStartCmdAbsPath[PK_LONGFILENAME_MAXLEN * 3]; // exe����·��,�磺d:\peak\bin\drivers\modbus\start.bat
	char	m_szStopCmdAbsPath[PK_LONGFILENAME_MAXLEN * 3]; // exe����·��,�磺d:\peak\bin\drivers\modbus\stop.bat
	char	m_szWorkingDirAbsPath[PK_LONGFILENAME_MAXLEN]; // exe����·��,�磺d:\peak\bin\drivers\modbus\

	bool m_bShowConsole;
	bool m_bAlive; // ���Exist��״̬��alive��notexist��stopped
	bool m_bExeExist;	// �����ļ�exe�Ƿ����
	pid_t m_pid;
	bool m_bIsDriverExe;	// �Ƿ�������
public:
	CPKProcess(char *szExeName, char *szExeDir, char *szDispName, char *szStartCmd, char *szStopCmd, char *szRunAtPlatForm, long lSleepTime, CPKProcessMgr *pProcessMgr);
	virtual ~CPKProcess();

public:
	// void CheckOutBuffer();

	int		m_nRestartCount; // ��������
	string	m_strLastRestartTime; // �ϴγ���������ʱ��
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
	long CV_ProcessMutex_Acquire(ACE_Process_Mutex *pMutex, ACE_Time_Value tvTimeout); // ����Solaris�³�ʱ��ȡ������������������������Լ��ṩһ��

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
	int GenerateAbsPaths(bool bIsDriver); // ����exeȫ·����StartCmdȫ·����StopCmdȫ·����WorkdingDirȫ·��
	bool IsBatchFile(); // StartCmd�Ƿ���bat file
	bool IsWebServer();
public:
	char m_szArguments[PK_LONGFILENAME_MAXLEN];
    int m_lRunAtPlatform;
    int m_lSleepSeconds;
	ACE_Process_Mutex	*m_pMutexProcessAlive;	// ���ӽ����Ƿ񻹻��ŵ��ź����������ȡ����˵��������
	ACE_Process_Mutex	*m_pMutexNotifyQuit;		// ���ӽ����Ƿ���Ҫ�˳����ź�����������ӽ��̻�ȡ����˵����Ҫ�˳�

	bool m_bRestartEnable;
	bool m_bRestartEnableInit;			// ��ʼ���õ��Ƿ���������ѡ��			
	//bool m_bManualStopped;
	bool m_bShouldUpdateStatusInPdb;	// �Ƿ���Ҫ��pdb����״̬��pdb�Լ���Ȼ�ǲ���Ҫ�ģ�����������������Ҫ
	bool m_bBatchFile;
private:
	CProcessQueue * m_pProcessQue;
	//int _UpdatePdbInfo();
    long StartBatchProcess();
	long StopBatProcess(bool bDisplayLog);
	int UpdateTagsOfDriverToBad();
};


#endif // !defined(AFX_CVPROCESS_H__CBC0E21E_C7B1_48EC_AE9B_50D60309B2B4__INCLUDED_)
