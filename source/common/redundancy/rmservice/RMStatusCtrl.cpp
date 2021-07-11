/**************************************************************
 *  Filename:    RMStatusCtrl.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: Impl. Of CRMStatusCtrl.
 *
 *  @author:     chenzhiquan
 *  @version     05/20/2008  chenzhiquan  Initial Version
 *  @version     07/16/2008  chenzhiquan  Print Log Info For Debug and analysis.
**************************************************************/
#include "RMStatusCtrl.h"
#include "ShareMemHelper.h"
#include <ace/OS_NS_unistd.h>

#include "errcode/ErrCode_iCV_RM.hxx"

ACE_UINT64 get_msec (timeval val)
{
  // ACE_OS_TRACE ("ACE_Time_Value::get_msec");
  ACE_UINT64 ms = ACE_Utils::truncate_cast<ACE_UINT64> (val.tv_sec);
  ms *= 1000;
  ms += (val.tv_usec / 1000);
  return ms;
}

/**
 *  Thread Func.
 *
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 */
int CRMStatusCtrl::svc()
{
	// Thread 
	while (!m_bExit)
	{
		if(m_pComm != NULL)
		{
			// Do Control Stuffs
			DoCtrl();
		}
		// Sleep 
		ACE_OS::sleep(m_tvInterval);
	}
	return 0;	
}

/**
*  Start Thread.
*
*
*  @version     05/20/2008  chenzhiquan  Initial Version.
*/
void CRMStatusCtrl::StartThread()
{
	m_bExit = false;
	this->activate();	
}

/**
*  Stop Thread.
*
*
*  @version     05/20/2008  chenzhiquan  Initial Version.
*/
void CRMStatusCtrl::StopThread()
{
	m_bExit = true;
	this->wait();
}

/**
*  Set Peer Communicator.
*
*  @param  -[in,out]  CRMPeerCommunicator*  pComm: [comment]
*
*  @version     05/20/2008  chenzhiquan  Initial Version.
*/
void CRMStatusCtrl::SetPeerCommunicator( CRMPeerCommunicator* pComm )
{
	m_pComm = pComm;	
}

/**
 *  Constructor.
 *
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 */
CRMStatusCtrl::CRMStatusCtrl() : m_nStatus(RM_STATUS_INACTIVE), m_nThreshold(0), m_nStatusCount(0), m_nCtrlCount(0),
								 m_nSequence(1), m_nCtrlExecing(RM_STATUS_CTRL_NIL), m_pComm(0), m_bExit(true)
{
	
}

/**
 *  Set Strategy Params..
 *
 *  @param  -[in]  long  nIntervalInMs: [Interval in msec]
 *  @param  -[in]  long  nThreshold: [ Threshold]
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 */
void CRMStatusCtrl::SetStrategy( long nIntervalInMs, long nThreshold )
{
	m_nThreshold = nThreshold;
	m_tvInterval.sec(0);
	m_tvInterval.usec(nIntervalInMs * 1000);
}

long CRMStatusCtrl::AddRMObject(RMObject *pRMObject)
{
	// ���ͬ�������Ƿ񼺴���
	std::list<RMObject>::iterator iter = m_RMObjectList.begin();
	for ( ; iter != m_RMObjectList.end(); iter++)
	{
		if (strcmp((*iter).szObjName, pRMObject->szObjName) == 0)
		{
			return EC_ICV_RMSERVICE_OBJ_EXISTED;
		}
	}

	m_RMObjectList.push_back(*pRMObject);

	return ICV_SUCCESS;
}

void CRMStatusCtrl::SetRMObject(RMObject *pRMObject)
{
	std::list<RMObject>::iterator iter = m_RMObjectList.begin();
	for ( ; iter != m_RMObjectList.end(); iter++)
	{
		if (strcmp((*iter).szObjName, pRMObject->szObjName) == 0)
		{
			(*iter).bBeSingle = pRMObject->bBeSingle;
			(*iter).bNeedSync = pRMObject->bNeedSync;
			(*iter).nCtrlCount = pRMObject->nCtrlCount;
			(*iter).nPrimSeq = pRMObject->nPrimSeq;
			(*iter).nCtrlExec = pRMObject->nCtrlExec;
			(*iter).nSend = pRMObject->nSend;
			(*iter).nStatusCount = pRMObject->nStatusCount;
		}
	}
}

