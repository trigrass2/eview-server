// CVProcess.cpp: implementation of the CCVProcess class.
//
//////////////////////////////////////////////////////////////////////
#include "ace/ACE.h"
#include "ace/OS_NS_sys_time.h"
#include <ace/OS_NS_stdio.h>
#include <ace/OS_NS_sys_stat.h>
#include "pklog/pklog.h"
#include "pkcomm/pkcomm.h"
#include "common/RMAPI.h"
#include "ProcessMngr.h"
#include "pkProcess.h"
#include "MainTask.h"
#include "../pknodeserver/RedisFieldDefine.h"
#ifdef _WIN32
#include "shellapi.h"
#endif

extern CPKLog PKLog;
#include "pkeviewdbapi/pkeviewdbapi.h"

#include <algorithm>
#ifndef _WIN32
#include <ace/OS_NS_dirent.h>
#include <ace/OS_NS_stdio.h>
#endif
#ifdef __sun
#include <procfs.h>
#endif

#define PK_DATESTRING_MAXLEN			11	// ���ڵ���󳤶�(��ʽYYYY-MM-DD)����󳤶�
#define ICV_SCANTIMESTRING_MAXLEN		13	// ʱ�����󳤶�(��ʽhh.mm.ss.xxx)����󳤶�
#define PROCESSMGR_RUNTIME_DIR_NAME ACE_TEXT("PROCESSMGR") 
#define PROCESSMGR_CMDTYPE_REFRESH			1
#define PROCESSMGR_CMDTYPE_STOP				2

#define NULLASSTRING(x) x==NULL?"":x

#ifdef _WIN32
#include "EnumProc.h"
static CFindKillProcess g_findKillProcess;
#endif

#define INVALID_PROCESS_ID							-1
#define PROCESSMGR_CONFIG_FILENAME					"ProcessMgr.xml"

#define WAIT_AFTER_PROCESS_EXIT                     3

extern ACE_HANDLE g_hOutput;
//#ifndef _WIN32
//static ACE_HANDLE g_hOutput = NULL;//ACE_OS::open("/dev/null", 0);
//#else
//static ACE_HANDLE g_hOutput = NULL;//ACE_OS::open("/dev/null", 0);
//#endif

#ifndef _WIN32
#  ifdef __hpux
#include <sys/pstat.h>
#include <sys/types.h>
int FindPidByName(const char* szProcName, std::list<pid_t> &listPid)
{
	if (NULL == szProcName)
		return -1;

	static const size_t BURST = 10;
	struct pst_status pst[BURST];
	int i, count;
	int idx = 0; /* pstat index within the Process pstat context */
	while ((count = pstat_getproc(pst, sizeof(pst[0]),BURST,idx))>0) 
	{
		/* got count this time.   process them */
		for (i = 0; i < count; i++) 
		{
			if (ACE_OS::strstr(pst[i].pst_cmd, szProcName) != NULL)
			{
				listPid.push_back(pst[i].pst_pid);
			}
		}

		/*
			* now go back and do it again, using the next index after
			* the current 'burst'
			*/
		idx = pst[count-1].pst_idx + 1;
		(void)memset(pst,0,BURST*sizeof(struct pst_status));
	}
	if (count == -1)
		return -1;
	return 0;
}
#  else  // (__hpux)
/**
 *  $(Desp)��δ�����ժ��linux��pidof���� .
 *  $(Detail) .
 *
 *  @param		-[in]  char * szProcName : ��������
 *  @param		-[out] std::list<int> &listPid : ����pid�б�
 *  @return		int.
 *
 *  @version	8/29/2013  shijunpu  Initial Version.
 */
