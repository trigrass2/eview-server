/**************************************************************
 *  Filename:    RMPeerCommunicator.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: Handle Communication Stuffs.
 *
 *  @author:     chenzhiquan
 *  @version     05/20/2008  chenzhiquan  Initial Version
**************************************************************/
#ifndef _RM_PEER_COMMUNICATOR_H_
#define _RM_PEER_COMMUNICATOR_H_

#include <ace/INET_Addr.h>
#include <ace/SOCK_Dgram.h>
#include "pklog/pklog.h"
#include "common/CommHelper.h"
#include "errcode/ErrCode_iCV_RM.hxx"
#include "common/eviewdefine.h"

#define ICV_RM_SCADA_MSGTYPE		1
#define ICV_RM_OBJ_MSGTYPE			2

class CRMPeerCommunicator
{
public:
	CRMPeerCommunicator(){};
	virtual ~CRMPeerCommunicator(){};
	// Send Status To Peer.
	virtual void SendStatus(int nType, long nStatus, long nCtrl, char *pszObjName) = 0;
	// Get Status From Peer.
	virtual long GetStatus(int &ntype, long &nStatus, long &nCtrl, std::string &strObjName) = 0;
	// Do Some Init Stuffs.
	virtual long Initialize() = 0;
	// Do Some Finalize Stuffs.
	virtual long Finalize() = 0;

	virtual bool operator!=(CRMPeerCommunicator&)
	{
		return false;
	}
};

class CRMUDPComm : public CRMPeerCommunicator
{
	UNITTEST(CRMUDPComm);
	friend class CRMConfigLoader;
public:
	vector<ACE_UINT32> m_vecIP;
	vector<ACE_SOCK_Dgram *> m_vecSockDgram;
	vector<u_short> m_vecPort;


	typedef struct msgBlock
	{
		u_short usType;
		u_short usStatus;
		u_short usCtrl;
		char szObjName[PK_SHORTFILENAME_MAXLEN];
	}tagMsgBlock;

	//bool m_bInited;
protected:
	long SendBuffer(const void* szBuf, size_t nLength, ACE_SOCK_Dgram &udp, ACE_INET_Addr &to);
	// Recv Buffer from udp port
	long RecvBuffer(void* szBuf, size_t nLength, ACE_SOCK_Dgram &udp);

public:
	CRMUDPComm();
	virtual ~CRMUDPComm();

	// Add IP Adress
	long AddIPAddr(const string &strHostIP);
	// Add Port Address
	long AddPortAddr(u_short usPort);
	// Send Buffer udp port

	// Initialization
	virtual long Initialize();
	// Finalization
	virtual long Finalize();

	// Send Status Info
	virtual void SendStatus(int nType, long nStatus, long nCtrl, char *pszObjName);
	// Get Status Info
	virtual long GetStatus(int &ntype, long &nStatus, long &nCtrl, std::string &strObjName);

	virtual bool operator!=(CRMPeerCommunicator&);

};

#endif//_RM_PEER_COMMUNICATOR_H_
