#include "common/RMAPI.h"
#include "errcode/ErrCode_iCV_RM.hxx"
#include "../rmservice/ShareMemHelper.h"
#include "pkcomm/pkcomm.h"

#include <ace/Singleton.h>
#include <ace/Null_Condition.h>
#include <ace/ACE.h>
#include <ace/OS_NS_sys_time.h>
#include <ace/OS_NS_sys_stat.h>
#include "common/eviewdefine.h"
//#include "common/Quality.h"

class RMAPI_GlobleObject
{
	friend class ACE_Unmanaged_Singleton<RMAPI_GlobleObject, ACE_Thread_Mutex>;
public:
	RMAPI_GlobleObject() : m_bInit(false) 
	{
		///m_nFini = ACE::init();
	};
	~RMAPI_GlobleObject()
	{
		if (!m_nFini)
		{
			//ACE::fini();
		}
		
	}

	CShareMemHelper m_ShareMemHelper;
	bool m_bInit ;
	int m_nFini;
	string m_strShmPath;
};

#define g_ShareMemHelper ACE_Unmanaged_Singleton<RMAPI_GlobleObject, ACE_Thread_Mutex>::instance()->m_ShareMemHelper
#define g_bInit ACE_Unmanaged_Singleton<RMAPI_GlobleObject, ACE_Thread_Mutex>::instance()->m_bInit
#define g_strShmPath ACE_Unmanaged_Singleton<RMAPI_GlobleObject, ACE_Thread_Mutex>::instance()->m_strShmPath

/**
 *  Initialize.
 *
 *
 *  @version     05/29/2008  chenzhiquan  Initial Version.
 */
RMAPI_DLLEXPORT int RMAPI_Init()
{
	int nErr = PK_SUCCESS;
	return nErr;



	if (NULL == PKComm::GetRunTimeDataPath())
	{
		return EC_ICV_COMM_GETENVFAIL;
	}

	string strPath = PKComm::GetRunTimeDataPath();
	ACE_OS::mkdir(strPath.c_str());
	strPath += ACE_DIRECTORY_SEPARATOR_STR;
	strPath += DEFAULT_SHM_FILE_NAME;

	if (g_strShmPath.empty() || strPath.compare(g_strShmPath) != 0)
	{
		g_bInit = false;
	}

	if (!g_bInit)
	{
		nErr = g_ShareMemHelper.InitializeMM(strPath, false);
		if (nErr  == PK_SUCCESS)
		{
			g_bInit = true;
			g_strShmPath = strPath;
		}
	}
	return nErr;
}

/**
 *  Get Status.
 *
 *  @param  -[in,out]  long*  pnStatus: [comment]
 *
 *  @version     05/29/2008  chenzhiquan  Initial Version.
 */
RMAPI_DLLEXPORT int GetRMStatus( int* pnStatus )
{
	*pnStatus = RM_STATUS_ACTIVE;
	return PK_SUCCESS;

	int nErr = RMAPI_Init();
	if (nErr == PK_SUCCESS)
	{
		nErr = g_ShareMemHelper.GetStatus(*pnStatus);
	}

	return nErr;
}

/**
 *  Set Status Ctrl.
 *
 *  @param  -[in,out]  int  nStatus: [comment]
 *
 *  @version     05/29/2008  chenzhiquan  Initial Version.
 */
RMAPI_DLLEXPORT int SetRMStatus(int nStatus)
{
	return PK_SUCCESS;
	int nErr = RMAPI_Init();
	if (nErr == PK_SUCCESS)
	{
		nErr = g_ShareMemHelper.SetStatusCtrl(nStatus);
	}
	
	return nErr;
}
/**
 *  Get object RM status.
 *
 *  @param  -[in,out]  int  pnRMStatus: Object's RM status
 *  @param  -[in] const char *pszObjName:  Object Name.
 *  @return -0 if success
 */
RMAPI_DLLEXPORT int GetObjRMStatus(const char *pszObjName, int* pnRMStatus)
{
	int nErr = RMAPI_Init();
	if (nErr == PK_SUCCESS)
	{
		nErr = g_ShareMemHelper.GetObjRMStatus(*pnRMStatus,pszObjName);
	}
	
	return nErr;
}
/**
 *  Switch object RM status.
 *
 *  @param  -[in]  int  nStatus: Object's RM status
 *  @param  -[in] const char *pszObjName:  Object Name.
 *  @return - 0 if success
 */
RMAPI_DLLEXPORT int SwitchObjRMStatus(const char *pszObjName, int nStatus)
{
	int nErr = RMAPI_Init();
	if (nErr == PK_SUCCESS)
	{
		nErr = g_ShareMemHelper.SetObjRMStatusCtrl(nStatus,pszObjName);
	}
	
	return nErr;
}
/**
 *  Update object status.
 *
 *  @param  -[in]  int  nStatus: Object's status
 *  @param  -[in] const char *pszObjName:  Object Name.
 *  @return - 0 if success
 */
RMAPI_DLLEXPORT int SetObjStatus(const char *pszObjName, int nStatus)
{
	int nErr = RMAPI_Init();
	if (nErr == PK_SUCCESS)
	{
		nErr = g_ShareMemHelper.SetObjStatus(nStatus,pszObjName);
	}
	
	return nErr;
}

/**
 *  Get object RM info.
 *
 *  @param  -[in] const char *pszObjName:  Object Name.
 *  @param  -[out]  bool bNeedSync: Need synchronize data or not
 *  @param  -[out]  bool bPrim:	Current object is primer object or not
 *  @param  -[out] int bSingle: Current object runmode is single or not
 *  @return - 0 if success
 */
RMAPI_DLLEXPORT int GetObjRMInfo(const char *pszObjName, bool *bNeedSync, bool *bPrim, int *bSingle)
{
	int nErr = RMAPI_Init();
	if (nErr == PK_SUCCESS)
	{
		nErr = g_ShareMemHelper.GetObjRMInfo(pszObjName, bNeedSync, bPrim, bSingle);
	}
	
	return nErr;
}
/**
 *  Object need synchronize data or not.
 *
 *  @param  -[in] const char *pszObjName:  Object Name.
 *  @param  -[out]  bool *pnNeedSync: Need synchronize data or not
 *  @return - 0 if success
 */
RMAPI_DLLEXPORT int ObjNeedSync(const char *pszObjName, bool *pnNeedSync)
{
	int nErr = RMAPI_Init();
	if (nErr == PK_SUCCESS)
	{
		nErr = g_ShareMemHelper.NeedSync(pszObjName, *pnNeedSync);
	}
	
	return nErr;
}
/**
 *  Object need do service or not.
 *
 *  @param  -[in] const char *pszObjName:  Object Name.
 *  @param  -[out]  bool *pnNeedDoSvc: Need do service or not
 *  @return - 0 if success
 */
RMAPI_DLLEXPORT int ObjNeedDoSvc(const char *pszObjName, bool *pnNeedDoSvc)
{
	int nErr = RMAPI_Init();
	*pnNeedDoSvc = false;
	if (nErr == PK_SUCCESS)
	{
		nErr = g_ShareMemHelper.NeedDoSvc(pszObjName, *pnNeedDoSvc);
	}
	
	return nErr;
}