int FindPidByName( const char* szProcName, std::list<pid_t> &listPid)  // ARM-LINUX�½��������15���ֽڣ������Ľضϡ���pkmqttforwardserver��ض�Ϊ��pkmqttforwardse
{  
	if (NULL == szProcName)
		return -1;

	/* Open the /proc directory. */  
	ACE_DIR *dir = ACE_OS::opendir("/proc");  
	if (!dir)  
	{  
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "cannot open /proc");
		return -1;  
	}  

	struct ACE_DIRENT *d = NULL;  
	/* Walk through the directory. */  
	while ((d = ACE_OS::readdir(dir)) != NULL) 
	{  
		char *s = NULL; 
		char *q = NULL;
		char path [PATH_MAX+1] = {0};  
		char buf[PATH_MAX+1] = {0}; 
		int len = 0;  
		int namelen = 0;  
		int pid = 0;
		FILE *fp = NULL;

		/* See if this is a process */  
		if ((pid = ACE_OS::atoi(d->d_name)) == 0) 
			continue;  
#ifdef __sun 
		ACE_OS::snprintf(path, sizeof(path), "/proc/%s/psinfo", d->d_name);
		if ((fp = ACE_OS::fopen(path, "r")) != NULL)
		{                  
			psinfo_t psinfo;
			memset(&psinfo, 0x00, sizeof(psinfo));
			ACE_OS::fgets((char*)&psinfo, sizeof(psinfo), fp);
			ACE_OS::fclose(fp);
			if (ACE_OS::strstr(psinfo.pr_fname, szProcName) != NULL)
			{  
				listPid.push_back(pid);
			}  
		} 
#else
		ACE_OS::snprintf(path, sizeof(path), "/proc/%s/stat", d->d_name);  
		  /* Read SID & statname from it. */                 
		if ((fp = ACE_OS::fopen(path, "r")) != NULL) 
		{                         
			buf[0] = 0;
			ACE_OS::fgets(buf, sizeof(buf), fp);
			ACE_OS::fclose(fp);
			/* See if name starts with '(' */
			s = buf; 
			while (*s != ' ') s++; 
			s++; 
			if (*s == '(') 
			{   
				/* Read program name. */
				q = ACE_OS::strrchr(buf, ')');
				if (q == NULL) 
					continue;      
				s++;  
			} 
			else 
			{  
				q = s;  
				while (*q != ' ') q++;  
			}        
			*q = 0;  

			// ARM-LINUX�½��������15���ֽڣ������Ľضϡ���pkmqttforwardserver��ض�Ϊ��pkmqttforwardse
			char szTrimProcName[16] = { 0 };
			strncpy(szTrimProcName, szProcName, sizeof(szTrimProcName) - 1); // Ҫ��ѯ״̬�Ľ��̣� ��ౣ��15���ֽ�
			char szTrimAliveProcName[16] = { 0 };
			strncpy(szTrimAliveProcName, s, sizeof(szTrimAliveProcName) - 1); //��ǰ�������еĽ��̣� ��ౣ��15���ֽ�

			if (!PKStringHelper::StriCmp(szTrimProcName, szTrimAliveProcName)) // ���ǰ15���ֽ���ȣ������ִ�Сд������Ϊ��ȣ�
			{  
				listPid.push_back(pid);
			}  
		}
#endif
	}
	ACE_OS::closedir(dir);

	return  0;  
} 
#  endif // (__hpux)
#endif
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPKProcess::~CPKProcess()
{
	m_pMutexProcessAlive->release();
	m_pMutexNotifyQuit->release();
	m_pMutexProcessAlive->remove();
	m_pMutexNotifyQuit->remove();
	delete m_pMutexProcessAlive;
	delete m_pMutexNotifyQuit;
	m_bRestartEnable = true;
	m_bShouldUpdateStatusInPdb = true;
	m_pipeOut.close();

	//m_strlistOutBuff.clear();
}

CPKProcess::CPKProcess(char *szExeName, char *szExeDir, char *szDispName, char *szStartCmd, char *szStopCmd,
					   char *szRunAtPlatForm, long lSleepTime, CPKProcessMgr *pProcessMgr)
{
	memset(m_szDispName, 0, sizeof(m_szDispName));
	memset(m_szExeName, 0, sizeof(m_szExeName));
	memset(m_szStartCmd, 0, sizeof(m_szStartCmd));
	memset(m_szStopCmd, 0, sizeof(m_szStopCmd));
	memset(m_szWorkingDir, 0, sizeof(m_szWorkingDir));
	memset(m_szExeAbsPath, 0, sizeof(m_szExeAbsPath));
	memset(m_szStartCmdAbsPath, 0, sizeof(m_szStartCmdAbsPath));
	memset(m_szStopCmdAbsPath, 0, sizeof(m_szStopCmdAbsPath));
	memset(m_szWorkingDirAbsPath, 0, sizeof(m_szWorkingDirAbsPath));

	//���ڽ��̻״̬��⣬��ͨ�������жϵģ��ֽ���������ͳһ�޸�ΪСд
	//�������û����ý��̹�����ʱ������������Դ�Сд
	std::string strModuleName(szExeName);
	transform(strModuleName.begin(), strModuleName.end(), strModuleName.begin(),(int (*)(int))tolower);
	std::string strExeDir(szExeDir);
	transform(strExeDir.begin(), strExeDir.end(), strExeDir.begin(), (int(*)(int))tolower);

	char szAliveMutexName[PK_LONGFILENAME_MAXLEN];
	sprintf(szAliveMutexName, "Global\\PROC_%s_ALIVE_ALREADY", strModuleName.c_str());	// Global\\PROC_%s_ALIVE_ALREADY
	char szExitMutexName[PK_LONGFILENAME_MAXLEN];
	sprintf(szExitMutexName, "Global\\PROC_%s_NOTIFY_QUIT", strModuleName.c_str());

	m_pMutexProcessAlive = new ACE_Process_Mutex(szAliveMutexName);
	m_pMutexNotifyQuit = new ACE_Process_Mutex(szExitMutexName);
	strncpy(m_szExeName, strModuleName.c_str(), sizeof(m_szExeName) - 1);
	strncpy(m_szDispName, szDispName, sizeof(m_szDispName) - 1);
	strncpy(m_szExeDir, strExeDir.c_str(), sizeof(m_szExeDir) - 1);
	// ��������
	string strRunTimePath = PKComm::GetRunTimeDataPath();
	ACE_OS::mkdir((strRunTimePath).c_str());
	strRunTimePath += ACE_DIRECTORY_SEPARATOR_STR PROCESSMGR_RUNTIME_DIR_NAME;
	ACE_OS::mkdir((strRunTimePath).c_str()); 

	m_pProcessQue = new CProcessQueue((char *)strRunTimePath.c_str(), (char *)strModuleName.c_str());
    unsigned int nQueToProcessSize = PROCESSQUEUE_PROCESSMGR_CONTROL_AREASIZE;
    m_pProcessQue->Open(true, nQueToProcessSize);
	long lRunAtPlatForm = MODULE_RUNAT_PLATFORM_ALL;
	string strPlatform = szRunAtPlatForm;
	transform(strPlatform.begin(), strPlatform.end(), strPlatform.begin(), (int (*)(int))tolower);
	if(strPlatform.find("win") != string::npos)
		lRunAtPlatForm = MODULE_RUNAT_PLATFORM_WINDOWS;
	else if(strPlatform.find("linux") != string::npos || strPlatform.find("unix") != string::npos || strPlatform.find("ux") != string::npos)
		lRunAtPlatForm = MODULE_RUNAT_PLATFORM_UNIX;
	m_lRunAtPlatform = lRunAtPlatForm;

	m_bAlive = false;	// �����Ƿ�������
	m_bExeExist = true;	// �ļ��Ƿ����
	memset(m_szArguments, 0x00, sizeof(m_szArguments));
	if(szStartCmd)
		strncpy(m_szStartCmd, szStartCmd, sizeof(m_szStartCmd) - 1);

	if(szStopCmd)
		strncpy(m_szStopCmd, szStopCmd, sizeof(m_szStopCmd) - 1);

	m_nRestartCount = 0;
	m_strLastRestartTime = "";
	m_pProcessMgr = pProcessMgr;
	m_bBatchFile = false;
	m_strLastRestartTime = "";
	m_pid = INVALID_PROCESS_ID;
	m_lSleepSeconds = lSleepTime;
	m_bRestartEnable = true;
	m_bShowConsole = false;
	m_bIsDriverExe = false;
}

