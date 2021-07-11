/**************************************************************
 *  Filename:    ShareMemHelper.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: Share Memory RW Helper.
 *
 *  @author:     chenzhiquan
 *  @version     05/28/2008  chenzhiquan  Initial Version
**************************************************************/
#include "ShareMemHelper.h"
#include "errcode/ErrCode_iCV_RM.hxx"
#include "common/RMAPI.h"
#include <ace/OS_NS_sys_time.h>
#include <ace/Guard_T.h>
#include "pkcomm/pkcomm.h"

//RETURN CODE OF REDUNDANCY MANAGEMENT
#define RM_STATUS_CTRL_INACTIVE		RM_STATUS_INACTIVE
#define RM_STATUS_CTRL_ACTIVE		RM_STATUS_ACTIVE
#define RM_STATUS_CTRL_NIL			RM_STATUS_UNAVALIBLE + 1
#define RM_STATUS_CTRL_ACK			RM_STATUS_UNAVALIBLE + 2

// RM Object status
#define ICV_RM_OBJ_STATUS_READY			1
#define ICV_RM_OBJ_STATUS_NOT_READY		2


/**
 *  Constructor.
 *
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 */
CShareMemHelper::CShareMemHelper() : m_pShm(0), 
		m_mutexStatus(ACE_TEXT("CV_RM_P_MUTEX_STATUS")),
		//m_mutexCtrl(ACE_TEXT("CV_RM_P_MUTEX_CTRL")),
		//m_mutexObjBlock(ACE_TEXT("CV_RM_PMUTEX_OBJ_BLK")),
		m_pRMStatus(0), 	m_pRMStatusCtrl(0),
		m_pObjRMStatusBlock(0)
{

}

/**
 *  Destructor.
 *
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 */
CShareMemHelper::~CShareMemHelper()
{
// 	if (m_pShm)
// 	{
// 		m_pShm->free(m_pRMStatusBlock);
// 		m_pShm->free(m_pRMStatusCtrl);
// 		//m_pShm->remove();
// 	}
	if (m_pShm)
	{
		delete m_pShm;
		m_pShm = NULL;
	}
}


/**
 *  Do Initialize Stuffs.
 *
 *  @param  -[in,out]  const string&  strName: [comment]
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 */
int CShareMemHelper::InitializeMM( const string &strName, bool bInit)
{

	ACE_GUARD_RETURN(ACE_Process_Mutex, guard, m_mutexStatus, EC_ICV_RMSERVICE_INIT_SHARED_MEM_FAILURE);
	//m_pShm = new ACE_Shared_Memory_MM(strName.c_str(),sizeof(tagRMStatusBlock), O_RDWR|O_CREAT);

	if (m_pShm)
	{
		delete m_pShm;
		m_pShm = NULL;
	}

	// SCADA�ڵ������+������������
	m_pShm = new ACE_Shared_Memory_MM(strName.c_str(),sizeof(tagRMStatus) + sizeof(tagRMStatusCtrl) + ICV_RM_MAX_OBJNUM*sizeof(tagObjRMStatusBlock), O_RDWR|O_CREAT);

	if (m_pShm == NULL)
	{
		return EC_ICV_RMSERVICE_INIT_SHARED_MEM_FAILURE;
	}

	tagRMStatusBlock* pRMStatusBlock = (tagRMStatusBlock *) m_pShm->malloc(sizeof(tagRMStatusBlock) + ICV_RM_MAX_OBJNUM*sizeof(tagObjRMStatusBlock));
	if (bInit)
	{
		memset((char*)pRMStatusBlock, 0, sizeof(tagRMStatusBlock) + ICV_RM_MAX_OBJNUM*sizeof(tagObjRMStatusBlock));
	}
	
	// ����ʧ��
	if (!pRMStatusBlock)
	{
		m_pRMStatus = NULL;
		m_pRMStatusCtrl = NULL;
		m_pObjRMStatusBlock = NULL;
		m_pShm->remove();
		m_pShm = NULL;
		return EC_ICV_RMSERVICE_INIT_SHARED_MEM_FAILURE;
	}

	// ��ʼ�������ָ��
	m_pRMStatus = & pRMStatusBlock->status;
	m_pRMStatusCtrl = & pRMStatusBlock->ctrl;
	m_pObjRMStatusBlock = (tagObjRMStatusBlock*)((char *)pRMStatusBlock + sizeof(tagRMStatusBlock));

	return PK_SUCCESS;

}

