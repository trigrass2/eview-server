// CVProcess.cpp: implementation of the CCVProcess class.
//
//////////////////////////////////////////////////////////////////////
#include "ace/Time_Value.h"
#include "ProcessMngr.h"
#include "pklog/pklog.h"
#include "pkcomm/pkcomm.h"
#include <ace/OS_NS_stdio.h>
#include "errcode/error_code.h"
#include <algorithm>
#include "common/RMAPI.h"
#include "tinyxml/tinyxml.h"
#include "pkeviewdbapi/pkeviewdbapi.h"
extern CPKLog PKLog;

#define NULLASSTRING(x) x==NULL?"":x
#define _(STRING) STRING
#define INVALID_PROCESS_ID							-1

#define WAIT_AFTER_PROCESS_EXIT			3
#define CHECK_PERIOD_DEFAULT			30

#ifndef _WIN32
ACE_HANDLE g_hOutput = ACE_OS::open("/dev/null", 0);
#else
ACE_HANDLE g_hOutput = ACE_OS::open("/dev/null", 0);
#endif

CPKProcessMgr::CPKProcessMgr()
{
	m_processList.clear();
	m_usPort = 0;
	m_pfnLogCallBack = NULL;
	m_nCurrentIONum = 0;
	m_bLicExpired = false;
	m_pPdbProcess = NULL;
	time(&m_tmLastMaintance);
	time(&m_tmExpire);
	m_strApplyFor = "";
	m_nCheckPeriod = CHECK_PERIOD_DEFAULT; // ȱʡ�������Ϊ60��
}

CPKProcessMgr::~CPKProcessMgr()
{
	m_objProcPlugin.UnLoad();
}