long CPKProcess::GetPlatformName(char *szPlatform, long lBuffLen)
{
	switch (m_lRunAtPlatform)
	{
	case MODULE_RUNAT_PLATFORM_WINDOWS:
		strncpy(szPlatform, "Windows", lBuffLen - 1);
		break;
	case MODULE_RUNAT_PLATFORM_UNIX:
		strncpy(szPlatform, "Unix(Linux)", lBuffLen - 1);
		break;
	case MODULE_RUNAT_PLATFORM_ALL:
		strncpy(szPlatform, "All", lBuffLen - 1);
		break;
	default:
		ACE_OS::snprintf(szPlatform, lBuffLen, "(%d) Unknown, assume all", m_lRunAtPlatform);
		break;
	}
	return PK_SUCCESS;
}

void CPKProcess::DumpProcessInfo()
{
	char szPlatform[PK_LONGFILENAME_MAXLEN];
	GetPlatformName(szPlatform, sizeof(szPlatform));
#ifdef _WIN32
	PKLog.LogMessage(PK_LOGLEVEL_INFO,_("ģ����: %15s, ��ʾ��: %14s, �Ƿ���ʾ:%s, ����ǰ�ȴ�: %d��"),
		m_szExeName, m_szDispName, m_bShowConsole ? "��" : "��", m_lSleepSeconds);
#else
	PKLog.LogMessage(PK_LOGLEVEL_INFO,_("Module name: %s, DisplayName: %s, IsShow:%s, sleep peroid before: %d sec"), 
		m_szExeName, m_szDispName, m_bShowConsole?"Yes":"No", m_lSleepSeconds);
#endif
}

// ȡ�������binĿ¼�����·��
int CPKProcess::prepare(ACE_Process_Options &options)
{
	int nRet = 0;
	string strProcessName = m_szExeAbsPath;
	//if(!IsWebServer())
	if (strlen(m_szStartCmd) <= 0) // û������StartCmd
	{
        //transform(strProcessName.begin(), strProcessName.end(), strProcessName.begin(),(int (*)(int))tolower);
		options.command_line(ACE_TEXT("%s %s"), strProcessName.c_str(), m_szArguments);
	}
	else // ������StartCmd
	{
		options.command_line(ACE_TEXT("%s"), m_szStartCmdAbsPath);
	}

    string strWorkDir = "";
    int nPos = strProcessName.find_last_of(ACE_DIRECTORY_SEPARATOR_STR);
    if(nPos >= 0)
        strWorkDir = strProcessName.substr(0, nPos);
    options.working_directory(strWorkDir.c_str());
	// ��Щ����������ù���·������pmemdb�������õĻ����Ҳ��������ļ����õ�../../logĿ¼�����޷�����

#ifdef _WIN32
	// int nRet = m_pipeOut.open(); Linux and Windows ACE_Pipe ��socketʵ�֣����뱣֤���н����ߣ����з����ߵ�ʱ����Щ�ܵ������Բ����ã������ò�Ҫʹ�������װ
	//if(nRet == -1)
	//{
	//	PKLog.LogMessage(PK_LOGLEVEL_ERROR, "Open OutPipe of %s failed", strProcessName.c_str());
	//}
	// nRet =  options.set_handles(ACE_STDIN ,m_pipeOut.write_handle(), m_pipeOut.write_handle());
	// nRet = options.set_handles(ACE_STDIN, ACE_INVALID_HANDLE, ACE_INVALID_HANDLE);//
	nRet = options.set_handles(NULL, NULL, NULL);//
#else
    nRet = options.set_handles(g_hOutput, g_hOutput, g_hOutput); // linux���ض����������
#endif
    options.working_directory(this->m_szWorkingDir); // set working dir
	//return nRet;
	return PK_SUCCESS;
}

