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
	m_nCheckPeriod = CHECK_PERIOD_DEFAULT; // 缺省检查周期为60秒
}

CPKProcessMgr::~CPKProcessMgr()
{
	m_objProcPlugin.UnLoad();
}

int CPKProcessMgr::ReadAllServicesConfig()
{
	// sqlite的配置文件路径
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

	TiXmlElement* pElemRoot = pDoc->RootElement(); // Servers节点
	if(pElemRoot == NULL)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "services config file:%s has no root node", szCfgPathName);
		delete pDoc;
		return -2;
	}

	// 是否仅仅根据列表检查
	const char *szApplyFor = pElemRoot->Attribute("app");
	if (szApplyFor && strlen(szApplyFor) > 0)
		m_strApplyFor = szApplyFor;

	const char *szCheckPeriod = pElemRoot->Attribute("CheckPeriod");
	if(szCheckPeriod && strlen(szCheckPeriod) > 0)
	{
		m_nCheckPeriod = ::atoi(szCheckPeriod) != 0;
		if(m_nCheckPeriod <= 0 || m_nCheckPeriod > 60*60*24) // 1天
			m_nCheckPeriod = CHECK_PERIOD_DEFAULT;
	}

	TiXmlElement* pElemExclude = pElemRoot->FirstChildElement("Exclude");
	for(; pElemExclude != NULL; pElemExclude = pElemExclude->NextSiblingElement())
	{
		// 模块名称，最终运行的exe
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
		// 模块名称，最终运行的exe
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

		// 显示名称
		string strDisplayName;
		char *szDisplayName = (char *)pElemService->Attribute("Name");
		if (!szDisplayName)
			strDisplayName = szExeName;
		else
			strDisplayName = szDisplayName;

		char *szStartCmd = (char *)pElemService->Attribute("StartCmd");
		char *szStopCmd = (char *)pElemService->Attribute("StopCmd");

		// 当前exe的目录
		string strExeDir;
		char *szExeDir = (char *)pElemService->Attribute("ExeDir");
		if (szExeDir)
			strExeDir = szExeDir;

		// new 线程
		CPKProcess *pCVProcess = new CPKProcess((char *)szExeName, (char *)strExeDir.c_str(), (char *)strDisplayName.c_str(), szStartCmd, szStopCmd, "", 0, this);
		pCVProcess->m_bIsDriverExe = false;
		// 服务参数
		char *szParameter = (char *)pElemService->Attribute("Parameter");
		if(szParameter)
			sprintf(pCVProcess->m_szArguments, " %s", szParameter); // 参数，配置文件

		// 是否允许自动重启
		char *szAutoRestart = (char *)pElemService->Attribute("AutoRestart");
		if(szAutoRestart)
			pCVProcess->m_bRestartEnable = ::atoi(szAutoRestart) == 0 ?false:true;
		else
			pCVProcess->m_bRestartEnable = true;
		pCVProcess->m_bRestartEnableInit = pCVProcess->m_bRestartEnable;

		// 是否显示控制台窗口
		char *szShowConsole = (char *)pElemService->Attribute("ShowConsole");
		if(szShowConsole)
			pCVProcess->m_bShowConsole = ::atoi(szShowConsole) == 0 ?false:true;
		else
			pCVProcess->m_bShowConsole = false;

		// 启动后延迟时间
		char *szDelaySec = (char *)pElemService->Attribute("DelaySec");
		if(szDelaySec)
			pCVProcess->m_lSleepSeconds = ::atoi(szDelaySec);
		if(pCVProcess->m_lSleepSeconds < 0)
			pCVProcess->m_lSleepSeconds = 0;

		if(strcmp(szExeName, MODULENAME_MEMDB) == 0) // 是不是pdb
		{
			pCVProcess->m_bShouldUpdateStatusInPdb = false; // pdb自己显然不需要更新自己
			//PKLog.LogMessage(PK_LOGLEVEL_INFO, "进程:%s 设置为不更新状态到内存数据库!!", szExeName);
			m_pPdbProcess = pCVProcess;
			string strPdbConfigFile = PKComm::GetConfigPath();
			strPdbConfigFile = strPdbConfigFile + ACE_DIRECTORY_SEPARATOR_STR + "pkmemdb.conf";
			sprintf(pCVProcess->m_szArguments, " %s", strPdbConfigFile.c_str()); // 参数，配置文件
		}
		else
			pCVProcess->m_bShouldUpdateStatusInPdb = true;

		if (strcmp(szExeName, MODULENAME_HISDB) == 0) // 是不是hisdb
		{
			pCVProcess->m_bShouldUpdateStatusInPdb = true; // pdb自己显然不需要更新自己
			string strConfigFile = PKComm::GetConfigPath();
			strConfigFile = strConfigFile + ACE_DIRECTORY_SEPARATOR_STR + "pkhisdb.conf";
			sprintf(pCVProcess->m_szArguments, " -f %s", strConfigFile.c_str()); // 参数，配置文件
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
	// 读取所有的驱动的列表
	vector<vector<string> > vecDevice;
	nRet = PKEviewDbApi.Init();
	if (nRet != 0)
	{
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "初始化数据库失败!");
		PKEviewDbApi.Exit();
		return -1;
	}

	PKStringHelper::Snprintf(szSql, sizeof(szSql), "SELECT B.id as drv_id,B.name as drv_name,B.modulename as drv_modulename,B.description as drv_description,B.platform as drv_platform, B.enable as drv_enable, A.id as dev_id, A.name as dev_name, A.description as dev_description,A.simulate as dev_simulate , A.enable as dev_enable FROM t_device_list A, t_device_driver B where A.driver_id = B.id and (B.enable is null or B.enable<>0) and (A.enable is null or A.enable<>0)");
	string strError;
	nRet = PKEviewDbApi.SQLExecute(szSql, vecDevice, &strError);
	if (nRet != 0)
	{
		PKEviewDbApi.Exit();
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "查询数据库失败,错误码:%d,SQL:%s, error:%s", nRet,szSql, strError.c_str());
		return -2;
	}

	if (vecDevice.size() <= 0)
	{
		PKEviewDbApi.Exit();
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "未找到任何可用的驱动, SQL:%s!", szSql);
		return -1;
	}

	PKEviewDbApi.Exit();

	for (int iDev = 0; iDev < vecDevice.size(); iDev++)
	{
		vector<string> &row = vecDevice[iDev];
		int iCol = 0;
		// 驱动的信息
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

		// 设备的信息
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

		// 禁用的驱动或者设备不需要考虑
		if (strDeviceEnable.compare("0") == 0)
			continue;
		if (strDeviceEnable.compare("0") == 0)
			continue;

		// 模拟设备的话，直接加载模拟驱动
		if (bDeviceSimulate)
		{
			drvInfo.m_strModuleName = "simdriver";
			drvInfo.m_strDriverName = "simdriver";
		}

		// 如果没有重复的驱动加入，则加入该驱动
		bool bFound = false;
		for (vector<CDriverInfo>::iterator itDrv = vecDriver.begin(); itDrv != vecDriver.end(); itDrv++)
		{
			if (itDrv->m_strDriverName.compare(drvInfo.m_strDriverName) == 0) // 驱动已经加入了则不需要处理
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
	// 如果是通用守护程序，那么是不用读取eview的驱动列表的
	if (IsGeneralPM())
		return 0;

	vector<CDriverInfo> vecDriver;
	ReadDriverAndDevicesFromDB(vecDriver);

	int nRet = 0;
	int nValidDriverNum = 0;

	// pkdriver.exe的路径
	string strTemplDriverPath = PKComm::GetBinPath();
	strTemplDriverPath = strTemplDriverPath + ACE_DIRECTORY_SEPARATOR_STR + "pkdriver.exe";
	printf("+============================================================+\n");
	// 只有配置了设备的驱动才需要运行起来！
	for (int iDrv = 0; iDrv < vecDriver.size(); iDrv++)
	{
		CDriverInfo &drv = vecDriver[iDrv];
		CPKProcess *pCVProcess = new CPKProcess((char *)drv.m_strModuleName.c_str(), "", (char *)drv.m_strDriverName.c_str(), NULL, NULL, (char *)drv.m_strPlatform.c_str(), 0, this);
		pCVProcess->m_bRestartEnable = true;
		pCVProcess->m_bShowConsole = false;
		pCVProcess->m_bIsDriverExe = true; // 是驱动，做个标识
		PKStringHelper::Safe_StrNCpy(pCVProcess->m_szArguments, "--silent", sizeof(pCVProcess->m_szArguments));
		pCVProcess->GenerateAbsPaths(true);

		string strDriverDir = PKComm::GetBinPath(); // 驱动目录
		strDriverDir = strDriverDir + ACE_DIRECTORY_SEPARATOR_STR + "drivers" + ACE_DIRECTORY_SEPARATOR_STR + pCVProcess->m_szExeName + ACE_DIRECTORY_SEPARATOR_STR;
		// 如果驱动模块exe不存在，但目录存在，且模板驱动pkdriver存在，那么拷贝pkdriver.exe到驱动目录
		if (PKFileHelper::IsFileExist(strTemplDriverPath.c_str()) && PKFileHelper::IsDirectoryExist(strDriverDir.c_str()))
		{
#ifdef _WIN32
            CopyFile(strTemplDriverPath.c_str(), pCVProcess->m_szExeAbsPath, false);
#endif
		}

		if (!PKFileHelper::IsFileExist(pCVProcess->m_szExeAbsPath))
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "驱动模块不存在: %s, 路径:%s ", drv.m_strDriverName.c_str(), pCVProcess->m_szExeAbsPath);
		}
		else
		{
			PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "已读取驱动配置: %s", drv.m_strDriverName.c_str());

			m_processList.push_back(pCVProcess);
			// pCVProcess->DumpProcessInfo();
			nValidDriverNum++;
		}
	}

	if (vecDriver.size() != nValidDriverNum)
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, _("驱动列表共有 %d 个驱动, 其中无效驱动个数: %d!"), nValidDriverNum, vecDriver.size() - nValidDriverNum);
	else
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, _("驱动列表共有 %d 个驱动!"), nValidDriverNum);
	printf("+============================================================+\n\n");
	return nRet;
}