int CPKProcessMgr::ReadAllServicesConfig()
{
	// sqlite�������ļ�·��
	char szCfgPathName[PK_SHORTFILENAME_MAXLEN] = {0};
	char *szCVProjCfgPath = (char *)PKComm::GetConfigPath(); // D:\Work\Gateway\bin\drivers\modbus\..\..\\config
	sprintf(szCfgPathName, "%s%c%s",szCVProjCfgPath, ACE_DIRECTORY_SEPARATOR_CHAR,"pkservermgr.xml"); // config/gateway.db

	m_processList.empty();
	TiXmlDocument* pDoc = new TiXmlDocument(szCfgPathName);
	if(!pDoc->LoadFile() )
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "load services config file:%s failed", szCfgPathName);
		return -1;
	}

	TiXmlElement* pElemRoot = pDoc->RootElement(); // Servers�ڵ�
	if(pElemRoot == NULL)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "services config file:%s has no root node", szCfgPathName);
		delete pDoc;
		return -2;
	}

	// �Ƿ���������б���
	const char *szApplyFor = pElemRoot->Attribute("app");
	if (szApplyFor && strlen(szApplyFor) > 0)
		m_strApplyFor = szApplyFor;

	const char *szCheckPeriod = pElemRoot->Attribute("CheckPeriod");
	if(szCheckPeriod && strlen(szCheckPeriod) > 0)
	{
		m_nCheckPeriod = ::atoi(szCheckPeriod) != 0;
		if(m_nCheckPeriod <= 0 || m_nCheckPeriod > 60*60*24) // 1��
			m_nCheckPeriod = CHECK_PERIOD_DEFAULT;
	}

	TiXmlElement* pElemExclude = pElemRoot->FirstChildElement("Exclude");
	for(; pElemExclude != NULL; pElemExclude = pElemExclude->NextSiblingElement())
	{
		// ģ�����ƣ��������е�exe
		char *szExeName = (char *)pElemExclude->Attribute("ExeName");
		if(!szExeName)
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "exclude config file:%s, service node has no ExeName attribute!", szCfgPathName);
			continue;
		}

		m_mapExcludeSrvDrv[szExeName] = 1;
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "exclude config file:%s, will not be started!", szExeName);
	}

	string strBinPath = PKComm::GetBinPath();

	TiXmlElement* pElemService = pElemRoot->FirstChildElement("Server");
	for(; pElemService != NULL; pElemService = pElemService->NextSiblingElement())
	{
		// ģ�����ƣ��������е�exe
		char *szExeName = (char *)pElemService->Attribute("ExeName");
		if(!szExeName)
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "services config file:%s, service node has no ExeName attribute!", szCfgPathName);
			continue;
		}
		if(m_mapExcludeSrvDrv.find(szExeName) != m_mapExcludeSrvDrv.end())
		{
			PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "server %s, is excluded, will not be started!", szExeName);
			continue;
		}

		// ��ʾ����
		string strDisplayName;
		char *szDisplayName = (char *)pElemService->Attribute("Name");
		if (!szDisplayName)
			strDisplayName = szExeName;
		else
			strDisplayName = szDisplayName;

		char *szStartCmd = (char *)pElemService->Attribute("StartCmd");
		char *szStopCmd = (char *)pElemService->Attribute("StopCmd");

		// ��ǰexe��Ŀ¼
		string strExeDir;
		char *szExeDir = (char *)pElemService->Attribute("ExeDir");
		if (szExeDir)
			strExeDir = szExeDir;

		// new �߳�
		CPKProcess *pCVProcess = new CPKProcess((char *)szExeName, (char *)strExeDir.c_str(), (char *)strDisplayName.c_str(), szStartCmd, szStopCmd, "", 0, this);
		pCVProcess->m_bIsDriverExe = false;
		// �������
		char *szParameter = (char *)pElemService->Attribute("Parameter");
		if(szParameter)
			sprintf(pCVProcess->m_szArguments, " %s", szParameter); // �����������ļ�

		// �Ƿ������Զ�����
		char *szAutoRestart = (char *)pElemService->Attribute("AutoRestart");
		if(szAutoRestart)
			pCVProcess->m_bRestartEnable = ::atoi(szAutoRestart) == 0 ?false:true;
		else
			pCVProcess->m_bRestartEnable = true;
		pCVProcess->m_bRestartEnableInit = pCVProcess->m_bRestartEnable;

		// �Ƿ���ʾ����̨����
		char *szShowConsole = (char *)pElemService->Attribute("ShowConsole");
		if(szShowConsole)
			pCVProcess->m_bShowConsole = ::atoi(szShowConsole) == 0 ?false:true;
		else
			pCVProcess->m_bShowConsole = false;

		// �������ӳ�ʱ��
		char *szDelaySec = (char *)pElemService->Attribute("DelaySec");
		if(szDelaySec)
			pCVProcess->m_lSleepSeconds = ::atoi(szDelaySec);
		if(pCVProcess->m_lSleepSeconds < 0)
			pCVProcess->m_lSleepSeconds = 0;

		if(strcmp(szExeName, MODULENAME_MEMDB) == 0) // �ǲ���pdb
		{
			pCVProcess->m_bShouldUpdateStatusInPdb = false; // pdb�Լ���Ȼ����Ҫ�����Լ�
			//PKLog.LogMessage(PK_LOGLEVEL_INFO, "����:%s ����Ϊ������״̬���ڴ����ݿ�!!", szExeName);
			m_pPdbProcess = pCVProcess;
			string strPdbConfigFile = PKComm::GetConfigPath();
			strPdbConfigFile = strPdbConfigFile + ACE_DIRECTORY_SEPARATOR_STR + "pkmemdb.conf";
			sprintf(pCVProcess->m_szArguments, " %s", strPdbConfigFile.c_str()); // �����������ļ�
		}
		else
			pCVProcess->m_bShouldUpdateStatusInPdb = true;

		if (strcmp(szExeName, MODULENAME_HISDB) == 0) // �ǲ���hisdb
		{
			pCVProcess->m_bShouldUpdateStatusInPdb = true; // pdb�Լ���Ȼ����Ҫ�����Լ�
			string strConfigFile = PKComm::GetConfigPath();
			strConfigFile = strConfigFile + ACE_DIRECTORY_SEPARATOR_STR + "pkhisdb.conf";
			sprintf(pCVProcess->m_szArguments, " -f %s", strConfigFile.c_str()); // �����������ļ�
		}

		pCVProcess->GenerateAbsPaths(false);
		pCVProcess->DumpProcessInfo();
		m_processList.push_back(pCVProcess);
	}

	return 0;
}