// ����Solaris�³�ʱ��ȡ������������������������Լ��ṩһ��
// tvTimeWaitΪ����ʱ��
long CPKProcess::CV_ProcessMutex_Acquire(ACE_Process_Mutex *pMutex, ACE_Time_Value tvTimeout)
{
	long lRet = pMutex->tryacquire();
	if(lRet >= 0)
		return lRet;

	ACE_Time_Value tvCycle;
	tvCycle.msec(50);

	ACE_Time_Value tvTimeOutAbsTime = tvTimeout; // �������ʱ��
	while(true)
	{
		ACE_OS::sleep (tvCycle);
		lRet = pMutex->tryacquire();
		if(lRet >= 0)
			return lRet;

		ACE_Time_Value tvCurrent = ACE_OS::gettimeofday();
		if(tvCurrent > tvTimeOutAbsTime)
			return lRet;
	}

	return lRet;
}

long CPKProcess::StartProcess()
{
	m_bRestartEnable = m_bRestartEnableInit;

	// ������̲����ڣ��Ͳ���Ҫ�����ˣ�����Ҳ����Ҫ��ӡ��־
	//if(!IsWebServer())
	if (strlen(m_szStartCmd) <= 0)
	{
		if(!PKFileHelper::IsFileExist(this->m_szStartCmdAbsPath))
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "�����ļ�������,����:%s", this->m_szStartCmdAbsPath);
			UpdateStatusInPdb(false);
			return 0;
		}
	}

	//if(!IsWebServer()) // ����webserver
	if (strlen(m_szStartCmd) <= 0)
	{
		this->m_bRestartEnable = true; // ��������

		bool bAliveAlready = IsProcessAlive();
		if(bAliveAlready)
		{
			std::string strModuleName(m_szExeName);
            //transform(strModuleName.begin(), strModuleName.end(), strModuleName.begin(),(int (*)(int))tolower);

	#ifdef _WIN32
			m_pid = g_findKillProcess.GetProcessID(strModuleName.c_str(), TRUE);
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("Child process %s was started by other means![pid = %d]"), strModuleName.c_str(), m_pid);
	#else
			std::list<pid_t> listPid;
			FindPidByName(strModuleName.c_str(), listPid);
			//�������ǵĽ���ֻ��������Ψһ��һ�������ȡ��һ������
			if (!listPid.empty())
				m_pid = *listPid.begin();
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("Child process %s was started by other means![pid = %d]"), strModuleName.c_str(), m_pid);
	#endif
			UpdateStatusInPdb(true);
			return EC_ICV_PROCMGR_PROCESSSTARTEDALAREADY;
		}
	}

#if _WIN32
    if (m_bBatchFile || IsWebServer())
#else
    if (false)//m_bBatchFile || IsWebServer())
#endif
	{
		m_pid = StartBatchProcess();
		int nCheckCount = 3;
		while (nCheckCount-- > 0)
		{
			m_bAlive = IsProcessAlive();
			if (m_bAlive)
				break;
			ACE_OS::sleep(1); // ������������������̿���û�������ϼ�����
		}		
	}
	else
	{
		ACE_Process_Options options;
	#ifdef _WIN32
		options.startup_info()->dwFlags = STARTF_USESHOWWINDOW | CREATE_NEW_CONSOLE;
		options.startup_info()->wShowWindow =  m_bShowConsole? TRUE :FALSE;
	#else
		//options.handle_inheritance(0);
		options.handle_inheritance(1);
	#endif
		
		// �������̡���spawnʱ���ȵ���prepare,int CCVProcess::prepare(ACE_Process_Options &options) ������exe·������ǰ����·��
		m_pid = spawn(options);
		options.release_handles();
	}

	m_bAlive = IsProcessAlive();
	unsigned int nMilSeconds;
	unsigned int nSeconds = PKTimeHelper::GetHighResTime(&nMilSeconds);
	char szTimeStr[PK_DATESTRING_MAXLEN+ICV_SCANTIMESTRING_MAXLEN] = {'\0'};
	PKTimeHelper::HighResTime2String(szTimeStr, sizeof(szTimeStr), nSeconds, nMilSeconds);
	m_strLastRestartTime = szTimeStr;
	m_nRestartCount ++;

	if(m_pid == INVALID_PROCESS_ID)
	{
#ifdef _WIN32
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("��������ʧ��: %s!"), m_szExeName);
#else
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("Fail to start %s!"), m_szExeName);
#endif
		UpdateStatusInPdb(false);
		return EC_ICV_PROCMGR_FAILTOSTRTPROCESS;
	}
	else
	{
#ifdef _WIN32
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, _("�����Ѿ�����: %16s, %16s ...... [pid = %d]"), m_szDispName, m_szExeName, m_pid);
#else
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, _("process started: %s, %s ...... [pid = %d]"), m_szDispName, m_szExeName, m_pid);
#endif
		UpdateStatusInPdb(true);
		return PK_SUCCESS;
	}
}

long CPKProcess::StartBatchProcess()
{
#ifdef _WIN32
	int nCmdShow = SW_HIDE;
	if (m_bShowConsole)
		nCmdShow = SW_NORMAL;
	if (!PKFileHelper::IsFileExist(m_szStartCmdAbsPath))
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("����������ʧ��, �ļ�������: %s"), m_szStartCmdAbsPath);
		return -1;
	}

	HINSTANCE hInst = ShellExecute(NULL, "open", m_szStartCmdAbsPath, "", m_szWorkingDirAbsPath, nCmdShow); // ���һ����ʶ��������Ч��
	int nHInst = (int)hInst;
	if(nHInst > 32)
	{
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, _("�������̳ɹ�: %s, ��ǰĿ¼:%s"), m_szStartCmdAbsPath, m_szWorkingDirAbsPath);
		return (int)hInst;
	}
	else
	{
		long nError = GetLastError();
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "��������������ʧ��: '%s', ��ǰĿ¼:%s, ������: %d", m_szStartCmdAbsPath, m_szWorkingDirAbsPath, nError);
		return -1;
	}
