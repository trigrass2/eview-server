/**************************************************************
 *  Filename:    RMPeerCommunicator.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: Handler Communication To Peer Node.
 *
 *  @author:     chenzhiquan
 *  @version     05/28/2008  chenzhiquan  Initial Version
**************************************************************/
#include "RMPeerCommunicator.h"
#include <ace/SOCK_Dgram.h>
#include <ace/Time_Value.h>
#include <ace/OS_NS_sys_time.h>
#include "pklog/pklog.h"
#include "common/cvGlobalHelper.h"

/**
 *  Add IP Address.
 *
 *  @param  -[in,out]  const string&  strHostIP: [IP addr]
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 */
long CRMUDPComm::AddIPAddr( const string &strHostIP )
{
	ACE_INET_Addr addr(0, strHostIP.c_str(), AF_UNSPEC);
	ACE_UINT32 ulIP = addr.get_ip_address();
	m_vecIP.push_back(ulIP);
	return ICV_SUCCESS;
}

/**
 *  Add Port Address.
 *
 *  @param  -[in,out]  u_short  usPort: [comment]
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 */
long CRMUDPComm::AddPortAddr( u_short usPort )
{
	m_vecPort.push_back(usPort);
	return ICV_SUCCESS;
}

/**
 *  Send Status To Peer.
 *
 *  @param  -[in]  long  nStatus: [RM Status]
 *  @param  -[in]  long  nCtrl: [RM Ctrl Cmd]
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 */
void CRMUDPComm::SendStatus(int nType, long nStatus, long nCtrl, char *pszObjName)
{
	tagMsgBlock msg;
	memset(&msg, 0, sizeof(tagMsgBlock));
	msg.usStatus = (u_short)nStatus;
	msg.usCtrl = (u_short)nCtrl;
	msg.usType = nType;

	// 对象名为空，发送SCADA节点心跳
	if(pszObjName != NULL && pszObjName[0] != 0)
	{
		CVStringHelper::Safe_StrNCpy( msg.szObjName, pszObjName, PK_SHORTFILENAME_MAXLEN);
	}

	ACE_INET_Addr AddrLocal;
	ACE_SOCK_Dgram udp(AddrLocal);

	for (size_t i = 0; i < m_vecIP.size(); i++)
	{
		for (size_t j=0; j < m_vecPort.size(); j++)
		{
			ACE_INET_Addr addrTo(m_vecPort[j], m_vecIP[i]);
			SendBuffer(&msg, sizeof(tagMsgBlock), udp, addrTo);
		}
	}
	udp.close();
}

/**
 *  Get Status From Peer.
 *
 *  @param  -[out]  long&  nStatus: [Status]
 *  @param  -[out]  long&  nCtrl: [Ctrl Cmd]
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 */
long CRMUDPComm::GetStatus(int &ntype, long &nStatus, long &nCtrl, std::string &strObjName)
{
	long nErr = EC_ICV_RMSERVICE_NO_HEART_BEAT_RECVED;
	tagMsgBlock msg;
	for (size_t j=0; j < m_vecSockDgram.size(); j++)
	{
		if (RecvBuffer(&msg, sizeof(tagMsgBlock), *m_vecSockDgram[j]) == ICV_SUCCESS)
		{
			nErr = ICV_SUCCESS;
		}
	}

	if (nErr == ICV_SUCCESS)
	{
		nStatus = msg.usStatus;
		nCtrl = msg.usCtrl;
		ntype = msg.usType;

		if( msg.usType == ICV_RM_OBJ_MSGTYPE )
		{
			LH_LOG((LOG_LEVEL_DEBUG, ACE_TEXT("Receive a obj message!status:%d, ctrl:%d, type:%d"),nStatus,nCtrl,ntype));
			if(msg.szObjName[0] != 0)
			{
				strObjName = msg.szObjName;
			}
			else
			{
				LH_LOG((LOG_LEVEL_ERROR, ACE_TEXT("Invalid message, object name is null!")));
				nErr = EC_ICV_RMSERVICE_ERR_MSG;
			}
		}
		else
		{
			LH_LOG((LOG_LEVEL_DEBUG, ACE_TEXT("Receive a scada message!status:%d, ctrl:%d, type:%d"),nStatus,nCtrl,ntype));
		}
	}

	return nErr;	
}

/**
 *  Constructor.
 *
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 */
CRMUDPComm::CRMUDPComm()
{
	
}