// ��չ����ڴ��е���������
void CShareMemHelper::ResetMM()
{
	ACE_GUARD_RETURN(ACE_Process_Mutex, guard, m_mutexStatus, );

	if (m_pShm)
	{
		//m_mutexObjBlock.acquire_write();
		//GET CTRL
		memset(m_pObjRMStatusBlock, 0, ICV_RM_MAX_OBJNUM*sizeof(tagObjRMStatusBlock));

 		//m_mutexObjBlock.release();
	}
}

/**
 *  Get RM Status From Share Mem.
 *
 *  @param  -[in,out]  int&  nStatus: [comment]
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 */
int CShareMemHelper::GetStatus( int &nStatus )
{
	ACE_GUARD_RETURN(ACE_Process_Mutex, guard, m_mutexStatus, (EC_ICV_RMDS_API_FAIL_ACQUIRE_LOCK));

	// Checking if Initialize is finished.
	if (!m_pShm)
	{
		return EC_ICV_RMSERVICE_SHARED_MEM_NOT_INIT;
	}
	// Copy Memory To Local.
	tagRMStatus rmStatusBlock;

	{
		//m_mutexStatus.acquire();
		try
		{

			memcpy(&rmStatusBlock, m_pRMStatus, sizeof(tagRMStatus));
		}
		catch (...)
		{
			return EC_ICV_RMSERVICE_SHARED_MEM_NOT_INIT;
		}
		//m_mutexStatus.release();
	}
	
	// �������״ֵ̬��ʱ���
	CheckRMStatus(rmStatusBlock.nStatus, rmStatusBlock.timeStamp, nStatus);

	return PK_SUCCESS;
	
}

/**
 *  Setting Status To ShareMem.
 *
 *  @param  -[in,out]  int  nStatus: [comment]
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 */
int CShareMemHelper::SetStatus( int nStatus )
{
	ACE_GUARD_RETURN(ACE_Process_Mutex, guard, m_mutexStatus, (EC_ICV_RMDS_API_FAIL_ACQUIRE_LOCK));

	// Checking if Initialize is finished.
	if (!m_pShm)
	{
		return EC_ICV_RMSERVICE_SHARED_MEM_NOT_INIT;
	}
	// Copy Memory To Local.
	tagRMStatus rmstatus;
	rmstatus.nStatus = nStatus;
	rmstatus.timeStamp = ACE_OS::gettimeofday();

	//bool bCtrl = false;

	//LOCK

	//m_mutexStatus.acquire_write();
	//GET CTRL
	memcpy(m_pRMStatus, &rmstatus, sizeof(tagRMStatus));

 	//m_mutexStatus.release();

	return PK_SUCCESS;
}

/**
 *  Setting Status Ctrl To Share Mem.
 *
 *  @param  -[in,out]  int  nCtrl: [comment]
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 */
int CShareMemHelper::SetStatusCtrl(int nCtrl)
{
	ACE_GUARD_RETURN(ACE_Process_Mutex, guard, m_mutexStatus, (EC_ICV_RMDS_API_FAIL_ACQUIRE_LOCK));

	if (!m_pShm)
	{
		return EC_ICV_RMSERVICE_SHARED_MEM_NOT_INIT;
	}

	if (nCtrl != RM_STATUS_ACTIVE && nCtrl != RM_STATUS_CTRL_INACTIVE)
	{
		return EC_ICV_RMSERVICE_UNKOWN_STATUS_CTRL_CMD;
	}

	int nStatus;
	GetStatus(nStatus);
	if (nStatus == RM_STATUS_UNAVALIBLE)
	{
		return EC_ICV_RMSERVICE_SERVICE_UNAVALIBLE;
	}
tagRMStatusCtrl ctrl;
	ctrl.nCtrl = nCtrl;
	ctrl.nReadFlag = ICV_RM_STATUS_CTRL_UNREAD;
	ctrl.timeStamp = ACE_OS::gettimeofday();
	//m_mutexCtrl.acquire();
	memcpy(m_pRMStatusCtrl, &ctrl, sizeof(tagRMStatusCtrl));
	//m_mutexCtrl.release();
	return PK_SUCCESS;	
}