#else
    return -1;
#endif
}

long CPKProcess::StopBatProcess(bool bDisplayLog)
{
#ifdef _WIN32
	// �����StopCmd�����StopCmd
	if(strlen(m_szStopCmdAbsPath) > 0)
	{
		if (!PKFileHelper::IsFileExist(m_szStopCmdAbsPath))
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("ֹͣ����ʧ��, �ű��ļ�������: %s"), m_szStopCmdAbsPath);
			return -1;
		}

		HINSTANCE hInst = ShellExecute(NULL, "open", m_szStopCmdAbsPath, "", m_szWorkingDirAbsPath, SW_HIDE); // ���һ����ʶ��������Ч��
		int nHInst = (int)hInst;
		if(nHInst > 32)
		{
			if (bDisplayLog)
				PKLog.LogMessage(PK_LOGLEVEL_NOTICE, _("�ɹ�ֹͣ����������: %s"), m_szStopCmdAbsPath);
			return (int)hInst;
		}
		long nError = GetLastError();
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "ֹͣ����������ʧ��: '%s', ����Ŀ¼:%s, ������: %d", m_szStopCmdAbsPath, m_szWorkingDirAbsPath, nError);
		return -1;
    }
	return 0;
#else
    return -1;
#endif
}
/************************************************************************/
/*   �ӽ����Ƿ񻹻���                                                                   */
/************************************************************************/
long CPKProcess::NotifyProcessToStop(bool bDisableRestart, bool bDisplayLog)
{
	if(bDisableRestart) // �Ƿ���Ҫ����Restart����
		this->m_bRestartEnable = false; // ��������

	// bat����Ҫ����ֹͣ��. �������IsProcessAlive֮ǰ
	//if(m_bBatchFile || IsWebServer())
	if (strlen(m_szStopCmd) > 0)
	{
		StopBatProcess(bDisplayLog);
		return 0;
	}

	// ����Ѿ�ֹͣ������Ҫ��ֹ֪ͨͣ
	bool bIsAlive = IsProcessAlive();
	if(!bIsAlive)
	{
		UpdateStatusInPdb(bIsAlive); // ����Ϊֹͣ״̬nengjiazai
		return PK_SUCCESS;
	}

	if(!m_pProcessQue)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("Fail to notify process %s to refresh because of empty queue"), m_szExeName);
		return -1;
	}
	
	char szNotifyCmd[PROCESSQUEUE_DEFAULT_RECORD_LENGHT] = {0};
	int nType = PROCESSMGR_CMDTYPE_STOP;
	memcpy(szNotifyCmd, &nType, sizeof(int));
	time_t tmNow;
	time(&tmNow);
	memcpy(szNotifyCmd + sizeof(int), &tmNow, sizeof(time_t));
	long lRet = m_pProcessQue->EnQueue(szNotifyCmd, sizeof(int) + sizeof(time_t));
	if(lRet == PK_SUCCESS)
	{
		UpdateStatusInPdb(false); // ����Ϊֹͣ״̬
		PKLog.LogMessage(PK_LOGLEVEL_DEBUG, _("Successful to send stop mesage to process %s ......"), m_szExeName);
	}
	else
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("Fail to send stop message to process %s, error code %d"), m_szExeName, lRet);

	return lRet;
}

// tvTimeout �Ǿ���ʱ��
long CPKProcess::WaitProcessToStop(ACE_Time_Value &tvTimeout, bool bForceStop)
{	
	// ����Ѿ��˳�
	if(!IsProcessAlive())
	{
#if defined (ACE_WIN32)
		// Free resources allocated in kernel.
		ACE_OS::close (this->process_info_.hThread);
		this->process_info_.hThread = NULL;
		ACE_OS::close (this->process_info_.hProcess);
		this->process_info_.hProcess = NULL;
#endif /* ACE_WIN32 */
		this->close_dup_handles();
        m_pipeOut.close();
		UpdateStatusInPdb(false);
		return 0;
	}

	// ���������ȷ��Ϲ���Ľ���,��δֹͣ
	if(PK_SUCCESS != CV_ProcessMutex_Acquire(m_pMutexProcessAlive, tvTimeout)) // �����δֹͣ
	{
		if (false == bForceStop)
		{
			UpdateStatusInPdb(true);
			return EC_ICV_PROCMGR_PROCESS_STILL_RUNNING;
		}
		// ��ȡ����֪ͨ�ź�����Ϊ��������ռ�ã�Ҳ��Ϊ��֪ͨ����
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("Fail to notify process %s to exit, will force to exit......"), m_szExeName);
	}

	// δ��ʱ�˳������߲����Ϲ�����pdb���Ľ���
	// ����ǿ����ֹ����
	m_pid = GetProcessId();
	if(m_pid > 0 )
	{
		long lRet = ACE::terminate_process(m_pid);//terminate();
		if(lRet == 0)
			PKLog.LogMessage(PK_LOGLEVEL_DEBUG, _("Successful to force to terminate process %s"), m_szExeName);
		else
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("Fail to force to terminate process %s, error code %d"), m_szExeName, lRet);
		this->wait();
	}

	m_pMutexProcessAlive->release();
	PKLog.LogMessage(PK_LOGLEVEL_DEBUG, _("m_pMutexProcessAlive->release()"));
	m_pMutexNotifyQuit->release();
	PKLog.LogMessage(PK_LOGLEVEL_DEBUG, _("m_pMutexNotifyQuit->release()"));

