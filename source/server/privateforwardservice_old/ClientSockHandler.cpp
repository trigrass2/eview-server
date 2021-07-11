#include "ace/ACE.h"
#include "ClientSockHandler.h"
#include "MainTask.h"
#include "common/pklog.h"
#include "SystemConfig.h"
#include "SystemConfig.h"
#include "ace/SOCK_Connector.h"
#include "MainTask.h"
#include "ace/Connector.h"
#include "CycleReader.h"
#include "GWProtoDef.h"

#define REQUESTCMD_SOCK_INPUT_SIZE		1024		// 每次请求命令的长度不会太长，所以1024就够了
extern CPKLog	PKLog;

extern ACE_DLL g_dllforwardaccessor;

CClientSockHandler::CClientSockHandler(ACE_Reactor *reactor)
{
	m_nPeerPort = 0;
	m_strPeerIP = "";
	
	m_nConnSource = CONN_FROM_UNKNOWN;
	m_nRecvBufLen = 0;
	memset(m_szRecvBuf, 0x00, sizeof(m_szRecvBuf));

	m_nSendBufLen = 0;
	memset(m_szSendBuf, 0x00, sizeof(m_szSendBuf));

	m_nTransId = 0; // 发送事务号	
	m_pMainTask = NULL;
	m_pCycleReader = NULL;
	m_bConnected = false;
	m_tmLastConnect = 0;
}

CClientSockHandler::~CClientSockHandler()
{
}

// 当有客户端连接时
int CClientSockHandler::open( void* p /*= 0*/ )
{
	if (super::open (p) == -1)
	{
		PKLog.LogMessage(PK_LOGLEVEL_INFO, "connect to server, super::open (p) failed!");
		return -1;	
	}

	ACE_INET_Addr addr;
	if (this->peer().get_remote_addr (addr) != 0)
	{
		PKLog.LogMessage(PK_LOGLEVEL_INFO, "this->peer().get_remote_addr (addr) failed!");
		return -1;
	}

	// 获取对端的IP和Port信息
	m_strPeerIP = addr.get_host_addr();
	m_nPeerPort = addr.get_port_number();
	PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "connect to server(%s, %d) success!", m_strPeerIP.c_str(), m_nPeerPort);
	m_bConnected = true;
	return 0;
}

int CClientSockHandler::handle_input( ACE_HANDLE fd /*= ACE_INVALID_HANDLE*/ )
{
	// 接收数据
	ssize_t nRecvBytes = this->peer().recv(m_szRecvBuf + m_nRecvBufLen, MAX_RECVBUF_LEN - m_nRecvBufLen);
	if (nRecvBytes <= 0)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "接收到(%s:%d)请求命令长度为 %d!", m_strPeerIP.c_str(), m_nPeerPort, nRecvBytes);
		return -1; // 断开连接。因为如果不断开的话，handle_input会不断调用
	}
	m_nRecvBufLen += nRecvBytes;
	do
	{
		ACE_Time_Value tvTimeout(0, 1000); // 1ms ONLY
		nRecvBytes = this->peer().recv(m_szRecvBuf + m_nRecvBufLen, MAX_RECVBUF_LEN - m_nRecvBufLen, &tvTimeout);
		if(nRecvBytes <= 0) // 收不到数据了
			break;
		m_nRecvBufLen += nRecvBytes;
	}while(nRecvBytes > 0 && m_nRecvBufLen < MAX_RECVBUF_LEN);

	if(m_nRecvBufLen < PACK_MIN_LENGTH) // 包还不完整，直接继续
		return 0;
	
	do 
	{
		if(m_szRecvBuf[0] != PACK_HEADER_FLAG)
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "接收到(%s:%d)请求命令长度为 %d!", m_strPeerIP.c_str(), m_nPeerPort, nRecvBytes);
			return -1;
		}

		unsigned short nPackLen = *((unsigned short *) (m_szRecvBuf + 1));
		if(m_nRecvBufLen < (nPackLen + HEADER_FOOTER_LEN)) // 接收到的包的总长度还不够
		{
			return 0;
		}

		if(m_szRecvBuf[nPackLen + HEADER_FOOTER_LEN - 1] != PACK_FOOTER_FLAG) //包尾标识符不对
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "接收到(%s:%d)请求命令长度为 %d!", m_strPeerIP.c_str(), m_nPeerPort, nRecvBytes);
			return -1; // 断开连接。因为如果不断开的话，handle_input会不断调用
		}

		int nRet = ParseAPackage(m_szRecvBuf + HEADER_FOOTER_LEN - 1, nPackLen);
		if(nRet != 0) // 说明可能是包是未经授权的连接
			return -1; // 断开连接
		m_nRecvBufLen -= (nPackLen + HEADER_FOOTER_LEN);
		memcpy(m_szRecvBuf, m_szRecvBuf + nPackLen + HEADER_FOOTER_LEN, m_nRecvBufLen);
	} while (m_nRecvBufLen > PACK_MIN_LENGTH);

	return 0;
}