/**
 *  Calc RM Status.
 *    1.Check Status Timestamp
 *    2.if not outdated return status
 *    3.if outdated return status = RM_STATUS_UNAVALIBLE.
 *
 *  @param  -[in,out]  tagRMStatus&  rmStatus: [comment]
 *  @param  -[in,out]  ACE_Time_Value&  now: [comment]
 *  @param  -[in,out]  int&  nStatus: [comment]
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 */
void CShareMemHelper::CheckRMStatus( int nCurRMStatus, timeval timestamp, int &nRealStatus )
{
	ACE_Time_Value now = ACE_OS::gettimeofday();
	ACE_Time_Value stamp(timestamp);
	// Checking TimeStamp.
	if (stamp > now || (now - stamp) > ACE_Time_Value(ICV_RM_STATUS_TIMEOUT))
	{
		nRealStatus = RM_STATUS_UNAVALIBLE;
		return;
	}
	
	// �������״̬ȡֵ
	if (nCurRMStatus == RM_STATUS_ACTIVE ||
		nCurRMStatus == RM_STATUS_INACTIVE)
	{
		nRealStatus = nCurRMStatus;
	}
	else
	{
		nRealStatus = RM_STATUS_UNAVALIBLE;
	}
}

// void CShareMemHelper::ReadRMStatusCtrl( tagRMStatusCtrl &rmStatusCtrl, ACE_Time_Value now, int &nCtrl )
// {
// 	if (rmStatusCtrl.nReadFlag == ICV_RM_STATUS_CTRL_READED)
// 	{
// 		nCtrl = RM_STATUS_CTRL_NIL;
// 		return;
// 	}
// 
// 	ACE_Time_Value stamp(rmStatusCtrl.timeStamp);
// 	// Checking TimeStamp.
// 	if (stamp > now || (now - stamp) < ACE_Time_Value(ICV_RM_STATUS_CTRL_TIMEOUT))
// 	{
// 		nCtrl = RM_STATUS_CTRL_NIL;
// 	}
// 	
// 	if (rmStatusCtrl.nCtrl == RM_STATUS_CTRL_ACTIVE ||
// 		rmStatusCtrl.nCtrl == RM_STATUS_CTRL_INACTIVE)
// 	{
// 		nCtrl = rmStatusCtrl.nCtrl;
// 	}
// 	else
// 	{
// 		nCtrl = RM_STATUS_UNAVALIBLE;
// 	}	
// }

