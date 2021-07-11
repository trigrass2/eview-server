// CVProcess.h: interface for the CCVProcess class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(CVPROCESS_MNGR_H)
#define CVPROCESS_MNGR_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "pkProcess.h"
#include "ProcPluginLoad.h"
#include "pklic/pkLicense.h"

#include <string>
#include <list>
#include <vector>
#include <map>
using namespace std;

typedef list<CPKProcess *> CVPROCESSINFOLIST;

typedef void (*PFN_LogCallBack)(const std::string&, const std::string&, void*);

// 有效的驱动信息
class CDriverInfo
{
public:
	string m_strDriverName;
	string m_strModuleName;
	string m_strPlatform;
public:
	CDriverInfo()
	{
	}
};

class CPKProcessMgr
{
public:
	CPKProcessMgr();
	~CPKProcessMgr();
public:
	long ReadAllProcessConfig();
	void StopAllProcess(bool bRestart, bool bIncludePdb, bool bDisableRestart, bool bDisplayLog);

	void NotifyAllProcessStop(bool bRestart, bool bIncludePdb, bool bDisableRestart, bool bDisplayLog);
	long StartAllProcess(bool bIncludePdb);

	long InitProcessMgr();
	long FiniProcessMgr();

	void PrintAllProcessState();
	void PrintLicenseState();

	void MaintanceProcess();

	CPKProcess* FindProcessByName(const char* szProcessName);

	void LogMessage(const std::string &module_name, const std::string &message);

	void RegLogCallBack(PFN_LogCallBack pfn_callback, void* param)
	{
		m_pfnLogCallBack = pfn_callback;
		m_pCallbackParams = param;
	}

	inline void SetLicExpired(bool bLicExpired){m_bLicExpired = bLicExpired;}
	unsigned short		m_usPort;
	CVPROCESSINFOLIST	m_processList;
	PFN_LogCallBack		m_pfnLogCallBack;
	void*				m_pCallbackParams;

	//获取/设置当前io点数
	int GetCurrentIONum() {return m_nCurrentIONum;}
	void SetCurrentIONum(int nNum){m_nCurrentIONum = nNum;}
	void NotifyAllProcessRefresh(bool bIncludePdb);
	int ReadAllDriversConfig();
	int ReadDriverAndDevicesFromDB(std::vector<CDriverInfo> &vecDrv);
	void RestartPdb();
	int DelManageTagsInPdb();
	int InitLogConfTagsInPdb();
	int ReadAllServicesConfig();
	void ProcessWebServerConfig();
	bool IsCurTimeExpired();
	int ToggleShow( int nProcNo );
	bool IsGeneralPM(){
		if (m_strApplyFor.compare("peakinfo.cn") == 0)
			return true;
		return false;
	}
private:
	string m_strApplyFor;	// 仅仅检查列表，不检查驱动.peakinfo.cn则不检查
	unsigned int m_nCheckPeriod;	// 检查的周期
	int  m_nCurrentIONum;   //当前io点数

	PKLIC_HANDLE	m_hLicense;
	int			m_nLicStatus;
	time_t		m_tmExpire;	// 过期时间
	bool m_bLicExpired;

	CProcPluginLoad m_objProcPlugin;
	time_t	m_tmLastMaintance;
	CPKProcess	*m_pPdbProcess;
	map<string,int>	m_mapExcludeSrvDrv; // 不启动的驱动或程序（ExeName）
};


#endif // !defined(CVPROCESS_MNGR_H)