// 包已经不含包头、长度和包尾标识符了
int CClientSockHandler::ParseAPackage(unsigned char *szBuff, unsigned int nBufLen)
{
	unsigned char *pDataBuf = szBuff;

	unsigned char nTransId = *pDataBuf;
	pDataBuf ++ ;

	unsigned char nCmdId = *pDataBuf;
	pDataBuf ++ ;

	nCmdId -=  0x80;

	if (nCmdId == COMMAND_NOTIFY_SOURCE)
	{
		// 取出ID
		unsigned char cRetCode = 0;
		// 本地组的ID
		memcpy(&cRetCode, pDataBuf, sizeof(unsigned char));
		pDataBuf += sizeof(unsigned char);

		time_t timeSvr;
		memcpy(&timeSvr, pDataBuf, sizeof(time_t));
		m_pCycleReader->OnRegisterToServerResponse(cRetCode, timeSvr);
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE,"注册到服务端成功, 同步时间成功");
		return 0;

	}
	else if(nCmdId == COMMAND_ID_REGISTER_GROUP)
	{
		// 网关的ID
		memcpy(m_pCycleReader->m_szGatewayId, pDataBuf ,GATEWAY_ID_LEN);
		pDataBuf += GATEWAY_ID_LEN;

		// 本地组的ID
		unsigned short uLocalGrpId = 0;
		memcpy(&uLocalGrpId, pDataBuf, sizeof(unsigned short));
		pDataBuf += sizeof(unsigned short);

		// 服务端返回的的ID。0表示错误
		unsigned short uServerGrpId = 0;
		memcpy(&uServerGrpId, pDataBuf, sizeof(unsigned short));
		pDataBuf += sizeof(unsigned short);

		m_pCycleReader->OnRegisterGroupResponse(uLocalGrpId, uServerGrpId);
		return 0;
	}
	else if(nCmdId == COMMAND_ID_UNREGISTER_ALL)
	{
		// 网关的ID
		memcpy(m_pCycleReader->m_szGatewayId, pDataBuf ,GATEWAY_ID_LEN);
		pDataBuf += GATEWAY_ID_LEN;

		// 服务端返回的的ID。0表示错误
		unsigned short uServerGrpId = 0;
		memcpy(&uServerGrpId, pDataBuf, sizeof(unsigned short));
		pDataBuf += sizeof(unsigned short);

		m_pCycleReader->OnUnRegisterAllGroupResponse(uServerGrpId);
	}
	else if(nCmdId == COMMAND_ID_WRTITE_GROUP)
	{
		// 服务端返回的的ID。0表示错误
		unsigned short uServerGrpId = 0;
		memcpy(&uServerGrpId, pDataBuf, sizeof(unsigned short));
		pDataBuf += sizeof(unsigned short);

		unsigned char nStatus = *pDataBuf ++;
		m_pCycleReader->OnWriteGroupResponse(uServerGrpId, nStatus);
	}
	else if(nCmdId == COMMAND_ID_CONTROL)
	{
		// 对设备进行操作
		// 报文的格式： [Tag点个数 2B ][tag名字长度 2B ，tag名字，tag类型 1B，值 ]
	}

	return 0;
}