/**
 *  Get Status Ctrl Cmd.
 *	  1. Check timestamp 
 *    2. if outdated nCtrl = CTRL_NIL
 *    3. else set Read flag and return ctrl cmd..
 *
 *  @param  -[in,out]  int&  nCtrl: [comment]
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 */
int CShareMemHelper::GetStatusCtrl( int &nCtrl )
{
	ACE_GUARD_RETURN(ACE_Process_Mutex, guard, m_mutexStatus, (EC_ICV_RMDS_API_FAIL_ACQUIRE_LOCK));

	if (!m_pShm)
	{
		return EC_ICV_RMSERVICE_SHARED_MEM_NOT_INIT;
	}

	tagRMStatusCtrl rmStatusCtrl;

	//m_mutexCtrl.acquire_write();
	//GET CTRL
	memcpy(&rmStatusCtrl, m_pRMStatusCtrl, sizeof(tagRMStatusCtrl));
	
	m_pRMStatusCtrl->nReadFlag = ICV_RM_STATUS_CTRL_READED;

	//m_mutexCtrl.release();

	nCtrl = RM_STATUS_CTRL_NIL;

	if (rmStatusCtrl.nReadFlag == ICV_RM_STATUS_CTRL_UNREAD)
	{
		ACE_Time_Value now = ACE_OS::gettimeofday();
		ACE_Time_Value stamp(rmStatusCtrl.timeStamp);
		// Checking TimeStamp.
		if (stamp > now || (now - stamp) > ACE_Time_Value(ICV_RM_STATUS_CTRL_TIMEOUT))
		{ // TimeStamp is out of date.
			nCtrl = RM_STATUS_CTRL_NIL;
		}
		else
		{
			if (rmStatusCtrl.nCtrl == RM_STATUS_CTRL_ACTIVE ||
				rmStatusCtrl.nCtrl == RM_STATUS_CTRL_INACTIVE)
			{
				nCtrl = rmStatusCtrl.nCtrl;
			}
		}
	}
	return PK_SUCCESS;
}

// �����������������ڴ�
int CShareMemHelper::AddObject(RMObject *pRmObj, int nCurSeq)
{
	if(pRmObj == NULL)
	{
		return EC_ICV_COMM_INVALIDPARA;
	}

	ACE_GUARD_RETURN(ACE_Process_Mutex, guard, m_mutexStatus, (EC_ICV_RMDS_API_FAIL_ACQUIRE_LOCK));

	// Checking if Initialize is finished.
	if (!m_pShm)
	{
		return EC_ICV_RMSERVICE_SHARED_MEM_NOT_INIT;
	}
	// ��ѯ��������Ƿ񼺴���
	tagObjRMStatusBlock * pObjBlock = FindObjBlockByName(pRmObj->szObjName);
	if(pObjBlock != NULL)
	{
		return EC_ICV_RMSERVICE_OBJ_EXISTED;
	}
	// �����ռ�¼������Ӷ���
	pObjBlock =	m_pObjRMStatusBlock;
	int i = 0;
	for( i = 0; i < ICV_RM_MAX_OBJNUM ; ++i, ++pObjBlock )
	{
		//�ҵ��ռ�¼
		if( pObjBlock->szObjName[0] == 0 )
		{
			tagObjRMStatusBlock tmpObjBlock;
			memset(&tmpObjBlock,0, sizeof(tagObjRMStatusBlock));
			PKStringHelper::Safe_StrNCpy(tmpObjBlock.szObjName, pRmObj->szObjName, sizeof(tmpObjBlock.szObjName));
			tmpObjBlock.bNeedSync = pRmObj->bNeedSync;
			tmpObjBlock.bPrim = (nCurSeq == pRmObj->nPrimSeq) ;
			tmpObjBlock.bSingle = pRmObj->bBeSingle;
			if(tmpObjBlock.bPrim == true)
			{
				tmpObjBlock.objRmStatus.nPeerStatus = RM_STATUS_INACTIVE;
				tmpObjBlock.objRmStatus.nStatus = RM_STATUS_ACTIVE;
			}
			else
			{
				tmpObjBlock.objRmStatus.nPeerStatus = RM_STATUS_ACTIVE;
				tmpObjBlock.objRmStatus.nStatus = RM_STATUS_INACTIVE;
			}
			tmpObjBlock.objRmStatus.peerTimeStamp = ACE_OS::gettimeofday();
			tmpObjBlock.objRmStatus.timeStamp = ACE_OS::gettimeofday();
			tmpObjBlock.objRMStatusctrl.nCtrl = RM_STATUS_CTRL_NIL;
			
			//LOCK
			//m_mutexObjBlock.acquire_write();
			//GET CTRL
			memcpy(pObjBlock, &tmpObjBlock, sizeof(tagObjRMStatusBlock));

 			//m_mutexObjBlock.release();

			pRmObj->nObjBlockIdx = i;
			return PK_SUCCESS;
		}
	}

	return EC_ICV_RMSERVICE_OBJ_TBLFULL;

}