#if defined (ACE_WIN32)
	// Free resources allocated in kernel.
	ACE_OS::close (this->process_info_.hThread);
	this->process_info_.hThread = NULL;
	ACE_OS::close (this->process_info_.hProcess);
	this->process_info_.hProcess = NULL;
#endif /* ACE_WIN32 */
	this->close_dup_handles();

	UpdateStatusInPdb(false); // ����Ϊֹͣ״̬

	/*
	ACE_Time_Value now = ACE_OS::gettimeofday();
	if (tvTimeout > now)
	{
		ACE_Time_Value tvAbsWaitTime = tvTimeout - now;
		this->wait(tvAbsWaitTime);
	}
	else
	{
		ACE_Time_Value tvAbsWaitTime(1);
		this->wait(tvAbsWaitTime);
	}
	PKLog.LogMessage(PK_LOGLEVEL_DEBUG, _("WaitProcessToStop(%s) finish"), m_szModuleName);
	*/

 	return PK_SUCCESS;
}

pid_t CPKProcess::GetProcessId()
{
	string strModuleName = this->m_szExeName;
	pid_t tempPid = 0;
#ifdef _WIN32
	tempPid = g_findKillProcess.GetProcessID(strModuleName.c_str(), TRUE);
#else
	std::list<pid_t> listPid;
	FindPidByName(strModuleName.c_str(), listPid);
	//�������ǵĽ���ֻ��������Ψһ��һ�������ȡ��һ������
	if (!listPid.empty())
		tempPid = *listPid.begin();
#endif
	m_pid = tempPid;
	return tempPid; // ���̴���
}
/************************************************************************/
/*   �ӽ����Ƿ񻹻���                                                                   */
/************************************************************************/
bool CPKProcess::IsProcessAlive()
{
	if(m_pMutexProcessAlive->tryacquire() != PK_SUCCESS) // if another instance is running
	{
		if (m_pid == INVALID_PROCESS_ID)
			m_pid = GetProcessId();

		if(!m_bAlive)
			PKLog.LogMessage(PK_LOGLEVEL_DEBUG, _("process [%s] started."), GetModuleName());
		
		m_bAlive = true;
		m_bExeExist = true;
		return true; // ��ȡ����
	}
	
	// �Ǳ�׼���̣���ô�鿴�Ƿ���ϵͳ��ȷʵ�����˸ý���
	m_pMutexProcessAlive->release();
	m_pid = GetProcessId();
	if(m_pid > 0)
	{
		if(!m_bAlive)
			PKLog.LogMessage(PK_LOGLEVEL_DEBUG, _("process [%s] started."), GetModuleName());

		m_bAlive = true;
		m_bExeExist = true;
		return true;
	}
	
	if(m_bAlive) // ֮ǰ����
		PKLog.LogMessage(PK_LOGLEVEL_DEBUG, _("process [%s] stopped."), GetModuleName());
	m_bAlive = false;
	m_bExeExist = PKFileHelper::IsFileExist(this->m_szExeAbsPath);
	return false;
}

/**
 *  ��������.
 *
 *
 *  @version     04/01/2009  chenzhiquan  Initial Version.
 */
void CPKProcess::RestartProcess()
{
	this->NotifyProcessToStop(false, true);
	// �������н��̵���ȴ�ʱ��
	int nSecTimeOut = 3;
	ACE_Time_Value tv = ACE_OS::gettimeofday();
	tv += ACE_Time_Value(nSecTimeOut);

	this->WaitProcessToStop(tv, true);
	this->StartProcess();
}

void CPKProcess::ToggleConsoleStatus()
{
	m_bShowConsole = !m_bShowConsole;
#ifdef _WIN32
	g_findKillProcess.ShowProcessWindow(m_pid, m_bShowConsole? SW_SHOWNORMAL : SW_HIDE);
#endif
}

bool CPKProcess::IsShowConsole()
{
	return m_bShowConsole;
}

// �ú���Ŀǰ��Զ�޷�������
void CPKProcess::ManualStopProcess(int nWaitSeconds)
{
	this->NotifyProcessToStop(true, true);// �ֹ�ֹͣ�˾Ͳ�����������
	
	// �������н��̵���ȴ�ʱ�� nWaitSeconds
	int nSecTimeOut = nWaitSeconds;
	ACE_Time_Value tv = ACE_OS::gettimeofday();
	tv += ACE_Time_Value(nSecTimeOut);

	this->WaitProcessToStop(tv, true);
}

long CPKProcess::NotifyProcessToRefresh()
{
	if(!m_pProcessQue)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("Fail to notify process %s to refresh, because of empty queue"), m_szExeName);
		return -1;
	}
	UpdateStatusInPdb(false); // ����Ϊֹͣ��״̬

	char szNotifyCmd[PROCESSQUEUE_DEFAULT_RECORD_LENGHT] = {0};
	int nType = PROCESSMGR_CMDTYPE_REFRESH;
	memcpy(szNotifyCmd, &nType, sizeof(int));
	time_t tmNow;
	time(&tmNow);
	memcpy(szNotifyCmd + sizeof(int), &tmNow, sizeof(time_t));
	long lRet = m_pProcessQue->EnQueue(szNotifyCmd, sizeof(int) + sizeof(time_t));
	if(lRet == PK_SUCCESS)
		PKLog.LogMessage(PK_LOGLEVEL_WARN, _("Process manager sent refresh message to process %s  ......"), m_szExeName);
	else
		PKLog.LogMessage(PK_LOGLEVEL_WARN, _("Process manager failed to send refresh message to process %s, error code %d"), m_szExeName, lRet);

	UpdateStatusInPdb(true);
	return lRet;
} 