/**
 *  Initialize.
 *
 *  @param  -[in,out]  const string&  strHome: [comment]
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 */
long CRMStatusCtrl::Initialize( const string &strHome )
{
	// ��ʼ��communicator
	long nErr = m_pComm->Initialize();
	if (nErr != ICV_SUCCESS)
	{
		return nErr;
	}

	// ��ʼ�������ڴ�
	nErr = m_shmHelper.InitializeMM(strHome + ACE_DIRECTORY_SEPARATOR_CHAR + DEFAULT_SHM_FILE_NAME);
	if(nErr != ICV_SUCCESS)
	{
		return nErr;
	}

	m_shmHelper.ResetMM();

	// �������ļ��м��ص������������������ڴ�����������
	if(m_RMObjectList.size() > 0)
	{
		std::list<RMObject>::iterator objIter = m_RMObjectList.begin();
		for(; objIter != m_RMObjectList.end(); ++objIter)
		{
			RMObject* pTmpRmObj = &(*objIter);
			nErr = m_shmHelper.AddObject(pTmpRmObj, m_nSequence);

			if(nErr != ICV_SUCCESS)
			{
				return nErr;
			}
		}
	}

	return ICV_SUCCESS;
}

void CRMStatusCtrl::ProcOneLocalCmd(long nCtrlCmd, long &nCurRMStatus, long &nCtrlExecing, long &nCtrlSend)
{
	if (nCtrlCmd == RM_STATUS_CTRL_ACTIVE)
	{
		LH_LOG((LOG_LEVEL_INFO, ACE_TEXT("Recieve RM_STATUS_CTRL_ACTIVE Ctrl!")));
		if (nCurRMStatus != RM_STATUS_ACTIVE)
		{// When Switching To Active, we do not turn inactive to active right now
			// But to wait the peer node to be inactive and then turn to active.
			nCtrlExecing = RM_STATUS_CTRL_ACTIVE;
			nCtrlSend = RM_STATUS_CTRL_INACTIVE;
		}
	}
	else if (nCtrlCmd == RM_STATUS_CTRL_INACTIVE)
	{//RM_STATUS_CTRL_INACTIVE
		LH_LOG((LOG_LEVEL_INFO, ACE_TEXT("RMStatus Recieve RM_STATUS_CTRL_INACTIVE Ctrl!")));
		if (nCurRMStatus != RM_STATUS_INACTIVE)
		{// When Switching To InActive, we turn active to inactive right now
			// Then to wait the peer node to be active.
			nCurRMStatus = RM_STATUS_INACTIVE; // Trun InActive
			nCtrlExecing = RM_STATUS_CTRL_INACTIVE;
			nCtrlSend = RM_STATUS_CTRL_ACTIVE;
			LH_LOG((LOG_LEVEL_INFO, ACE_TEXT("RMStatus Change From Active To InActive!")));
		}			
	}
}