// ͨ��������ȡ��������
tagObjRMStatusBlock* CShareMemHelper::GetObjBlockByIdx(int nIndex)
{
	if(nIndex <0 || nIndex >= ICV_RM_MAX_OBJNUM)
		return NULL;

	return m_pObjRMStatusBlock + nIndex;
}

// ͨ������������������
int CShareMemHelper::SetObjBlockByIdx(int nIndex, tagObjRMStatusBlock* pObjBlock)
{
	if(nIndex <0 || nIndex >= ICV_RM_MAX_OBJNUM || pObjBlock == NULL)
		return EC_ICV_COMM_INVALIDPARA;

	ACE_GUARD_RETURN(ACE_Process_Mutex, guard, m_mutexStatus, (EC_ICV_RMDS_API_FAIL_ACQUIRE_LOCK));

	//m_mutexObjBlock.acquire();
	memcpy(m_pObjRMStatusBlock + nIndex, pObjBlock, sizeof(tagObjRMStatusBlock));
	//m_mutexObjBlock.release();

	return PK_SUCCESS;
}

// ���ݶ�����,���Ҷ����ڹ����ڴ��е�λ��
tagObjRMStatusBlock * CShareMemHelper::FindObjBlockByName(const char* pszObjName)
{
	if(pszObjName == NULL)
		return NULL;

	int i = 0;
	for( i = 0; i < ICV_RM_MAX_OBJNUM; ++i)
	{
		//�ҵ�����
		if((NULL != m_pObjRMStatusBlock[i].szObjName)
			&&('\0' != m_pObjRMStatusBlock[i].szObjName[0])
			&&(strcmp(pszObjName, m_pObjRMStatusBlock[i].szObjName) == 0))
			break;
	}

	// �Ҳ�������
	if(i == ICV_RM_MAX_OBJNUM)
	{
		//LH_LOG((PK_LOGLEVEL_WARN, ACE_TEXT("Failed to find RM object!")));
		return NULL;
	}

	return m_pObjRMStatusBlock + i;
}

	// ���ݶ�����������������, �����󲻴��ڣ���ݹ���Ҹ������
tagObjRMStatusBlock *CShareMemHelper::FindObjBlockByNameEx(const char* pszObjName)
{
	tagObjRMStatusBlock *pObjRmBlock = NULL;
	string strParObjName = pszObjName;
	string strObjName = pszObjName;
	int nErr = PK_SUCCESS;
	while(pObjRmBlock == NULL)
	{
		pObjRmBlock = FindObjBlockByName(strObjName.c_str());	
		// �Ҳ�����������DataBlock��
		if(pObjRmBlock == NULL)
		{
			// ��ȡ������
			nErr =  GetParentObjName(strObjName.c_str(), strParObjName);
			if(nErr != PK_SUCCESS)
				break;
			strObjName = strParObjName;
		}
	}

	return pObjRmBlock;
}

// ��ȡ����״̬
int CShareMemHelper::GetObjStatus(int &nStatus, const char* pszObjName)
{
	if(pszObjName == NULL || pszObjName[0] == 0)
	{
		return EC_ICV_COMM_INVALIDPARA;
	}

	tagObjRMStatusBlock *pObjRmBlock = FindObjBlockByName(pszObjName);	

	if(pObjRmBlock != NULL)
	{
		nStatus = pObjRmBlock->objStatus.nStatus;
		return PK_SUCCESS;
	}

	return EC_ICV_RMSERVICE_OBJ_UNEXISTED;
}

// ��ȡ��������
int CShareMemHelper::GetParentObjName(const char *pszObjName, string &strPareObjName)
{
	if(pszObjName == NULL || pszObjName[0] == 0)
	{
		return EC_ICV_COMM_INVALIDPARA;
	}

	strPareObjName = pszObjName;
	int pos = strPareObjName.find_last_of(':');
	if(pos != -1)
	{
		strPareObjName = strPareObjName.substr(0, pos);
		return PK_SUCCESS;
	}
	else
	{
		return EC_ICV_RMSERVICE_OBJ_UNEXISTED;
	}
}