void CPKProcess::GetProcessState(string &strStatus)
{
	if(m_bAlive)
		strStatus = "running";
	else
	{
		if(PKFileHelper::IsFileExist(this->m_szExeAbsPath) == false)
			strStatus = "not existed";
		else
			strStatus = "stopped";
	}
}

int CPKProcess::DelManageTagInPdb()
{
#ifndef DSIABLE_PM_REMOTE_API
	char szTagName[PK_NAME_MAXLEN] = {0};
	char szTagValue[PK_TAGDATA_MAXLEN] = {0};
	sprintf(szTagName, "pm.%s",this->m_szExeName);
	int nRet = MAIN_TASK->m_memDBClientAsync.del(szTagName);
	MAIN_TASK->m_memDBClientAsync.commit();
#endif
	return nRet;
}

int CPKProcess::InitLogConfTagInPdb()
{
	return 0;
	char szTagName[PK_NAME_MAXLEN] = {0};
	sprintf(szTagName, "logconf:%s",this->m_szExeName);
	string strCurLogConfig;
	CMemDBClientAsync &redisRW = MAIN_TASK->m_memDBClientAsync;
	strCurLogConfig = redisRW.get(szTagName);
	if(strCurLogConfig.empty()) // ���û����level
	{
		char szTime[PK_HOSTNAMESTRING_MAXLEN] = {'\0'};
		PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));

		Json::Value jsonConfig;
		jsonConfig["level"] = "none";
		jsonConfig["time"] = szTime;
		jsonConfig["help"] = "none/debug/warn/error/notice/comm";

		MAIN_TASK->m_memDBClientAsync.set(szTagName, jsonConfig.toStyledString().c_str());
	}
	MAIN_TASK->m_memDBClientAsync.commit();
	return 0;
}

/************************************************************************/
/*  
����pdb�еĸý�����Ϣ

procmgr��Ҫ����
tag:manstate������״̬(running)�����һ�γ�������ʱ��(restarttime)����������(restartcount)

������Ҫ����
tag: starttime, procmgr����������Ҫ����
tag��devnum,�豸����(��������Ҫ,devnum��
tag: errinfo,(errno,errdesc),�������״̬

procmgr��鵽����δ��������Ҫ���������µ���������Ϊ��                                                                     
*/
/************************************************************************/
int CPKProcess::UpdateStatusInPdb(bool bAlive)
{
	if (m_pProcessMgr->IsGeneralPM())
		return 0;

	//if(m_bShouldUpdateStatusInPdb == false)
	if(strcmp(m_szExeName, MODULENAME_MEMDB) == 0) // ��pdb
	{
		//PKLog.LogMessage(PK_LOGLEVEL_INFO, "����:%s ����Ҫ����״̬���ڴ����ݿ�", this->m_szExeName);
		return 0;
	}

	time_t tmNow;
	time(&tmNow);
	//if(labs(tmNow - m_tmLastFailTime) > 0)
	//	return -1;

	int nRet = 0;
	char szTime[PK_HOSTNAMESTRING_MAXLEN] = {'\0'};
	PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));

	char szProcessInfo[PK_TAGDATA_MAXLEN] = {0};
	string strStatus;
	GetProcessState(strStatus);

	string strDispName = this->m_szDispName;
	string strExeName = this->m_szExeName;
	//strDispName = PKCodingConv::AnsiToUtf8(strDispName);
	//strExeName = PKCodingConv::AnsiToUtf8(strExeName);

	char szTagName[PK_NAME_MAXLEN] = {0};
	char szRestartCount[PK_TAGDATA_MAXLEN] = {0};
	sprintf(szTagName, "pm.%s",strExeName.c_str());
	sprintf(szRestartCount, "%d", this->m_nRestartCount);

	string strAlive = "0";
	if(bAlive)
		strAlive = "1";

	Json::Value jsonValue;
	jsonValue["name"] = strExeName.c_str();
	jsonValue["dispname"] = strDispName.c_str();
	jsonValue["status"] = strAlive.c_str();
	jsonValue["starttime"] = this->m_strLastRestartTime.c_str();
	jsonValue["startcount"] = szRestartCount;
	jsonValue["time"] = szTime;		 // ���һ�ε�ˢ��ʱ��
	jsonValue["type"] = m_bIsDriverExe ? "driver" : "service";
	string strValue = jsonValue.toStyledString();
#ifndef DSIABLE_PM_REMOTE_API
	MAIN_TASK->m_memDBClientAsync.set(szTagName, strValue.c_str());
	MAIN_TASK->m_memDBClientAsync.commit();
#endif
	// PKLog.LogMessage(PK_LOGLEVEL_INFO, "���½���(%s) ״̬: %s", szTagName, strValue.c_str());

	return nRet;
}

bool CPKProcess::IsMemDB()
{
	if(strcmp(m_szExeName, MODULENAME_MEMDB) == 0)
		return true;
	return false;
}