int CPKProcessMgr::ReadDriverAndDevicesFromDB(vector<CDriverInfo> &vecDriver)
{
	CPKEviewDbApi PKEviewDbApi;
	int nRet = 0;
	char szSql[2048] = { 0 };
	// ��ȡ���е��������б�
	vector<vector<string> > vecDevice;
	nRet = PKEviewDbApi.Init();
	if (nRet != 0)
	{
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "��ʼ�����ݿ�ʧ��!");
		PKEviewDbApi.Exit();
		return -1;
	}

	PKStringHelper::Snprintf(szSql, sizeof(szSql), "SELECT B.id as drv_id,B.name as drv_name,B.modulename as drv_modulename,B.description as drv_description,B.platform as drv_platform, B.enable as drv_enable, A.id as dev_id, A.name as dev_name, A.description as dev_description,A.simulate as dev_simulate , A.enable as dev_enable FROM t_device_list A, t_device_driver B where A.driver_id = B.id and (B.enable is null or B.enable<>0) and (A.enable is null or A.enable<>0)");
	string strError;
	nRet = PKEviewDbApi.SQLExecute(szSql, vecDevice, &strError);
	if (nRet != 0)
	{
		PKEviewDbApi.Exit();
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "��ѯ���ݿ�ʧ��,������:%d,SQL:%s, error:%s", nRet,szSql, strError.c_str());
		return -2;
	}

	if (vecDevice.size() <= 0)
	{
		PKEviewDbApi.Exit();
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "δ�ҵ��κο��õ�����, SQL:%s!", szSql);
		return -1;
	}

	PKEviewDbApi.Exit();

	for (int iDev = 0; iDev < vecDevice.size(); iDev++)
	{
		vector<string> &row = vecDevice[iDev];
		int iCol = 0;
		// ��������Ϣ
		string strDriverId = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDriverName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDriverModuleName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDriverDesc = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDriverSupportPlatform = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDriverEnable = NULLASSTRING(row[iCol].c_str());
		iCol++;

		// �豸����Ϣ
		string strDeviceId = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDeviceName = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDeviceDesc = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDeviceSimulate = NULLASSTRING(row[iCol].c_str());
		iCol++;
		string strDeviceEnable = NULLASSTRING(row[iCol].c_str());
		iCol++;

		if (m_mapExcludeSrvDrv.find(strDriverName) != m_mapExcludeSrvDrv.end())
		{
			PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "driver %s, is excluded, will not be started!", strDriverName.c_str());
			continue;
		}

		if (strDriverName.length() <= 0 || strDriverModuleName.length() <= 0){
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "driver: %s, modulename:%s, cannot empty, skip...", strDriverName.c_str(), strDriverModuleName.c_str());
			continue;
		}
		CDriverInfo drvInfo;
		drvInfo.m_strDriverName = strDriverName;
		drvInfo.m_strModuleName = strDriverModuleName;
		drvInfo.m_strPlatform = strDriverSupportPlatform;

		int bDeviceSimulate = ::atoi(strDeviceSimulate.c_str());
		int bDriverEnable = ::atoi(strDriverEnable.c_str());
		int bDeviceEnable = ::atoi(strDeviceEnable.c_str());

		// ���õ����������豸����Ҫ����
		if (strDeviceEnable.compare("0") == 0)
			continue;
		if (strDeviceEnable.compare("0") == 0)
			continue;

		// ģ���豸�Ļ���ֱ�Ӽ���ģ������
		if (bDeviceSimulate)
		{
			drvInfo.m_strModuleName = "simdriver";
			drvInfo.m_strDriverName = "simdriver";
		}

		// ���û���ظ����������룬����������
		bool bFound = false;
		for (vector<CDriverInfo>::iterator itDrv = vecDriver.begin(); itDrv != vecDriver.end(); itDrv++)
		{
			if (itDrv->m_strDriverName.compare(drvInfo.m_strDriverName) == 0) // �����Ѿ�����������Ҫ����
			{
				bFound = true;
				continue;
			}
		}
		if (!bFound)
			vecDriver.push_back(drvInfo);
	}

	return 0;
}

int CPKProcessMgr::ReadAllDriversConfig()
{
	// �����ͨ���ػ�������ô�ǲ��ö�ȡeview�������б��
	if (IsGeneralPM())
		return 0;

	vector<CDriverInfo> vecDriver;
	ReadDriverAndDevicesFromDB(vecDriver);

	int nRet = 0;
	int nValidDriverNum = 0;

	// pkdriver.exe��·��
	string strTemplDriverPath = PKComm::GetBinPath();
	strTemplDriverPath = strTemplDriverPath + ACE_DIRECTORY_SEPARATOR_STR + "pkdriver.exe";
	printf("+============================================================+\n");
	// ֻ���������豸����������Ҫ����������
	for (int iDrv = 0; iDrv < vecDriver.size(); iDrv++)
	{
		CDriverInfo &drv = vecDriver[iDrv];
		CPKProcess *pCVProcess = new CPKProcess((char *)drv.m_strModuleName.c_str(), "", (char *)drv.m_strDriverName.c_str(), NULL, NULL, (char *)drv.m_strPlatform.c_str(), 0, this);
		pCVProcess->m_bRestartEnable = true;
		pCVProcess->m_bShowConsole = false;
		pCVProcess->m_bIsDriverExe = true; // ��������������ʶ
		PKStringHelper::Safe_StrNCpy(pCVProcess->m_szArguments, "--silent", sizeof(pCVProcess->m_szArguments));
		pCVProcess->GenerateAbsPaths(true);

		string strDriverDir = PKComm::GetBinPath(); // ����Ŀ¼
		strDriverDir = strDriverDir + ACE_DIRECTORY_SEPARATOR_STR + "drivers" + ACE_DIRECTORY_SEPARATOR_STR + pCVProcess->m_szExeName + ACE_DIRECTORY_SEPARATOR_STR;
		// �������ģ��exe�����ڣ���Ŀ¼���ڣ���ģ������pkdriver���ڣ���ô����pkdriver.exe������Ŀ¼
		if (PKFileHelper::IsFileExist(strTemplDriverPath.c_str()) && PKFileHelper::IsDirectoryExist(strDriverDir.c_str()))
		{
#ifdef _WIN32
            CopyFile(strTemplDriverPath.c_str(), pCVProcess->m_szExeAbsPath, false);
#endif
		}

		if (!PKFileHelper::IsFileExist(pCVProcess->m_szExeAbsPath))
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "����ģ�鲻����: %s, ·��:%s ", drv.m_strDriverName.c_str(), pCVProcess->m_szExeAbsPath);
		}
		else
		{
			PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "�Ѷ�ȡ��������: %s", drv.m_strDriverName.c_str());

			m_processList.push_back(pCVProcess);
			// pCVProcess->DumpProcessInfo();
			nValidDriverNum++;
		}
	}

	if (vecDriver.size() != nValidDriverNum)
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("�����б��� %d ������, ������Ч��������: %d!"), nValidDriverNum, vecDriver.size() - nValidDriverNum);
	else
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, _("�����б��� %d ������!"), nValidDriverNum);
	printf("+============================================================+\n\n");
	return nRet;
}


