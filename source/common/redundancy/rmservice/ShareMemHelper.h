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
	// 本节点对象冗余状态
	int		nStatus;
	timeval		timeStamp;
	// 对等节点对等对象冗余状态
	int		nPeerStatus;
	timeval		peerTimeStamp;
}tagObjRMStatus;

typedef struct ObjRMStatusBlock 
{
	tagObjStatus objStatus;			// 对象状态
	tagObjRMStatus objRmStatus;		// 对象冗余状态
	tagRMStatusCtrl objRMStatusctrl;		//对象冗余控制命令
	char szObjName[PK_SHORTFILENAME_MAXLEN];	//对象名
	bool bSingle;		// 对象是否单节点运行
	bool bNeedSync;		// 对象是否需执行数据同步
	bool bPrim;			// 本节点是否为对象的主运行节点
}tagObjRMStatusBlock;

typedef struct RMObject
{
	char szObjName[PK_SHORTFILENAME_MAXLEN];		//对象名
	int nPrimSeq;									//对象主节点序号
	bool bBeSingle;									//对象运行模式是否为单节点
	bool bNeedSync;									//对象是否需执行数据同步
	int nCtrlExec;									//当前正在执行的冗余控制命令
	int nCtrlCount;								//当前冗余控制命令尝试执行的次数
    int nStatusCount;								//尝试获取对等节点冗余状态的次数
	int nSend;										//待发送给对等冗余节点的冗余控制命令
	int nObjBlockIdx;								// 记录共享内存对象对应的冗余块的索引

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
	// 检查对象冗余状态，若超时未更新，则返回无法获取冗余状态
	void CheckRMStatus( int nCurRMStatus, timeval timestamp, int &nRealStatus );

	//void ReadRMStatusCtrl( tagRMStatusCtrl &rmStatusCtrl, ACE_Time_Value now, int &nStatus );
	// 获取父对象名
	int GetParentObjName(const char *pszObjName, string &strPareObjName);

public:
	CShareMemHelper();
	virtual ~CShareMemHelper();

	// Initialize
	int InitializeMM(const string &strName, bool bInit = true);
	// 清空共享内存中的冗余对象块。
	void ResetMM();

	// 获取SCADA节点的冗余状态
	int GetStatus(int &nStatus);

	// 获取SCADA节点的冗余控制命令
	int GetStatusCtrl(int &nCtrl);

	// 设置SCADA节点的冗余状态。
	int SetStatus(int nStatus);

	// 切换SCADA节点的冗余状态
	int SetStatusCtrl(int nCtrl);

	// 添加冗余对象至共享内存
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
	
	// 冗余对象是否需进行数据同步
	int NeedSync(const char *pszObjName, bool &bNeedSync);

	// 冗余对象是否需执行服务
	int NeedDoSvc(const char *pszObjName, bool &bNeedDoSvc);

	// 通过索引获取冗余对象块
	tagObjRMStatusBlock *GetObjBlockByIdx(int index);
	
	// 根据对象名查找冗余对象块
	tagObjRMStatusBlock *FindObjBlockByName(const char* pszObjName);	

	// 根据对象名查找冗余对象块, 若对象不存在，则递归查找父对象块
	tagObjRMStatusBlock *FindObjBlockByNameEx(const char* pszObjName);	

	// 通过索引设置冗余对象块
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