string CPKProcess::GetAbsFilePath(string strFileOrDirName, bool bIsDriver)
{
	if(strFileOrDirName.length() == 0) // Ϊ���򷵻ؿ�
		return "";

	if (strstr(strFileOrDirName.c_str(), ":") != NULL)//���·���д��У��ţ��϶���ȫ·������Ϊwindows��������������а�������
		return strFileOrDirName;

	string strBinPath = m_szExeDir;
	if (strBinPath.length() == 0)
		strBinPath = PKComm::GetBinPath();
	
	if(bIsDriver)
		strBinPath = strBinPath + ACE_DIRECTORY_SEPARATOR_STR + "drivers" + ACE_DIRECTORY_SEPARATOR_STR + m_szExeName + ACE_DIRECTORY_SEPARATOR_STR;
	strBinPath = strBinPath + ACE_DIRECTORY_SEPARATOR_STR + strFileOrDirName; // ����·��
	return strBinPath;
}

int CPKProcess::GenerateAbsPaths(bool bIsDriver)
{
	// WebServer�Ƚϸ��ӣ������滻���ڶ�ȡ����ʱ��������Ѿ�����
	if(IsWebServer())
		return 0;

	string strExeAbsPath = GetAbsFilePath(m_szExeName, bIsDriver);
	string strStartCmdAbsPath = GetAbsFilePath(m_szStartCmd, bIsDriver);
	string strStopCmdAbsPath = GetAbsFilePath(m_szStopCmd, bIsDriver);
	if(strStartCmdAbsPath.length() == 0)
		strStartCmdAbsPath = strExeAbsPath;
	// ���̵ĵ�ǰ·�������û����Ϊpkservermgr.exe��·������exe��·��


	//transform(strExeAbsPath.begin(), strExeAbsPath.end(), strExeAbsPath.begin(),(int (*)(int))tolower);
	//transform(strStartCmdAbsPath.begin(), strStartCmdAbsPath.end(), strStartCmdAbsPath.begin(),(int (*)(int))tolower);
	//transform(strStopCmdAbsPath.begin(), strStopCmdAbsPath.end(), strStopCmdAbsPath.begin(),(int (*)(int))tolower);
	m_bBatchFile = false;
#ifdef _WIN32
	if(strExeAbsPath.length() >= 4)
	{
		string strExt= strExeAbsPath.substr(strExeAbsPath.length()-4,4) ;
		if(strExt.compare(".bat") != 0)
		{
			if (strExt.compare(".exe") != 0)
				strExeAbsPath += ".exe";
		}
	}

	if(strStartCmdAbsPath.length() >= 4)
	{
		string strExt= strStartCmdAbsPath.substr(strStartCmdAbsPath.length()-4,4) ;
		if(strExt.compare(".bat") != 0)
		{
			if (strExt.compare(".exe") != 0)
				strStartCmdAbsPath += ".exe";
		}
		else
			m_bBatchFile = true;
	}
	
	if(strStopCmdAbsPath.length() >= 4)
	{
		string strExt= strStopCmdAbsPath.substr(strStopCmdAbsPath.length()-4,4) ;
		if(strExt.compare(".bat") != 0)
		{
			strStopCmdAbsPath += ".exe";
		}
	}
#else // LINUX
	if (strStartCmdAbsPath.length() >= 3) // start_pkmemdb.sh�������ű����ж�
	{
		string strExt = strStartCmdAbsPath.substr(strStartCmdAbsPath.length() - 3, 3);
		if (strExt.compare(".sh") == 0)
			m_bBatchFile = true;
	}
#endif

	
	//transform(strWorkdingDir.begin(), strWorkdingDir.end(), strWorkdingDir.begin(), (int(*)(int))tolower);

	string strExeOrBatchFilePath = m_bBatchFile ? strStartCmdAbsPath : strExeAbsPath;
	if (strlen(m_szWorkingDir) <= 0) // δ���õ�ǰ·��
	{
		char szWorkingDir[PK_LONGFILENAME_MAXLEN] = { 0 };
		char szFileName[PK_LONGFILENAME_MAXLEN] = { 0 };
		PKFileHelper::SeparatePathName(strExeOrBatchFilePath.c_str(), szWorkingDir, sizeof(szWorkingDir), szFileName, sizeof(szFileName));
		strncpy(m_szWorkingDir, szWorkingDir, sizeof(m_szWorkingDir) - 1);
		strncpy(m_szWorkingDirAbsPath, szWorkingDir, sizeof(m_szWorkingDirAbsPath) - 1);
	}
	else // �����˹���·����
	{
		string strWorkdingDir = GetAbsFilePath(m_szWorkingDir, bIsDriver);
		strncpy(m_szWorkingDirAbsPath, strWorkdingDir.c_str(), sizeof(m_szWorkingDirAbsPath) - 1);
	}

	strncpy(m_szExeAbsPath, strExeAbsPath.c_str(), sizeof(m_szExeAbsPath) - 1);
	strncpy(m_szStartCmdAbsPath, strStartCmdAbsPath.c_str(), sizeof(m_szStartCmdAbsPath) - 1);
	strncpy(m_szStopCmdAbsPath, strStopCmdAbsPath.c_str(), sizeof(m_szStopCmdAbsPath) - 1);
	return 0;
}

bool CPKProcess::IsWebServer()
{
	if(PKStringHelper::StriCmp(m_szExeName, "java") != 0)
		return false;
	return false; // ����true��չ����bat�������ִ��shell������
}