long CPKProcessMgr::ReadAllProcessConfig()
{
	// ���ط������
	ReadAllServicesConfig();

	ProcessWebServerConfig(); // WebServer �����������

	// ��ȡ����������
	ReadAllDriversConfig();

	return PK_SUCCESS;
}

long CPKProcessMgr::StartAllProcess( bool bIncludePdb)
{
	printf("+============================================================+\n");
	// ����������pdb����������SetStatus�ᱨredis����pdb���ǵ�һ�����̣���
	CVPROCESSINFOLIST::iterator itProc = m_processList.begin();
	if(bIncludePdb)
	{
		for(; itProc != m_processList.end(); itProc ++)
		{
			CPKProcess * pCVProc = *itProc;
			if(pCVProc->IsMemDB())
			{
				pCVProc->StartProcess();
				ACE_OS::sleep(1); // ����pdb����ͣ1�룬�Ա�pdb������ȫ��������
			}
		}
	}

	printf("+============================================================+\n");
	// ���������������̣��������
	itProc = m_processList.begin();
	for(; itProc != m_processList.end(); itProc ++)
	{
		CPKProcess * pCVProc = *itProc;
		pCVProc->m_bRestartEnable = pCVProc->m_bRestartEnableInit;

		if(pCVProc->IsMemDB() || pCVProc->m_bIsDriverExe)
			continue;

		// �����Ҫ�ȴ�һ��ʱ����������һ��������ô�ȴ�
		if(pCVProc->m_lSleepSeconds > 0)
			ACE_OS::sleep(pCVProc->m_lSleepSeconds);

		pCVProc->StartProcess();
	}

	printf("+============================================================+\n");
	// ���������������̣���������
	itProc = m_processList.begin();
	for (; itProc != m_processList.end(); itProc++)
	{
		CPKProcess * pCVProc = *itProc;
		pCVProc->m_bRestartEnable = pCVProc->m_bRestartEnableInit;

		if (pCVProc->IsMemDB() || !pCVProc->m_bIsDriverExe)
			continue;

		// �����Ҫ�ȴ�һ��ʱ����������һ��������ô�ȴ�
		if (pCVProc->m_lSleepSeconds > 0)
			ACE_OS::sleep(pCVProc->m_lSleepSeconds);

		pCVProc->StartProcess();
	}
	return PK_SUCCESS;
}