/**
 *  Send Buffer To Peer.
 *
 *  @param  -[in]  const void*  szBuf: [Buffer]
 *  @param  -[in]  size_t  nLength: [Length Of Data]
 *  @param  -[in]  ACE_INET_Addr&  addr: [Address Of UDP]
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 */
long CRMUDPComm::SendBuffer( const void* szBuf, size_t nLength, ACE_SOCK_Dgram &udp, ACE_INET_Addr &to )
{
	//ACE_INET_Addr addrPeer;
	//ACE_SOCK_Dgram udp(addrLocal);
	//ACE_Time_Value now = ACE_OS::gettimeofday();
	//ssize_t nRet = udp.send(szBuf, nLength, addr, 0, &now);
	ssize_t nRet = udp.send(szBuf, nLength, to, 0); // No Timeout.
	//udp.close();
	if (nRet == -1 && ACE_OS::last_error() == ETIME) // TimeOut
	{
		LH_LOG((LOG_LEVEL_DEBUG, ACE_TEXT("Timeout When Sending Buffer To %s:%d"), to.get_host_addr(), to.get_port_number()));
	}
	else
	{
		if(nRet > 0)
			LH_LOG((LOG_LEVEL_DEBUG, ACE_TEXT("Successfully Send To %s:%d"), to.get_host_addr(), to.get_port_number()));
		else
		{
			LH_LOG((LOG_LEVEL_DEBUG, ACE_TEXT("Failed Send To %s:%d"), to.get_host_addr(), to.get_port_number()));
		}
	}
	return ICV_SUCCESS;
}

/**
 *  RecvBuffer From PeerAddress.
 *
 *  @param  -[in, out]  void*  szBuf: [Buffer]
 *  @param  -[in]  size_t  nLength: [Lenth Of Buffer]
 *  @param  -[in]  ACE_INET_Addr&  addr: [comment]
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 */
long CRMUDPComm::RecvBuffer( void* szBuf, size_t nLength, ACE_SOCK_Dgram &udp )
{
	long nErr = EC_ICV_RMSERVICE_NO_HEART_BEAT_RECVED;
	//ACE_SOCK_Dgram udp(udp);
	ACE_INET_Addr peer_addr;
	//ACE_SOCK_Dgram udp;

	//ACE_Time_Value now = ACE_OS::gettimeofday();
	ACE_Time_Value timeout(0, 1000); //  Wait For 1ms.
	do
	{
		ssize_t nRet = udp.recv(szBuf, nLength, peer_addr, 0, &timeout);

		if (nRet == -1 && ACE_OS::last_error() == ETIME) // TimeOut
		{
			break;
		}
		else if ((size_t)nRet == nLength)
		{
			ACE_UINT32 ulIP = peer_addr.get_ip_address();
			// 检查IP是否在接收列表中
			vector<ACE_UINT32>::iterator itIP = m_vecIP.begin();
			for (; itIP != m_vecIP.end() && (*itIP) != ulIP; itIP++);
			// 如果不存在，接收失败
			if (itIP == m_vecIP.end())
			{
				LH_LOG((LOG_LEVEL_DEBUG, "Receive msg from [ip = %s] is not peer addr, drop it", peer_addr.get_host_addr()));
			}
			else
			{
				LH_LOG((LOG_LEVEL_DEBUG, "Receive Msg From [ip = %s]", peer_addr.get_host_addr()));
				nErr = ICV_SUCCESS;
			}
			break;
		}
		else
		{
			LH_LOG((LOG_LEVEL_DEBUG, "Failed to rcv msg From [ip = %s],errno = %d", peer_addr.get_host_addr(), ACE_OS::last_error()));
		}
	}while(true);
	//udp.close();
	return nErr;
}

/**
 *  Do Some Init Stuffs.
 *
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 */
long CRMUDPComm::Initialize()
{
	// Init UDP SockDgram
	for (size_t i=0; i < m_vecPort.size(); i++)
	{
		ACE_INET_Addr addr(m_vecPort[i]);
		ACE_SOCK_Dgram *pDgram = new ACE_SOCK_Dgram();
		if (pDgram->open(addr) != ICV_SUCCESS)
		{ // Failed
			LH_LOG((LOG_LEVEL_ERROR, ACE_TEXT("Error Open UDP Port#%d"), m_vecPort[i]));
			delete pDgram;
		}
		else
		{
			m_vecSockDgram.push_back(pDgram);
		}
	}

	if (m_vecSockDgram.size() == 0)
	{
		LH_LOG((LOG_LEVEL_CRITICAL, ACE_TEXT("None Of UDP Port Avalible!")));
		return EC_ICV_RMSERVICE_NO_UDP_PORT_AVALIBLE;
	}
	return ICV_SUCCESS;
}
/**
 *  Destructure.
 *
 *
 *  @version     05/28/2008  chenzhiquan  Initial Version.
 */
CRMUDPComm::~CRMUDPComm()
{
	Finalize();
}

/**
 *  Finalize UDP ACE_SOCK_Dgram ojbect.
 *
 *
 *  @version     05/29/2008  chenzhiquan  Initial Version.
 */
long CRMUDPComm::Finalize()
{
	for (size_t i = 0; i < m_vecSockDgram.size(); i++)
	{
		ACE_SOCK_Dgram* pDgram = m_vecSockDgram[i];
		pDgram->close();
		delete pDgram;
	}
	m_vecSockDgram.clear();
	return ICV_SUCCESS;
}

bool CRMUDPComm::operator!=( CRMPeerCommunicator& obj)
{
	CRMUDPComm* pObj = dynamic_cast<CRMUDPComm*>(&obj);
	if (NULL == pObj)
	{
		return true;
	}

	if (m_vecIP != pObj->m_vecIP)
	{
		return true;
	}

	if (m_vecPort != pObj->m_vecPort)
	{
		return true;
	}

	return false;
}