// �����������������, ������SCADA��������������״̬
void CRMStatusCtrl::DoLocalCmds(long &nCtrlSend)
{
	long nCtrlCmd = RM_STATUS_CTRL_NIL;
	
	// ����scada�ڵ������������
	m_shmHelper.GetStatusCtrl(nCtrlCmd);
	if (nCtrlCmd != RM_STATUS_CTRL_NIL)
	{// Received A Ctrl Command.
		ProcOneLocalCmd(nCtrlCmd, m_nStatus, m_nCtrlExecing, nCtrlSend);
		m_nCtrlCount = 0;
	}

	// �����������������������״̬
	std::list<RMObject>::iterator objIter = m_RMObjectList.begin();
	for(;objIter != m_RMObjectList.end(); ++objIter)
	{
		tagObjRMStatusBlock *pObjStatusBlk = m_shmHelper.GetObjBlockByIdx((*objIter).nObjBlockIdx);
		tagObjRMStatusBlock tmpObjStatusBlk;
		memcpy(&tmpObjStatusBlk, pObjStatusBlk,sizeof(tagObjRMStatusBlock));
		long nCtrl = RM_STATUS_CTRL_NIL;
		m_shmHelper.GetObjRMStatusCtrl(&(tmpObjStatusBlk.objRMStatusctrl), nCtrl);
		// ������������������
		if(nCtrl != RM_STATUS_CTRL_NIL)
		{
			ProcOneLocalCmd(nCtrl, tmpObjStatusBlk.objRmStatus.nStatus, 
				(*objIter).nCtrlExec, (*objIter).nSend);	
			(*objIter).nCtrlCount = 0;
		}

		if( tmpObjStatusBlk.objStatus.nStatus == ICV_RM_OBJ_STATUS_READY)
		{
			(*objIter).nStatusCount = 0;
		}

		timeval curTime = ACE_OS::gettimeofday();
		tmpObjStatusBlk.objRmStatus.timeStamp = curTime;
		// ����������״̬,��״̬��ΪReady�򳬹�ʱ��δ���£�����Ϊ�����쳣, �л�Ϊ���
		if( pObjStatusBlk->objRmStatus.nStatus == RM_STATUS_ACTIVE)
		{
			// �������״̬�쳣
			if(tmpObjStatusBlk.objStatus.nStatus != ICV_RM_OBJ_STATUS_READY)
			{
                ++((*objIter).nStatusCount);
                // ����״̬�쳣���ޣ��л�
                if((*objIter).nStatusCount > m_nThreshold)
                {
    				ProcOneLocalCmd(RM_STATUS_CTRL_INACTIVE, tmpObjStatusBlk.objRmStatus.nStatus, 
    					(*objIter).nCtrlExec, (*objIter).nSend);
    				(*objIter).nCtrlCount = 0;                    
                }
            }
            
            // ����������������ʱ���л�����״̬
			if(get_msec(curTime) - get_msec(tmpObjStatusBlk.objStatus.timeStamp) > get_msec(m_tvInterval)*m_nThreshold)
			{
				ProcOneLocalCmd(RM_STATUS_CTRL_INACTIVE, tmpObjStatusBlk.objRmStatus.nStatus, 
					(*objIter).nCtrlExec, (*objIter).nSend);
				(*objIter).nCtrlCount = 0;
			}
		}
		// ������Ϊ�������ģʽ�������״̬ΪReadyʱ���л�Ϊ���
		else if( pObjStatusBlk->objRmStatus.nStatus == RM_STATUS_INACTIVE && pObjStatusBlk->bSingle == false)
		{
			if( tmpObjStatusBlk.objStatus.nStatus == ICV_RM_OBJ_STATUS_READY &&
				get_msec(curTime) - get_msec(tmpObjStatusBlk.objStatus.timeStamp) < get_msec(m_tvInterval)*m_nThreshold)
			{
				tmpObjStatusBlk.objRmStatus.nStatus = RM_STATUS_CTRL_ACTIVE;			
			}
		}


		// �Դ��������������ʱ�� �л�����״̬
		if(get_msec(curTime) - get_msec(tmpObjStatusBlk.objRmStatus.peerTimeStamp) > get_msec(m_tvInterval)*m_nThreshold)
		{
			tmpObjStatusBlk.objRmStatus.nStatus = RM_STATUS_ACTIVE;
			tmpObjStatusBlk.objRmStatus.nPeerStatus = RM_STATUS_INACTIVE;
			(*objIter).nCtrlExec = RM_STATUS_CTRL_ACTIVE;
			(*objIter).nSend = RM_STATUS_CTRL_INACTIVE;
		}

		m_shmHelper.SetObjBlockByIdx((*objIter).nObjBlockIdx, &tmpObjStatusBlk);
	}
}

void CRMStatusCtrl::ProcOneRemoteCmd(long nCtrlRecv, long &nCurRMStatus, long &nCtrlExecing, long &nCtrlSend, long &nCtrlCount)
{
	if (nCtrlRecv == RM_STATUS_CTRL_ACK)
	{// When Receive ctrl Ack msg.
		LH_LOG((LOG_LEVEL_INFO, ACE_TEXT("Recieve RM_STATUS_CTRL_ACK Ctrl From Peer Node!")));
		if (nCtrlExecing == RM_STATUS_CTRL_ACTIVE)
		{// Turn to Active after received ack msg.
			nCurRMStatus = RM_STATUS_ACTIVE;
			LH_LOG((LOG_LEVEL_INFO, ACE_TEXT("RMStatus Change From InActive To Active!")));
		}
		nCtrlExecing = RM_STATUS_CTRL_NIL;
		nCtrlSend = RM_STATUS_CTRL_NIL;
		nCtrlCount = 0;
	}
	else if( RM_STATUS_CTRL_ACTIVE == nCtrlRecv || RM_STATUS_CTRL_INACTIVE == nCtrlRecv )// RM_STATUS_CTRL_ACTIVE or RM_STATUS_CTRL_INACTIVE
	{
		LH_LOG((LOG_LEVEL_INFO, ACE_TEXT("Recieve Ctrl Cmd From Peer Node! Ctrl Cmd = %d, Old Status = %d"), nCtrlRecv, nCurRMStatus));
		long nStatusOld = nCurRMStatus;
		nCurRMStatus = nCtrlRecv;
		if (nStatusOld != nCurRMStatus)
		{
			LH_LOG((LOG_LEVEL_INFO, ACE_TEXT("RMStatus Change From %d To %d!"), nStatusOld, nCurRMStatus));
		}
		nCtrlSend = RM_STATUS_CTRL_ACK;
	}
}