void CPKProcessMgr::StopAllProcess(bool bRestart, bool bIncludePdb, bool bDisableRestart, bool bDisplayLog)
{
	if(bRestart) // ����������������
		bDisableRestart = false;

	NotifyAllProcessStop(bRestart,bIncludePdb, bDisableRestart, bDisplayLog);

	// �������н��̵���ȴ�ʱ��
	int nSecTimeOut = 10;
	ACE_Time_Value tv = ACE_OS::gettimeofday();
	tv += ACE_Time_Value(nSecTimeOut);

	// �������֪ͨ�����ܵ��½����windows ����֪ͨ��ǰ�������������̾�ǿ���˳��ˣ�����ʱ��10�룩�������������̶�û�йر�
	CVPROCESSINFOLIST::reverse_iterator itProc = m_processList.rbegin();
	for(; itProc != m_processList.rend(); itProc ++)
	{
		CPKProcess * pCVProc = *itProc;

		if(!bIncludePdb && m_pPdbProcess == pCVProc)
			continue;

		pCVProc->NotifyProcessToStop(bDisableRestart, bDisplayLog);
	}

	if (bDisplayLog)
	{
#ifdef _WIN32
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, _("���н���(����:%d) �ѱ�ֹ֪ͨͣ, ���ȴ�%d��..."), m_processList.size(), nSecTimeOut);
#else
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, _("all processes (count:%d) have been notifed to stop, now wait to stop (at most %d seconds)..."), m_processList.size(), nSecTimeOut);
#endif
	}

	itProc = m_processList.rbegin();
	for(; itProc != m_processList.rend(); itProc ++)
	{
		CPKProcess * pCVProc = *itProc;
		if(!bIncludePdb && m_pPdbProcess == pCVProc)
			continue;
		if(!pCVProc->m_bExeExist)
			continue;
		if(!pCVProc->m_bAlive)
			continue;
		//if (!bRestart || pCVProc->m_bRestartEnable)
		{
#ifdef _WIN32
			PKLog.LogMessage(PK_LOGLEVEL_DEBUG, _("�ȴ����� %s ֹͣ..."), pCVProc->GetModuleName());
			pCVProc->WaitProcessToStop(tv, true);
			PKLog.LogMessage(PK_LOGLEVEL_DEBUG, _("���� %s ��ֹͣ."), pCVProc->GetModuleName());
#else
			PKLog.LogMessage(PK_LOGLEVEL_DEBUG, _("wait for process %s to exit."), pCVProc->GetModuleName());
			pCVProc->WaitProcessToStop(tv, true);
			PKLog.LogMessage(PK_LOGLEVEL_DEBUG, _("process %s exit."), pCVProc->GetModuleName());
#endif
		}
	}

	if (bDisplayLog)
	{
#ifdef _WIN32
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, _("���н���(����:%d)�Ѿ�ֹͣ!"), m_processList.size());
#else
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, _("all processes (count:%d) have been stopped now"), m_processList.size());
#endif
	}

	if(bRestart)
	{
		StartAllProcess(bIncludePdb);
	}
}


//long CCVProcessMgr::GetRedundantStatus(long& nRMStatus)
//{
//	nRMStatus = RM_STATUS_UNAVALIBLE;
//	long lRet = GetRMStatus(&nRMStatus);
//	return lRet;
//}

void CPKProcessMgr::PrintAllProcessState()
{
	/*
	printf(_("--------------------------systeminfo----------------------- \n"));
	int nRMStatus = RM_STATUS_UNAVALIBLE;
	int nRet = GetRMStatus(&nRMStatus);
	char szRMStatus[ICV_LONGFILENAME_MAXLEN];
	if(nRet != PK_SUCCESS)
		sprintf(szRMStatus, _("fail to get node status"));
	else if(nRMStatus == RM_STATUS_INACTIVE)
		sprintf(szRMStatus, _("inactive node"));
	else if(nRMStatus == RM_STATUS_ACTIVE)
		sprintf(szRMStatus, _("active node"));
	else if(nRMStatus == RM_STATUS_UNAVALIBLE)
		sprintf(szRMStatus, _("RMService not ready"));
	else
		sprintf(szRMStatus, _("Unknown status(%d)"), nRMStatus);

	printf("%-30s -- %s\n", _("RM status"), szRMStatus);
	*/

	int iProcess = 0;
	printf("+=============================���̹���=====================================+\n");
	printf("%-6.6s %-24.24s %-15.15s %-25.25s %-6.6s %-12.12s\n", "No.","process","status","start time","count","pid");
	for(CVPROCESSINFOLIST::iterator itProc = m_processList.begin(); itProc != m_processList.end(); itProc ++)
	{
		CPKProcess * pCVProc = *itProc;
		string strStatus;
		iProcess ++;
		pCVProc->GetProcessState(strStatus);
		printf("No:%-3x %-24.24s %-15.15s %-25.25s %-6d %-12d\n", iProcess, pCVProc->m_szDispName, strStatus.c_str(), pCVProc->m_strLastRestartTime.c_str(), pCVProc->m_nRestartCount, pCVProc->getpid());
	}
	printf("+=====================�������ֱ�š�16���ơ�������ʾ/������Ӧ���̴���==================+\n\n");

}

long CPKProcessMgr::InitProcessMgr()
{
	// ��ȡ��������
	char *szBindir = (char *)PKComm::GetBinPath();
	PKLog.LogMessage(PK_LOGLEVEL_DEBUG, _("��ǰbinĿ¼: %s"), szBindir);

	//ִ�в��
	m_objProcPlugin.Load(szBindir);
	this->ReadAllProcessConfig();

	// ������һ��pdb����
	//CCVProcess *pProcess = *m_processList.begin();
	//pProcess->RestartProcess();

	return PK_SUCCESS;
}

long CPKProcessMgr::FiniProcessMgr()
{
	for(CVPROCESSINFOLIST::iterator itProc = m_processList.begin(); itProc != m_processList.end(); itProc ++)
	{
		CPKProcess * pCVProc = *itProc;
		delete pCVProc;
		pCVProc = NULL;
	}

	m_processList.clear();
	return PK_SUCCESS;
}