// ��ȡ��������״̬
int CShareMemHelper::GetObjRMStatus(int &nStatus, const char* pszObjName)
{
	if(pszObjName == NULL || pszObjName[0] == 0)
	{
		return EC_ICV_COMM_INVALIDPARA;
	}

	tagObjRMStatusBlock *pObjRmBlock = FindObjBlockByNameEx(pszObjName);

	// �ҵ�����򸸶���ȡ������״̬
	if(pObjRmBlock != NULL)
	{
		CheckRMStatus(pObjRmBlock->objRmStatus.nStatus, pObjRmBlock->objRmStatus.timeStamp,nStatus);
	}
	// �Ҳ���������ȡSCADA�ڵ������״̬
	else
	{
		GetStatus(nStatus);
	}
	
	return PK_SUCCESS;
}

int CShareMemHelper::GetObjRMStatusCtrl(tagRMStatusCtrl *pRMStatusCtrl, int &nCtrl)
{
	ACE_GUARD_RETURN(ACE_Process_Mutex, guard, m_mutexStatus, (EC_ICV_RMDS_API_FAIL_ACQUIRE_LOCK));

	if (!m_pShm)
	{
		return EC_ICV_RMSERVICE_SHARED_MEM_NOT_INIT;
	}

	tagRMStatusCtrl rmStatusCtrl;

	//m_mutexObjBlock.acquire_write();
	//GET CTRL
	memcpy(&rmStatusCtrl, pRMStatusCtrl, sizeof(tagRMStatusCtrl));
	
	pRMStatusCtrl->nReadFlag = ICV_RM_STATUS_CTRL_READED;

	//m_mutexObjBlock.release();

	nCtrl = RM_STATUS_CTRL_NIL;

	if (rmStatusCtrl.nReadFlag == ICV_RM_STATUS_CTRL_UNREAD)
	{
		ACE_Time_Value now = ACE_OS::gettimeofday();
		ACE_Time_Value stamp(rmStatusCtrl.timeStamp);
		// Checking TimeStamp.
		if (stamp > now || (now - stamp) > ACE_Time_Value(ICV_RM_STATUS_CTRL_TIMEOUT))
		{ // TimeStamp is out of date.
			nCtrl = RM_STATUS_CTRL_NIL;
		}
		else
		{
			if (rmStatusCtrl.nCtrl == RM_STATUS_CTRL_ACTIVE ||
				rmStatusCtrl.nCtrl == RM_STATUS_CTRL_INACTIVE)
			{
				nCtrl = rmStatusCtrl.nCtrl;
			}
		}
	}
	return PK_SUCCESS;
}

int CShareMemHelper::GetObjRMInfo(const char *pszObjName, bool *bNeedSync, bool *bPrim, int *bSingle)
{
	if(pszObjName == NULL || pszObjName[0] == 0)
	{
		return EC_ICV_COMM_INVALIDPARA;
	}

	tagObjRMStatusBlock *pObjRmBlock = FindObjBlockByName(pszObjName);	
	if( pObjRmBlock != NULL)
	{
		*bNeedSync = pObjRmBlock->bNeedSync;
		*bPrim = pObjRmBlock->bPrim;
		*bSingle = pObjRmBlock->bSingle;
		return PK_SUCCESS;
	}
	
	return EC_ICV_RMSERVICE_OBJ_UNEXISTED;
}