void  CRMStatusCtrl::DoRemoteNilCmdMsg(long nRemoteRmStatus, bool bSingle, long primSeq, long &nCtrlExec, long &nCurRmStatus, long &nCtrlSend)
{
	if (nCtrlExec == RM_STATUS_CTRL_NIL)
	{ // nil command 
		if(bSingle == true)
		{
			if(nRemoteRmStatus == nCurRmStatus)
			{ //Both Of Two Status are the SAME!
				LH_LOG((LOG_LEVEL_INFO, ACE_TEXT("Detect RMStatus Confliction: both of tow status is %d!"), nCurRmStatus));
				if (m_nSequence == primSeq) // 0 for Main
				{
					if (nCurRmStatus != RM_STATUS_ACTIVE)
					{
						LH_LOG((LOG_LEVEL_INFO, ACE_TEXT("RMStatus Change To Active!")));
					}
					nCurRmStatus = RM_STATUS_ACTIVE;
				}
				else
				{
					if (nCurRmStatus != RM_STATUS_INACTIVE)
					{
						LH_LOG((LOG_LEVEL_INFO, ACE_TEXT("RMStatus Change To InActive!")));
					}
					nCurRmStatus = RM_STATUS_INACTIVE;
				}
			}
		}
	}
	else
	{// Ctrl Cmd Should Be Resend.
		// Calc Send Cmd.
		nCtrlSend = (nCtrlExec == RM_STATUS_ACTIVE) ? RM_STATUS_CTRL_INACTIVE : RM_STATUS_CTRL_ACTIVE;
				
	}

}

// ����Զ�������������, ����SCADA��������������״̬
void CRMStatusCtrl::DoRemoteCmds(long &nCtrlSend, long &nCmdCount, long &nMsgCount)
{
	long nStatus = 0, nCtrlRecv = 0;
	int nType = 0;
	nCmdCount = 0;
	nMsgCount = 0;
	std::string strObjName;

	while (m_pComm->GetStatus(nType,nStatus, nCtrlRecv, strObjName) == ICV_SUCCESS)
	{	// Communication OK!
		if(nType == ICV_RM_SCADA_MSGTYPE)
		{
			LH_LOG((LOG_LEVEL_DEBUG, ACE_TEXT("Receive SCADA Heart Beat Success!")));
			m_nStatusCount = 0;
			// Զ�������������
			if(nCtrlRecv != RM_STATUS_CTRL_NIL)
			{
				ProcOneRemoteCmd(nCtrlRecv, m_nStatus, m_nCtrlExecing, nCtrlSend, m_nCtrlCount);
				++nCmdCount;
			}
			// ����
			else
			{
				DoRemoteNilCmdMsg(nStatus, true, 0, m_nCtrlExecing, m_nStatus, nCtrlSend);
			}
		}
		else if(nType == ICV_RM_OBJ_MSGTYPE)
		{
			LH_LOG((LOG_LEVEL_DEBUG, ACE_TEXT("Receive Object Heart Beat Success!")));
			tagObjRMStatusBlock tmpObjStatusBlk;
			// �����������
			RMObject * pTmpObj = NULL;
			std::list<RMObject>::iterator iter = m_RMObjectList.begin();
			for(; iter != m_RMObjectList.end(); ++iter)
			{
				if( strcmp((*iter).szObjName, strObjName.c_str()) == 0)
				{
					pTmpObj = &(*iter);
					break;
				}
			}

			// �Ҳ�������, ��������������Զ����Ϣ
			if(pTmpObj == NULL)
			{
				LH_LOG((LOG_LEVEL_WARN, ACE_TEXT("Failed to find RM object! ObjName = %s"), strObjName.c_str()));
				break;
			}
			// ��ȡ���������
			tagObjRMStatusBlock * pObjBlock = m_shmHelper.GetObjBlockByIdx(pTmpObj->nObjBlockIdx);
			// �������ֲ�����
			memcpy(&tmpObjStatusBlk, pObjBlock,sizeof(tagObjRMStatusBlock));
			// ���¶Ե������������״̬��ʱ���
			tmpObjStatusBlk.objRmStatus.nPeerStatus = nStatus;
			tmpObjStatusBlk.objRmStatus.peerTimeStamp = ACE_OS::gettimeofday();
			// ������������������
			if(nCtrlRecv != RM_STATUS_CTRL_NIL)
			{
				ProcOneRemoteCmd(nCtrlRecv, tmpObjStatusBlk.objRmStatus.nStatus, pTmpObj->nCtrlExec, 
					pTmpObj->nSend, pTmpObj->nCtrlCount);
				++nCmdCount;
			}
			// ���Ե��������״̬�뱾�ض���״̬�������ͻ����ʱ
			else
			{
				DoRemoteNilCmdMsg(nStatus, tmpObjStatusBlk.bSingle, pTmpObj->nPrimSeq,pTmpObj->nCtrlExec, tmpObjStatusBlk.objRmStatus.nStatus, 
					pTmpObj->nSend);
			}
			// ����������������ڴ�
			m_shmHelper.SetObjBlockByIdx(pTmpObj->nObjBlockIdx, &tmpObjStatusBlk);
		}
		++nMsgCount;
	}
}