void CPKProcessMgr::MaintanceProcess()
{
	// ���С��10���������
	time_t tmNow;
	time(&tmNow);
	if(labs(tmNow - m_tmLastMaintance) < m_nCheckPeriod)
		return;

	for(CVPROCESSINFOLIST::iterator itProc = m_processList.begin(); itProc != m_processList.end(); itProc ++)
	{
		CPKProcess * pCVProc = *itProc;
		bool bAlive = pCVProc->IsProcessAlive();
		pCVProc->UpdateStatusInPdb(bAlive);
		
		// �����Ѿ�ֹͣ������Ƿ���Ҫ����
		if (!bAlive)
		{
			// ������̲����ڣ��Ͳ���Ҫ�����ˣ�����Ҳ����Ҫ��ӡ��־
			if(!PKFileHelper::IsFileExist(pCVProc->m_szExeAbsPath))
				continue;

			// �ֹ�ֹͣ����������
			if (!pCVProc->m_bRestartEnable)
				continue;

			// ���֤����
			if (m_bLicExpired)
				continue;

			//�״μ�⵽�����˳�
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("Detected process '%s',displayName:%s not running, starting it again..."), 
				pCVProc->GetModuleName(), pCVProc->m_szDispName);			
			pCVProc->RestartProcess();
		}
	}		

	time(&m_tmLastMaintance);
}

bool CPKProcessMgr::IsCurTimeExpired()
{
	time_t tmNow;
	time(&tmNow);
	if(m_tmExpire - tmNow >= 0) //����Ч�ڷ�Χ��
		return false;
	else
		return true;
}

void CPKProcessMgr::PrintLicenseState()
{
	if (IsGeneralPM())
		return;

	//��ȡ�ļ�·��
	const char * szCVCfgPath = PKComm::GetConfigPath();
	if(!szCVCfgPath)
	{
		printf("pkOpenLic, ��ȡ�����ļ�Ŀ¼ʧ��,����Ŀ¼\n");
		return;
	}

	char szLicFilePath[1024] = {0};
	//·��+�ָ���+�ļ���
	sprintf(szLicFilePath, "%s" ACE_DIRECTORY_SEPARATOR_STR "license.pkl", szCVCfgPath);

	#define CV_LICENSE_TEMPORARY_VALIDATE (60*60*2) 
	char szExpireTime[32] = {0};
	char szErrTip[256] = {0};
	long lRet = pkOpenLic(szLicFilePath, &m_hLicense, &m_nLicStatus, szErrTip, sizeof(szErrTip));
	if(m_nLicStatus == PK_LIC_STATUS_EXIST_VALID) // ��Ч���֤���ɵõ�����
	{
		pkGetLicValue(m_hLicense, PK_LIC_KEY_EXPIRE_DATE, szExpireTime, sizeof(szExpireTime));
		strcat(szExpireTime, " 23:59:59"); //yyyy-mm-dd hh:mm:ss
		m_tmExpire = PKTimeHelper::String2Time(szExpireTime);
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "�������֤�ɹ�, ��Ч���֤������ʱ�䣺%s", szExpireTime);
	}
	else if(m_nLicStatus == PK_LIC_STATUS_NOEXIST) // ��Ч���֤���ɵõ�����
	{
		time(&m_tmExpire);
		m_tmExpire += CV_LICENSE_TEMPORARY_VALIDATE;
		PKTimeHelper::Time2String(szExpireTime, sizeof(szExpireTime),m_tmExpire);
	}

	bool bExpired = IsCurTimeExpired();

	char szLicStatus[32] = {0};
	if(m_nLicStatus == PK_LIC_STATUS_NOEXIST)
	{
		strcpy(szLicStatus, "���֤������,��Сʱ����ģʽ");
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, szLicStatus);
	}
	else if(m_nLicStatus == PK_LIC_STATUS_EXIST_FILE_OPENFAILED)
	{
		strcpy(szLicStatus, "���֤�ļ���ʽ�Ƿ�");
	}
	else if(m_nLicStatus == PK_LIC_STATUS_EXIST_EXPIRED)
	{
		strcpy(szLicStatus, "���֤�ѹ���");
	}
	else if(m_nLicStatus == PK_LIC_STATUS_EXIST_UNDUE)
	{
		strcpy(szLicStatus, "���֤��δ����");
	}
	else if(m_nLicStatus == PK_LIC_STATUS_EXIST_HOSTID_NOT_MATCH)
	{
		strcpy(szLicStatus, "���֤������ƥ��");
	}
	else if(m_nLicStatus == PK_LIC_STATUS_EXIST_SIGNATURE_NOTMATCH)
	{
		strcpy(szLicStatus, "���֤ǩ����ƥ��");
	}
	else // if(m_nLicStatus == PK_LIC_STATUS_EXIST_VALID)
	{
		strcpy(szLicStatus, "��Ч");
	}

	PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "���֤״̬: %s, ��Ч��: %s", szLicStatus,szExpireTime);

	printf("--------------------�ڵ����֤��Ϣ------------------ \n");
	printf("\t���֤���ͣ�\t%s\n", szLicStatus);
	printf("\t�����Ч�ڣ�\t%s\n", szExpireTime);
	printf("------------------------------------------------- \n");
}

