#ifndef _SHARE_MEM_HELPER_H_
#define _SHARE_MEM_HELPER_H_

#include <ace/Shared_Memory_MM.h>
#include <ace/RW_Process_Mutex.h>
#include <ace/Process_Mutex.h>
#include <ace/Time_Value.h>
#include "common/CommHelper.h"
#include "common/eviewdefine.h"
#include <string>
using namespace std;

#define ICV_RM_MAX_OBJNUM			5000  
#define ICV_RMSVC_SEQUENCE_MAXLEN	32     // sequence len for scada node

#define ICV_RM_STATUS_TIMEOUT		3  //3 sec.
#define ICV_RM_STATUS_CTRL_TIMEOUT 10	//10 sec.

#define ICV_RM_STATUS_CTRL_READED  0x000EADED   // Flags for Readed Ctrl.
#define ICV_RM_STATUS_CTRL_UNREAD  0x000DEDAE   // Flags for UnRead Ctrl.

#define DEFAULT_SHM_FILE_NAME ACE_TEXT("_rm_svc_rtdt.dat")

typedef struct RMStatus
{
	int			nStatus;
	timeval		timeStamp;
}tagRMStatus;

typedef struct RMStatusCtrl
{
	int			nReadFlag;
	int			nCtrl;
	timeval		timeStamp;
}tagRMStatusCtrl;

typedef struct RMStatusBlock 
{
	tagRMStatus status;
	tagRMStatusCtrl ctrl;
}tagRMStatusBlock;

typedef struct ObjStatus
{
	int		nStatus;
	timeval		timeStamp;
}tagObjStatus;

typedef struct ObjRMStatus
{
	// ���ڵ��������״̬
	int		nStatus;
	timeval		timeStamp;
	// �ԵȽڵ�Եȶ�������״̬
	int		nPeerStatus;
	timeval		peerTimeStamp;
}tagObjRMStatus;

typedef struct ObjRMStatusBlock 
{
	tagObjStatus objStatus;			// ����״̬
	tagObjRMStatus objRmStatus;		// ��������״̬
	tagRMStatusCtrl objRMStatusctrl;		//���������������
	char szObjName[PK_SHORTFILENAME_MAXLEN];	//������
	bool bSingle;		// �����Ƿ񵥽ڵ�����
	bool bNeedSync;		// �����Ƿ���ִ������ͬ��
	bool bPrim;			// ���ڵ��Ƿ�Ϊ����������нڵ�
}tagObjRMStatusBlock;

typedef struct RMObject
{
	char szObjName[PK_SHORTFILENAME_MAXLEN];		//������
	int nPrimSeq;									//�������ڵ����
	bool bBeSingle;									//��������ģʽ�Ƿ�Ϊ���ڵ�
	bool bNeedSync;									//�����Ƿ���ִ������ͬ��
	int nCtrlExec;									//��ǰ����ִ�е������������
	int nCtrlCount;								//��ǰ������������ִ�еĴ���
    int nStatusCount;								//���Ի�ȡ�ԵȽڵ�����״̬�Ĵ���
	int nSend;										//�����͸��Ե�����ڵ�������������
	int nObjBlockIdx;								// ��¼�����ڴ�����Ӧ������������

	RMObject()
	{
		memset(this, 0, sizeof(RMObject));
	}
	bool operator==(const RMObject& obj) const
	{
		if (0 != strcmp(szObjName, obj.szObjName))
		{
			return false;
		}
		if (nPrimSeq != obj.nPrimSeq)
		{
			return false;
		}
		if (bBeSingle != obj.bBeSingle)
		{
			return false;
		}
		if (bNeedSync != obj.bNeedSync)
		{
			return false;
		}
		if (nCtrlExec != obj.nCtrlExec)
		{
			return false;
		}
		if (nCtrlCount != obj.nCtrlCount)
		{
			return false;
		}
		if (nStatusCount != obj.nStatusCount)
		{
			return false;
		}
		if (nSend != obj.nSend)
		{
			return false;
		}
		if (nObjBlockIdx != obj.nObjBlockIdx)
		{
			return false;
		}
		return true;
	}
}RMObject;

