/**************************************************************
 *  Filename:    RMStatusCtrl.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: Controller Thread.
 *
 *  @author:     chenzhiquan
 *  @version     05/20/2008  chenzhiquan  Initial Version
**************************************************************/
#ifndef _RM_STATUS_CTRL_H_
#define _RM_STATUS_CTRL_H_

#include <ace/Task.h>
#include "RMPeerCommunicator.h"
#include "errcode/ErrCode_iCV_RM.hxx"
#include "ShareMemHelper.h"
#include "common/CommHelper.h"
#include "common/eviewdefine.h"
#include "common/RMAPIDef.h"

#define ICV_RMSVC_OBJ_SINGLE_RUNMODE 1
#define ICV_RMSVC_OBJ_MULTI_RUNMODE  2

class CRMStatusCtrl:public ACE_Task<ACE_NULL_SYNCH>
{
UNITTEST(CRMStatusCtrl);

public:
	CRMStatusCtrl();
	virtual ~CRMStatusCtrl();
	
	// Start Thread
	void StartThread();
	
	// Stop Thread
	void StopThread();
	
	// Thread Func
	virtual int svc();
	
	// Initialization
	long Initialize(const string &strHome);
	
	// Get Status of Current node.
	long GetStatus(){ return m_nStatus; };
	//void SwitchStatus(long nStatus);
	
	// Setting Peer Communicator
	void SetPeerCommunicator(CRMPeerCommunicator* pComm);

	// Setting Strategy
	void SetStrategy(long nIntervalInMs, long nThreshold);

	// Setting Sequence
	void SetSequence(long nSequence)
	{
		m_nSequence = nSequence;
	}

	long GetSequence()
	{
		return m_nSequence;
	}

	long AddRMObject(RMObject *pRMObject);
	void SetRMObject(RMObject *pRMObject);

	bool operator!=(const CRMStatusCtrl& obj);

	

private:
	void ProcOneLocalCmd(long nCtrlCmd, long &nCurRMStatus, long &nCtrlExecing, long &nCtrlSend);
	void DoLocalCmds(long &nCtrlSend);
	void ProcOneRemoteCmd(long nCtrlRecv, long &nCurRMStatus, long &nCtrlExecing, long &nCtrlSend, long &nCtrlCount);
	void DoRemoteCmds(long &nCtrlSend, long &nCmdCount, long &nStatusCount);
	void DoRemoteNilCmdMsg(long nRemoteRmStatus, bool bSingle, long primSeq,long &nCtrlExec, long &nCurRmStatus, long &nCtrlSend);
	void SendCmdAndStatus(long nCtrlSend);

protected:
	void DoCtrl();

public:
	long m_nStatus;			// RM Status
	long m_nThreshold;		// Threshold for status count
	
	long m_nStatusCount;	// Count of interval did not recv status info from peer
	long m_nCtrlCount;		// Count of interval Control cmd Send before recv ack.

	ACE_Time_Value m_tvInterval;	// Scan Interval in msec.
	long m_nSequence;		// 0:for main 1:for standby
	long m_nCtrlExecing;			// Ctrl Command
	CRMPeerCommunicator* m_pComm; // Peer Communicator Obj Pointer
	
	CShareMemHelper m_shmHelper;	// Share Memory R/W Helper
	std::list<RMObject> m_RMObjectList;  // RM Object

	bool m_bExit;	//Exit Flag.
};

#endif//_RM_STATUS_CTRL_H_