long CPKProcessMgr::ReadAllProcessConfig()
{
	// 加载服务程序
	ReadAllServicesConfig();

	ProcessWebServerConfig(); // WebServer 是特殊的配置

	// 读取驱动的配置
	ReadAllDriversConfig();

	return PK_SUCCESS;
}

long CPKProcessMgr::StartAllProcess( bool bIncludePdb)
{
	printf("+============================================================+\n");
	// 必须先启动pdb，否则后面的SetStatus会报redis错误。pdb总是第一个进程！！
	CVPROCESSINFOLIST::iterator itProc = m_processList.begin();
	if(bIncludePdb)
	{
		for(; itProc != m_processList.end(); itProc ++)
		{
			CPKProcess * pCVProc = *itProc;
			if(pCVProc->IsMemDB())
			{
				pCVProc->StartProcess();
				ACE_OS::sleep(1); // 启动pdb后暂停1秒，以便pdb可以完全启动起来
			}
		}
	}

	printf("+============================================================+\n");
	// 继续启动其他进程，服务程序
	itProc = m_processList.begin();
	for(; itProc != m_processList.end(); itProc ++)
	{
		CPKProcess * pCVProc = *itProc;
		pCVProc->m_bRestartEnable = pCVProc->m_bRestartEnableInit;

		if(pCVProc->IsMemDB() || pCVProc->m_bIsDriverExe)
			continue;

		// 如果需要等待一段时间再启动下一个程序，那么等待
		if(pCVProc->m_lSleepSeconds > 0)
			ACE_OS::sleep(pCVProc->m_lSleepSeconds);

		pCVProc->StartProcess();
	}

	printf("+============================================================+\n");
	// 继续启动其他进程，驱动程序
	itProc = m_processList.begin();
	for (; itProc != m_processList.end(); itProc++)
	{
		CPKProcess * pCVProc = *itProc;
		pCVProc->m_bRestartEnable = pCVProc->m_bRestartEnableInit;

		if (pCVProc->IsMemDB() || !pCVProc->m_bIsDriverExe)
			continue;

		// 如果需要等待一段时间再启动下一个程序，那么等待
		if (pCVProc->m_lSleepSeconds > 0)
			ACE_OS::sleep(pCVProc->m_lSleepSeconds);

		pCVProc->StartProcess();
	}
	return PK_SUCCESS;
}