class CShareMemHelper
{

UNITTEST(CShareMemHelper);

private:
	// ����������״̬������ʱδ���£��򷵻��޷���ȡ����״̬
	void CheckRMStatus( int nCurRMStatus, timeval timestamp, int &nRealStatus );

	//void ReadRMStatusCtrl( tagRMStatusCtrl &rmStatusCtrl, ACE_Time_Value now, int &nStatus );
	// ��ȡ��������
	int GetParentObjName(const char *pszObjName, string &strPareObjName);

public:
	CShareMemHelper();
	virtual ~CShareMemHelper();

	// Initialize
	int InitializeMM(const string &strName, bool bInit = true);
	// ��չ����ڴ��е��������顣
	void ResetMM();

	// ��ȡSCADA�ڵ������״̬
	int GetStatus(int &nStatus);

	// ��ȡSCADA�ڵ�������������
	int GetStatusCtrl(int &nCtrl);

	// ����SCADA�ڵ������״̬��
	int SetStatus(int nStatus);

	// �л�SCADA�ڵ������״̬
	int SetStatusCtrl(int nCtrl);

	// �����������������ڴ�
	int AddObject(RMObject *pRmObj, int nCurSeq);

	// Get object status from share memory
	int GetObjStatus(int &nStatus, const char* pszObjName);

	// Get object rm status
	int GetObjRMStatus(int &nStatus, const char* pszObjName);

	int GetObjRMStatusCtrl(tagRMStatusCtrl *pRMStatusCtrl, int &nCtrl);

	// Set object status from share memory
	int SetObjStatus(int nStatus, const char* pszObjName);

	int SetObjRMStatus(int nStatus, const char* pszObjName);

	// Set object RMStatus ctrl
	int SetObjRMStatusCtrl(int Ctrl, const char* pszObjName);

	// Get object Rm info
	int GetObjRMInfo(const char *pszObjName, bool *bNeedSync, bool *bPrim, int *bSingle);
	
	// ��������Ƿ����������ͬ��
	int NeedSync(const char *pszObjName, bool &bNeedSync);

	// ��������Ƿ���ִ�з���
	int NeedDoSvc(const char *pszObjName, bool &bNeedDoSvc);

	// ͨ��������ȡ��������
	tagObjRMStatusBlock *GetObjBlockByIdx(int index);
	
	// ���ݶ�����������������
	tagObjRMStatusBlock *FindObjBlockByName(const char* pszObjName);	

	// ���ݶ�����������������, �����󲻴��ڣ���ݹ���Ҹ������
	tagObjRMStatusBlock *FindObjBlockByNameEx(const char* pszObjName);	

	// ͨ������������������
	int SetObjBlockByIdx(int index, tagObjRMStatusBlock* pObjBlock);

private:
	ACE_Shared_Memory_MM*	m_pShm;				// Share Mem Pointer
	//ACE_RW_Process_Mutex	m_mutexStatus;		// RW Mutex For Status Fetch and Modify
	ACE_Process_Mutex		m_mutexStatus;		// Mutex For Status
	//ACE_Process_Mutex		m_mutexCtrl;		// Mutex For Ctrl Commond
	//ACE_Process_Mutex		m_mutexObjBlock;	// Mutex for Obj block

	tagRMStatus*		m_pRMStatus;		// Pointer to Status Block
	tagRMStatusCtrl*	m_pRMStatusCtrl;		// Pointer to Ctrl Block
	tagObjRMStatusBlock* m_pObjRMStatusBlock;	// Pointer to ObjRMStatusBlock
};

#endif//#ifndef _SHARE_MEM_HELPER_H_