int CShareMemHelper::SetObjStatus(int nStatus, const char* pszObjName)
{
	if(pszObjName == NULL || pszObjName[0] == 0)
	{
		return EC_ICV_COMM_INVALIDPARA;
	}

	if(nStatus != ICV_RM_OBJ_STATUS_READY && nStatus != ICV_RM_OBJ_STATUS_NOT_READY)
	{
		return EC_ICV_COMM_INVALIDPARA;
	}
	
	ACE_GUARD_RETURN(ACE_Process_Mutex, guard, m_mutexStatus, (EC_ICV_RMDS_API_FAIL_ACQUIRE_LOCK));

	// Checking if Initialize is finished.
	if (!m_pShm)
	{
		return EC_ICV_RMSERVICE_SHARED_MEM_NOT_INIT;
	}
	// Copy Memory To Local.
	tagObjStatus objstatus;
	objstatus.nStatus = nStatus;
	objstatus.timeStamp = ACE_OS::gettimeofday();
	
	tagObjRMStatusBlock *pObjRmBlock = FindObjBlockByName(pszObjName);	
	if( pObjRmBlock != NULL)
	{
		//LOCK
		//m_mutexObjBlock.acquire_write();
		//GET CTRL
		memcpy(pObjRmBlock, &objstatus, sizeof(tagObjStatus));

 		//m_mutexObjBlock.release();

		return PK_SUCCESS;
	}

	return EC_ICV_RMSERVICE_OBJ_UNEXISTED;
}


// Set object rm status
int CShareMemHelper::SetObjRMStatus(int nStatus, const char* pszObjName)
{
	if(pszObjName == NULL || pszObjName[0] == 0)
	{
		return EC_ICV_COMM_INVALIDPARA;
	}
	// ����״ֵ̬�Ƿ�
	if(nStatus != RM_STATUS_INACTIVE && nStatus != RM_STATUS_ACTIVE && nStatus != RM_STATUS_UNAVALIBLE)
	{
		return EC_ICV_COMM_INVALIDPARA;
	}
	
	ACE_GUARD_RETURN(ACE_Process_Mutex, guard, m_mutexStatus, (EC_ICV_RMDS_API_FAIL_ACQUIRE_LOCK));

	// Checking if Initialize is finished.
	if (!m_pShm)
	{
		return EC_ICV_RMSERVICE_SHARED_MEM_NOT_INIT;
	}

	tagObjRMStatusBlock *pObjRmBlock = FindObjBlockByName(pszObjName);	
	if( pObjRmBlock != NULL)
	{
		// Copy Memory To Local.
		tagObjRMStatus objRMstatus;
		objRMstatus.nStatus = nStatus;
		objRMstatus.timeStamp = ACE_OS::gettimeofday();
		objRMstatus.nPeerStatus = pObjRmBlock->objRmStatus.nPeerStatus;
		objRMstatus.peerTimeStamp = pObjRmBlock->objRmStatus.peerTimeStamp;

		//LOCK
		//m_mutexObjBlock.acquire_write();
		//GET CTRL
		memcpy((char*)pObjRmBlock+sizeof(tagObjStatus), &objRMstatus, sizeof(tagObjRMStatus));

 		//m_mutexObjBlock.release();

		return PK_SUCCESS;
	}

	return EC_ICV_RMSERVICE_OBJ_UNEXISTED;
}


int CShareMemHelper::SetObjRMStatusCtrl(int nCtrl, const char* pszObjName)
{
	if(pszObjName == NULL || pszObjName[0] == 0)
	{
		return EC_ICV_COMM_INVALIDPARA;
	}

	// ��������ֵ�Ƿ�
	if(nCtrl != RM_STATUS_CTRL_ACK && nCtrl != RM_STATUS_CTRL_NIL && 
		nCtrl != RM_STATUS_CTRL_ACTIVE && nCtrl != RM_STATUS_CTRL_INACTIVE)
	{
		return EC_ICV_COMM_INVALIDPARA;
	}

	ACE_GUARD_RETURN(ACE_Process_Mutex, guard, m_mutexStatus, (EC_ICV_RMDS_API_FAIL_ACQUIRE_LOCK));

	// Checking if Initialize is finished.
	if (!m_pShm)
	{
		return EC_ICV_RMSERVICE_SHARED_MEM_NOT_INIT;
	}
	
	tagObjRMStatusBlock *pObjRmBlock = FindObjBlockByName(pszObjName);	

	if( pObjRmBlock != NULL)
	{
		if(pObjRmBlock->objRmStatus.nStatus == RM_STATUS_UNAVALIBLE)
		{
			return EC_ICV_RMSERVICE_SERVICE_UNAVALIBLE;
		};	

		// Copy Memory To Local.
		tagRMStatusCtrl objRMStatusCtrl;
		objRMStatusCtrl.nCtrl = nCtrl;
		objRMStatusCtrl.nReadFlag = ICV_RM_STATUS_CTRL_UNREAD;
		objRMStatusCtrl.timeStamp = ACE_OS::gettimeofday(); 

		//LOCK
		//m_mutexObjBlock.acquire_write();
		//GET CTRL
		memcpy((char*)pObjRmBlock+sizeof(tagObjStatus)+sizeof(tagObjRMStatus), &objRMStatusCtrl, sizeof(tagRMStatusCtrl));

 		//m_mutexObjBlock.release();

		return PK_SUCCESS;
	}

	return EC_ICV_RMSERVICE_OBJ_UNEXISTED;
}