void CPKProcessMgr::LogMessage(const std::string &module_name, const std::string &message )
{
	if (m_pfnLogCallBack)
	{
		m_pfnLogCallBack(module_name, message, m_pCallbackParams);
	}
}

void CPKProcessMgr::NotifyAllProcessStop(bool bRestart, bool bIncludePdb, bool bDisableRestart, bool bDisplayLog)
{
	CVPROCESSINFOLIST::reverse_iterator itProc = m_processList.rbegin();
	for(; itProc != m_processList.rend(); itProc ++)
	{
		CPKProcess * pCVProc = *itProc;
		if(!bIncludePdb && m_pPdbProcess == pCVProc)
			continue;

		//if (!bRestart || pCVProc->m_bRestartEnable)
		{
			long lRet = pCVProc->NotifyProcessToStop(bDisableRestart, bDisplayLog);
			if (PK_SUCCESS != lRet)
			{
				PKLog.LogMessage(PK_LOGLEVEL_WARN, _("Failed to stop process %s"), pCVProc->m_szExeName);
			}
		}
	}
}

void CPKProcessMgr::NotifyAllProcessRefresh(bool bIncludePdb)
{
	CVPROCESSINFOLIST::reverse_iterator itProc = m_processList.rbegin();
	for(; itProc != m_processList.rend(); itProc ++)
	{
		CPKProcess * pCVProc = *itProc;
		if(!bIncludePdb && m_pPdbProcess == pCVProc)
			continue;

		//if (!bRestart || pCVProc->m_bRestartEnable)
		{
			long lRet = pCVProc->NotifyProcessToRefresh();
			if (PK_SUCCESS != lRet)
			{
				PKLog.LogMessage(PK_LOGLEVEL_WARN, _("Failed to stop process %s"), pCVProc->m_szExeName);
			}
		}
	}
}

CPKProcess* CPKProcessMgr::FindProcessByName( const char* szProcessName )
{
	CVPROCESSINFOLIST::iterator it = m_processList.begin();
	int index = 0;
	for (; it != m_processList.end(); ++it, ++index)
	{
		CPKProcess *pProc = *it;
		if (PKStringHelper::StriCmp(pProc->GetModuleName(), szProcessName) == 0)
		{
			return pProc;
		}
	}//*/
	return NULL;
}

void CPKProcessMgr::RestartPdb()
{
	if (IsGeneralPM())
		return;

	if(!m_pPdbProcess)
		return;

	m_pPdbProcess->RestartProcess();
}

int CPKProcessMgr::DelManageTagsInPdb()
{
	if (IsGeneralPM())
		return 0;

	CVPROCESSINFOLIST::iterator it = m_processList.begin();
	for (; it != m_processList.end(); ++it)
	{
		CPKProcess *pProc = *it;
		pProc->DelManageTagInPdb();
	}
	return PK_SUCCESS;
}

int CPKProcessMgr::InitLogConfTagsInPdb()
{
	if (IsGeneralPM())
		return 0;

	int nRet = 0;
	CVPROCESSINFOLIST::iterator it = m_processList.begin();
	for (; it != m_processList.end(); ++it)
	{
		CPKProcess *pProc = *it;
		if(nRet == 0)
		{
			nRet = pProc->InitLogConfTagInPdb();
		}

		if(nRet != 0)
		{
			// ˵��redis����ʧ�ܣ�
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "����[%s, %s]���ڴ�������־����ʱʧ��(������:%d), �����ڴ����ݿ��Ƿ�����������!", 
				pProc->m_szDispName, pProc->m_szExeName, nRet);
			break;
		}
	}
	return PK_SUCCESS;
}

// replace all occurance of t in s to w  
void replace_all(std::string & s, std::string const & t, std::string const & w)  
{  
	string::size_type pos = s.find(t), t_size = t.size(), r_size = w.size();  
	while(pos != std::string::npos){ // found   
		s.replace(pos, t_size, w);   
		pos = s.find(t, pos + r_size );   
	}  
}  

string trim(string const & s, char c = ' ')  
{  
	string::size_type a = s.find_first_not_of(c);  
	string::size_type z = s.find_last_not_of(c);  
	if(a == string::npos){  
		a = 0;  
	}  

	if(z == string::npos){  
		z = s.size();  
	}  
	return s.substr(a, z);  
}  