void CPKProcessMgr::StopAllProcess(bool bRestart, bool bIncludePdb, bool bDisableRestart, bool bDisplayLog)
{
	if(bRestart) // 重启操作允许重启
		bDisableRestart = false;

	NotifyAllProcessStop(bRestart,bIncludePdb, bDisableRestart, bDisplayLog);

	// 设置所有进程的最长等待时间
	int nSecTimeOut = 10;
	ACE_Time_Value tv = ACE_OS::gettimeofday();
	tv += ACE_Time_Value(nSecTimeOut);

	// 如果不先通知，可能导致结果：windows 服务通知了前面少数几个进程就强制退出了（限制时间10秒），导致其他进程都没有关闭
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
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, _("所有进程(个数:%d) 已被通知停止, 最多等待%d秒..."), m_processList.size(), nSecTimeOut);
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
			PKLog.LogMessage(PK_LOGLEVEL_DEBUG, _("等待进程 %s 停止..."), pCVProc->GetModuleName());
			pCVProc->WaitProcessToStop(tv, true);
			PKLog.LogMessage(PK_LOGLEVEL_DEBUG, _("进程 %s 已停止."), pCVProc->GetModuleName());
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
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, _("所有进程(个数:%d)已经停止!"), m_processList.size());
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
	printf("+=============================进程管理=====================================+\n");
	printf("%-6.6s %-24.24s %-15.15s %-25.25s %-6.6s %-12.12s\n", "No.","process","status","start time","count","pid");
	for(CVPROCESSINFOLIST::iterator itProc = m_processList.begin(); itProc != m_processList.end(); itProc ++)
	{
		CPKProcess * pCVProc = *itProc;
		string strStatus;
		iProcess ++;
		pCVProc->GetProcessState(strStatus);
		printf("No:%-3x %-24.24s %-15.15s %-25.25s %-6d %-12d\n", iProcess, pCVProc->m_szDispName, strStatus.c_str(), pCVProc->m_strLastRestartTime.c_str(), pCVProc->m_nRestartCount, pCVProc->getpid());
	}
	printf("+=====================输入数字编号【16进制】可以显示/隐藏相应进程窗口==================+\n\n");

}