int CShareMemHelper::NeedSync(const char *pszObjName, bool &bNeedSync)
{
	if(pszObjName == NULL || pszObjName[0] == 0)
	{
		return EC_ICV_COMM_INVALIDPARA;
	}
	bool bNeedDoSvc = false;
	NeedDoSvc(pszObjName, bNeedDoSvc);
	if(bNeedDoSvc == false)
	{
		bNeedSync = false;
		return PK_SUCCESS;
	}

	tagObjRMStatusBlock *pObjRmBlock = FindObjBlockByNameEx(pszObjName);	

	// �ҵ�������Ӷ���
	if( pObjRmBlock != NULL)
	{
		// �������������䲻��Ҫͬ���� ����ͬ��
		if(pObjRmBlock->bNeedSync == false)
		{
			bNeedSync = false;
		}
		// ��ڵ������ҶԵȶ����� ����ͬ��
		else if((pObjRmBlock->bSingle == false) && (pObjRmBlock->objRmStatus.nPeerStatus == RM_STATUS_ACTIVE))
		{
			bNeedSync = false;
		}else
		{
			bNeedSync = true;
		}
	}
	else
	{
		int nTmpStatus = RM_STATUS_UNAVALIBLE;
		GetStatus(nTmpStatus);
		// SCADA�ڵ�����״̬Ϊ�����ͬ��
		if(nTmpStatus == RM_STATUS_ACTIVE)
			bNeedSync = true;
		else
			bNeedSync = false;
	}

	return PK_SUCCESS;
}

int CShareMemHelper::NeedDoSvc(const char *pszObjName, bool &bNeedDoSvc)
{
	if(pszObjName == NULL || pszObjName[0] == 0)
	{
		return EC_ICV_COMM_INVALIDPARA;
	}

	int nErr = PK_SUCCESS;
	tagObjRMStatusBlock *pObjRmBlock = FindObjBlockByNameEx(pszObjName);	

	int nTmpStatus = RM_STATUS_UNAVALIBLE;
	//�ҵ�����򸸶���
	if( pObjRmBlock != NULL)
	{
		CheckRMStatus(pObjRmBlock->objRmStatus.nStatus, pObjRmBlock->objRmStatus.timeStamp,nTmpStatus);
		// ���ڵ����У� �Ҷ�������״̬ΪINACTIVE
		if((pObjRmBlock->bSingle == true) && (nTmpStatus == RM_STATUS_INACTIVE))
		{
			bNeedDoSvc = false;
		}else
		{
			bNeedDoSvc = true;
		}
	}
	else
	{
		// ȡSCADA�ڵ�����״̬
		GetStatus(nTmpStatus);
		// ����״̬Ϊ�������ִ�з���
		if( nTmpStatus == RM_STATUS_INACTIVE)
		{
			bNeedDoSvc = false;
		}
		else
		{
			bNeedDoSvc = true;
		}
	}
	
	return PK_SUCCESS;
} 