void CPKProcessMgr::ProcessWebServerConfig()
{
	// ����������pdb����������SetStatus�ᱨredis����pdb���ǵ�һ�����̣���
	CVPROCESSINFOLIST::iterator itProc = m_processList.begin();
	for(; itProc != m_processList.end(); itProc ++)
	{
		CPKProcess * pCVProc = *itProc;
		if(!pCVProc->IsWebServer())
		if(PKStringHelper::StriCmp(pCVProc->m_szExeName, "pkwebserver") != 0)
			continue;

		// �õ�webĿ¼
		string strWebPath = PKComm::GetHomePath(); // d:\work\eview\bin
		//int nPos = strWebPath.rfind(ACE_DIRECTORY_SEPARATOR_STR);
		//if(nPos >= 0)
		//	strWebPath = strWebPath.substr(0, strWebPath.length() - 2); // d:\work\eview

		// web���� "d:\jdks\jdk1.7.0_67\\bin\java.exe"   -Djava.util.logging.config.file="d:\Work\eview\web\tomcat\conf\logging.properties" -Djava.util.logging.manager=org.apache.juli.ClassLoaderLogManager   -Djava.endorsed.dirs="d:\Work\eview\web\tomcat\endorsed" -classpath "%EVIW_HOME%\web\tomcat\bin\bootstrap.jar;%EVIW_HOME%\web\tomcat\bin\tomcat-juli.jar" -Dcatalina.base="%EVIW_HOME%\web\tomcat" -Dcatalina.home="d:\Work\eview\web\tomcat" -Djava.io.tmpdir="d:\Work\eview\web\tomcat\temp" org.apache.catalina.startup.Bootstrap  start
		string strJavaHome = strWebPath + ACE_DIRECTORY_SEPARATOR_STR + "web" + ACE_DIRECTORY_SEPARATOR_STR + "jre";
		string strEviewHome = strWebPath;
		string strJavaOptions = "\"%JAVA_HOME%\\bin\\java.exe\"   -Djava.util.logging.config.file=\"%EVIEW_HOME%\\web\\tomcat\\conf\\logging.properties\" -Djava.util.logging.manager=org.apache.juli.ClassLoaderLogManager   -Djava.endorsed.dirs=\"%EVIEW_HOME%\\web\\tomcat\\endorsed\" -classpath \"%EVIEW_HOME%\\web\\tomcat\\bin\\bootstrap.jar;%EVIEW_HOME%\\web\\tomcat\\bin\\tomcat-juli.jar\" -Dcatalina.base=\"%EVIEW_HOME%\\web\\tomcat\" -Dcatalina.home=\"%EVIEW_HOME%\\web\\tomcat\" -Djava.io.tmpdir=\"%EVIEW_HOME%\\web\\tomcat\\temp\" org.apache.catalina.startup.Bootstrap ";
		replace_all(strJavaOptions, "%JAVA_HOME%", strJavaHome);
		replace_all(strJavaOptions, "%EVIEW_HOME%", strEviewHome);
		string strStartCmd = strJavaOptions + " start";
		string strStopCmd = strJavaOptions + " stop";
		string strWorkingDir = strEviewHome + ACE_DIRECTORY_SEPARATOR_STR + "web";
		PKStringHelper::Safe_StrNCpy(pCVProc->m_szStartCmdAbsPath, strStartCmd.c_str(), sizeof(pCVProc->m_szStartCmdAbsPath));
		PKStringHelper::Safe_StrNCpy(pCVProc->m_szStopCmdAbsPath, strStopCmd.c_str(), sizeof(pCVProc->m_szStopCmdAbsPath));
		PKStringHelper::Safe_StrNCpy(pCVProc->m_szWorkingDirAbsPath, strWorkingDir.c_str(), sizeof(pCVProc->m_szWorkingDirAbsPath));
	}
}

int CPKProcessMgr::ToggleShow( int nProcNo )
{
	// ����������pdb����������SetStatus�ᱨredis����pdb���ǵ�һ�����̣���
	CPKProcess * pCVProc = NULL;
	int nTmp = 0;
	CVPROCESSINFOLIST::iterator itProc = m_processList.begin();
	for(; itProc != m_processList.end(); itProc ++)
	{
		nTmp ++;
		if(nTmp == nProcNo)
		{
			pCVProc = *itProc;
			break;
		}
	}

	if(!pCVProc)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "��ʾ���ش���: δ�ҵ���:%d������", nProcNo);
		return -1;
	}
	
	if(pCVProc->getpid() <= 0)
	{
		PKLog.LogMessage(PK_LOGLEVEL_INFO, "��ʾ���ش���: ��:%d�������޷���ʾ����", nProcNo);
		return 0;
	}

	pCVProc->ToggleConsoleStatus();


	return 0;
}