void CRMStatusCtrl::SendCmdAndStatus(long nCtrlSend)
{
	// ����SCADA��������������
	m_pComm->SendStatus(ICV_RM_SCADA_MSGTYPE, m_nStatus, nCtrlSend,NULL);
	//����SCADS�ڵ�����״̬
	m_shmHelper.SetStatus(m_nStatus);

	//���Ͷ�����������������
	std::list<RMObject>::iterator iter = m_RMObjectList.begin();
	for(; iter != m_RMObjectList.end(); ++iter)
	{
		tagObjRMStatusBlock* pObjBlock = m_shmHelper.GetObjBlockByIdx((*iter).nObjBlockIdx);
		m_pComm->SendStatus(ICV_RM_OBJ_MSGTYPE, pObjBlock->objRmStatus.nStatus, (*iter).nSend,(*iter).szObjName);
		(*iter).nSend = RM_STATUS_CTRL_NIL;
	}
}

/**
 *  Main Ctrl Flow.
 *
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 *  @version     07/16/2008  chenzhiquan  Print Log Info For Debug and analysis.
 */
void CRMStatusCtrl::DoCtrl()
{
	long nCtrlSend = RM_STATUS_CTRL_NIL;
	long nCmdCount = 0, nMsgCount = 0;
	
	// 1.�����ؿ�������
	DoLocalCmds(nCtrlSend);

	// 2.����Զ�̿����������������
	DoRemoteCmds(nCtrlSend, nCmdCount, nMsgCount);

	// 3. ����Զ������ʧ�ܣ�δ�յ���Ϣ��
	if (nMsgCount == 0)
	{
		LH_LOG((LOG_LEVEL_DEBUG, ACE_TEXT("Receive Heart Beat Failed!")));
		m_nStatusCount++;
		if (m_nStatusCount > m_nThreshold)
		{
			if (m_nStatus != RM_STATUS_ACTIVE)
			{
				LH_LOG((LOG_LEVEL_INFO, ACE_TEXT("Detect Peer Down! RMStatus Change To Active!")));
			}
			m_nStatus = RM_STATUS_ACTIVE;
		}
		
		if (m_nCtrlExecing != RM_STATUS_CTRL_NIL)
		{ // Ctrl Cmd Executing
			m_nCtrlCount ++;
			if (m_nCtrlCount > m_nThreshold)
			{
				m_nCtrlExecing = RM_STATUS_CTRL_NIL;
				m_nStatus = RM_STATUS_ACTIVE;
			}
			else
			{
				// Calc Send Cmd.
				nCtrlSend = (m_nCtrlExecing == RM_STATUS_ACTIVE) ? RM_STATUS_CTRL_INACTIVE : RM_STATUS_CTRL_ACTIVE;
			}
		}
	}

	// 4. ������������������
	SendCmdAndStatus(nCtrlSend);
}

/**
 *  Destructor.
 *
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 */
CRMStatusCtrl::~CRMStatusCtrl()
{
	if (m_pComm)
	{
		SAFE_DELETE(m_pComm);
	}
}



bool CRMStatusCtrl::operator!=( const CRMStatusCtrl& obj )
{
	if (m_nThreshold != obj.m_nThreshold)
	{
		return true;
	}

	if (m_tvInterval != obj.m_tvInterval)
	{
		return true;
	}

	if (m_nSequence != obj.m_nSequence)
	{
		return true;
	}

	if (*m_pComm != *(obj.m_pComm))
	{
		return true;
	}

	if (m_RMObjectList != obj.m_RMObjectList)
	{
		return true;
	}

	return false;
} 