// 发送数据包时调用
int CClientSockHandler::handle_output( ACE_HANDLE fd /*= ACE_INVALID_HANDLE*/ )
{
	return 0;
}

// 连接断开时调用
int CClientSockHandler::handle_close( ACE_HANDLE handle, ACE_Reactor_Mask close_mask)
{
	m_nRecvBufLen = 0;
	memset(m_szRecvBuf, 0x00, sizeof(m_szRecvBuf));
	this->shutdown();

	PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "Disconnected to server(%s, %d)!", m_strPeerIP.c_str(), m_nPeerPort);
	// delete this;
	m_bConnected = false;
	return 0;	
}

typedef ACE_Connector<CClientSockHandler,ACE_SOCK_CONNECTOR> MyConnector;
long CClientSockHandler::TryConnectToServer()
{	
	if(m_bConnected)
		return 0;

	// 小于2倍最小连接超时则不能重连
	time_t tmNow;
	time(&tmNow);
	int nTimeSpan = (int)abs(tmNow - m_tmLastConnect);
	if(nTimeSpan < 5 * 2)
		return - 1;

	time(&m_tmLastConnect);

	// 进行连接
	MyConnector connector(m_pMainTask->reactor());
	ACE_INET_Addr serverAddr(SYSTEM_CONFIG->m_nServerPort, SYSTEM_CONFIG->m_strServerIp.c_str());

	CClientSockHandler *pSockHandler = this;
	// 设置为同步方式，以便为后面的连接做准备
	peer().disable(ACE_NONBLOCK);
	
	// 设置默认连接超时
	ACE_Time_Value timeout(5);
	ACE_Synch_Options synch_option(ACE_Synch_Options::USE_TIMEOUT, timeout);
	int nRet = connector.connect(pSockHandler, serverAddr, synch_option);
	if(nRet != 0)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "连接服务 %s:%d失败, 返回 %d", SYSTEM_CONFIG->m_strServerIp.c_str(), SYSTEM_CONFIG->m_nServerPort, nRet);
		return -100;
	}
	else
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "连接服务 %s:%d成功，下面尝试注册组", SYSTEM_CONFIG->m_strServerIp.c_str(), SYSTEM_CONFIG->m_nServerPort);
		long lRetFailNum = (-1) * this->m_pCycleReader->RegisterAllGroups();
		long lGroupNum = this->m_pCycleReader->GetGroupsNum();

		if(lRetFailNum != 0)
		{
			if(lGroupNum == lRetFailNum)
			{
				PKLog.LogMessage(PK_LOGLEVEL_ERROR, "RegisterAllGroups after connected all failed(共%d个组, %d 个失败), retcode:%d, will disconnect...",  lGroupNum, lRetFailNum);
				m_nRecvBufLen = 0;
				memset(m_szRecvBuf, 0x00, sizeof(m_szRecvBuf));
				this->shutdown();
				PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "Disconnected to server(%s, %d)!", m_strPeerIP.c_str(), m_nPeerPort);
				// delete this;
				m_bConnected = false;
			}
			else
				PKLog.LogMessage(PK_LOGLEVEL_ERROR, "RegisterAllGroups failed(共%d个组, %d 个失败), retcode:%d, will continue...",  lGroupNum, lRetFailNum);
		}
		else
			PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "RegisterAllGroups success");
	}
	return 0;
}

long CClientSockHandler::SendDataToServer(char * pszSendBuf, long lBufLen )
{ 
	TryConnectToServer();

	if(!m_bConnected)
		return -1;

	ACE_Time_Value tvTimeout(0, 1000 * 3);
	int nSent = this->peer().send(pszSendBuf, lBufLen, &tvTimeout); // 会阻塞在这里，因此采用下面的非阻塞调用
	return nSent;
}