long CPKProcessMgr::InitProcessMgr()
{
	// 读取环境变量
	char *szBindir = (char *)PKComm::GetBinPath();
	PKLog.LogMessage(PK_LOGLEVEL_DEBUG, _("当前bin目录: %s"), szBindir);

	//执行插件
	m_objProcPlugin.Load(szBindir);
	this->ReadAllProcessConfig();

	// 启动第一个pdb任务
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
	// 如果小于10秒则不作检查
	time_t tmNow;
	time(&tmNow);
	if(labs(tmNow - m_tmLastMaintance) < m_nCheckPeriod)
		return;

	for(CVPROCESSINFOLIST::iterator itProc = m_processList.begin(); itProc != m_processList.end(); itProc ++)
	{
		CPKProcess * pCVProc = *itProc;
		bool bAlive = pCVProc->IsProcessAlive();
		pCVProc->UpdateStatusInPdb(bAlive);
		
		// 程序已经停止，检查是否需要重启
		if (!bAlive)
		{
			// 如果进程不存在，就不需要启动了，而且也不需要打印日志
			if(!PKFileHelper::IsFileExist(pCVProc->m_szExeAbsPath))
				continue;

			// 手工停止，不能重启
			if (!pCVProc->m_bRestartEnable)
				continue;

			// 许可证过期
			if (m_bLicExpired)
				continue;

			//首次检测到进程退出
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
	if(m_tmExpire - tmNow >= 0) //在有效期范围内
		return false;
	else
		return true;
}

void CPKProcessMgr::PrintLicenseState()
{
	if (IsGeneralPM())
		return;

	//获取文件路径
	const char * szCVCfgPath = PKComm::GetConfigPath();
	if(!szCVCfgPath)
	{
		printf("pkOpenLic, 获取配置文件目录失败,请检查目录\n");
		return;
	}

	char szLicFilePath[1024] = {0};
	//路径+分隔符+文件名
	sprintf(szLicFilePath, "%s" ACE_DIRECTORY_SEPARATOR_STR "license.pkl", szCVCfgPath);

	#define CV_LICENSE_TEMPORARY_VALIDATE (60*60*2) 
	char szExpireTime[32] = {0};
	char szErrTip[256] = {0};
	long lRet = pkOpenLic(szLicFilePath, &m_hLicense, &m_nLicStatus, szErrTip, sizeof(szErrTip));
	if(m_nLicStatus == PK_LIC_STATUS_EXIST_VALID) // 有效许可证，可得到日期
	{
		pkGetLicValue(m_hLicense, PK_LIC_KEY_EXPIRE_DATE, szExpireTime, sizeof(szExpireTime));
		strcat(szExpireTime, " 23:59:59"); //yyyy-mm-dd hh:mm:ss
		m_tmExpire = PKTimeHelper::String2Time(szExpireTime);
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "加载许可证成功, 有效许可证，到期时间：%s", szExpireTime);
	}
	else if(m_nLicStatus == PK_LIC_STATUS_NOEXIST) // 有效许可证，可得到日期
	{
		time(&m_tmExpire);
		m_tmExpire += CV_LICENSE_TEMPORARY_VALIDATE;
		PKTimeHelper::Time2String(szExpireTime, sizeof(szExpireTime),m_tmExpire);
	}

	bool bExpired = IsCurTimeExpired();

	char szLicStatus[32] = {0};
	if(m_nLicStatus == PK_LIC_STATUS_NOEXIST)
	{
		strcpy(szLicStatus, "许可证不存在,两小时试用模式");
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, szLicStatus);
	}
	else if(m_nLicStatus == PK_LIC_STATUS_EXIST_FILE_OPENFAILED)
	{
		strcpy(szLicStatus, "许可证文件格式非法");
	}
	else if(m_nLicStatus == PK_LIC_STATUS_EXIST_EXPIRED)
	{
		strcpy(szLicStatus, "许可证已过期");
	}
	else if(m_nLicStatus == PK_LIC_STATUS_EXIST_UNDUE)
	{
		strcpy(szLicStatus, "许可证尚未到期");
	}
	else if(m_nLicStatus == PK_LIC_STATUS_EXIST_HOSTID_NOT_MATCH)
	{
		strcpy(szLicStatus, "许可证主机不匹配");
	}
	else if(m_nLicStatus == PK_LIC_STATUS_EXIST_SIGNATURE_NOTMATCH)
	{
		strcpy(szLicStatus, "许可证签名不匹配");
	}
	else // if(m_nLicStatus == PK_LIC_STATUS_EXIST_VALID)
	{
		strcpy(szLicStatus, "有效");
	}

	PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "许可证状态: %s, 有效期: %s", szLicStatus,szExpireTime);

	printf("--------------------节点许可证信息------------------ \n");
	printf("\t许可证类型：\t%s\n", szLicStatus);
	printf("\t许可有效期：\t%s\n", szExpireTime);
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
			// 说明redis连接失败！
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "进程[%s, %s]在内存库更新日志配置时失败(错误码:%d), 请检查内存数据库是否已正常启动!", 
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
	// 必须先启动pdb，否则后面的SetStatus会报redis错误。pdb总是第一个进程！！
	CVPROCESSINFOLIST::iterator itProc = m_processList.begin();
	for(; itProc != m_processList.end(); itProc ++)
	{
		CPKProcess * pCVProc = *itProc;
		if(!pCVProc->IsWebServer())
		if(PKStringHelper::StriCmp(pCVProc->m_szExeName, "pkwebserver") != 0)
			continue;

		// 得到web目录
		string strWebPath = PKComm::GetHomePath(); // d:\work\eview\bin
		//int nPos = strWebPath.rfind(ACE_DIRECTORY_SEPARATOR_STR);
		//if(nPos >= 0)
		//	strWebPath = strWebPath.substr(0, strWebPath.length() - 2); // d:\work\eview

		// web服务 "d:\jdks\jdk1.7.0_67\\bin\java.exe"   -Djava.util.logging.config.file="d:\Work\eview\web\tomcat\conf\logging.properties" -Djava.util.logging.manager=org.apache.juli.ClassLoaderLogManager   -Djava.endorsed.dirs="d:\Work\eview\web\tomcat\endorsed" -classpath "%EVIW_HOME%\web\tomcat\bin\bootstrap.jar;%EVIW_HOME%\web\tomcat\bin\tomcat-juli.jar" -Dcatalina.base="%EVIW_HOME%\web\tomcat" -Dcatalina.home="d:\Work\eview\web\tomcat" -Djava.io.tmpdir="d:\Work\eview\web\tomcat\temp" org.apache.catalina.startup.Bootstrap  start
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
	// 必须先启动pdb，否则后面的SetStatus会报redis错误。pdb总是第一个进程！！
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
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "显示隐藏窗口: 未找到第:%d个进程", nProcNo);
		return -1;
	}
	
	if(pCVProc->getpid() <= 0)
	{
		PKLog.LogMessage(PK_LOGLEVEL_INFO, "显示隐藏窗口: 第:%d个进程无法显示隐藏", nProcNo);
		return 0;
	}

	pCVProc->ToggleConsoleStatus();


	return 0;
}
