#include "ace/ACE.h"
#include "ClientSockHandler.h"
#include "MainTask.h"
#include "common/pklog.h"
#include "SystemConfig.h"
#include "ace/SOCK_Connector.h"
#include "MainTask.h"
#include "ace/Connector.h"
#include "common/pkGlobalHelper.h"
#include "CycleReader.h"

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

	m_isSavedFile = false;
	m_nLastSendLength = 0;
}

CClientSockHandler::~CClientSockHandler()
{
}

// 当有客户端连接时
int CClientSockHandler::open( void* p /*= 0*/ )
{
	if (super::open (p) == -1)
	{
		PKLog.LogMessage(PK_LOGLEVEL_INFO, "Accept a link from, super::open (p) failed!");
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
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "Accept a link from client(%s, %d)!", m_strPeerIP.c_str(), m_nPeerPort);
	m_bConnected = true;
	return 0;
}

// 用于接受服务发来的数据
int CClientSockHandler::handle_input( ACE_HANDLE fd /*= ACE_INVALID_HANDLE*/ )
{
	//// 接收数据
	//ssize_t nRecvBytes = this->peer().recv(m_szRecvBuf + m_nRecvBufLen, MAX_RECVBUF_LEN - m_nRecvBufLen);
	//if (nRecvBytes <= 0)
	//{
	//	PKLog.LogMessage(PK_LOGLEVEL_ERROR, "接收到(%s:%d)请求命令长度为 %d!", m_strPeerIP.c_str(), m_nPeerPort, nRecvBytes);
	//	return -1; // 断开连接。因为如果不断开的话，handle_input会不断调用
	//}
	//m_nRecvBufLen += nRecvBytes;

	ssize_t nRecvBytes;
	do
	{
		nRecvBytes = this->peer().recv(m_szRecvBuf + m_nRecvBufLen, MAX_RECVBUF_LEN - m_nRecvBufLen);
		if (nRecvBytes <= 0)
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "接收到(%s:%d)请求命令长度为 %d!", m_strPeerIP.c_str(), m_nPeerPort, nRecvBytes);
			return -1; // 断开连接。因为如果不断开的话，handle_input会不断调用
		}
		m_nRecvBufLen += nRecvBytes;
		// 表示已经读取结束了  // 怎么样判断已经读取完成了？
		if (nRecvBytes < MAX_RECVBUF_LEN - m_nRecvBufLen - nRecvBytes)
		{
			break;
		}
	}while(nRecvBytes > 0 && m_nRecvBufLen < MAX_RECVBUF_LEN);

	if(m_nRecvBufLen < PACK_MIN_LENGTH) // 包还不完整，直接继续
		return 0;
	
	do 
	{
		if((unsigned char)m_szRecvBuf[0] != PACK_HEADER_FLAG)
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "接收到(%s:%d)请求内容不正确，断开连接");
			return -1;
		}

		unsigned short nPackLen = *((unsigned short *) (m_szRecvBuf + 1));
		if(m_nRecvBufLen < (nPackLen + PACK_FIXED_LENGTH)) // 接收到的包的总长度还不够
		{
			return 0;
		}

		if((unsigned char)m_szRecvBuf[nPackLen + PACK_FIXED_LENGTH - 1] != PACK_FOOTER_FLAG) //包尾标识符不对
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "接收到(%s:%d)请求命令长度为 %d!", m_strPeerIP.c_str(), m_nPeerPort, nRecvBytes);
			return -1; // 断开连接。因为如果不断开的话，handle_input会不断调用
		}

		int nRet = ParseAPackage(m_szRecvBuf + PACK_HEADER_LENGTH, nPackLen);
		if(nRet != 0) // 说明可能是包是未经授权的连接
			return -1; // 断开连接
		m_nRecvBufLen -= (nPackLen + 5);
		memcpy(m_szRecvBuf, m_szRecvBuf + nPackLen + 5, m_nRecvBufLen);
	} while (m_nRecvBufLen > PACK_MIN_LENGTH);

	return 0;
}

// 包已经不含包头、长度和包尾标识符了 nBufLen 是 包括了 命令+内容区 的长度
int CClientSockHandler::ParseAPackage(char *szBuff, unsigned int nBufLen)
{
	char *pDataBuf = szBuff;

	// 返回的事务号
	unsigned char nTransId = *pDataBuf;
	pDataBuf ++ ;

	// 返回的命令
	unsigned char nCmdId = *pDataBuf;
	pDataBuf ++ ;

	// 还原命令
	nCmdId -=  0x80;

	unsigned char nStatus = *pDataBuf;
	if (nCmdId != COMMAND_NOTIFY_SOURCE && nStatus == 0x01) // status 为 01 标识执行不成功
	{
		SendLastResponse();
		return 0;
	}

	if(nCmdId == COMMAND_NOTIFY_SOURCE)
	{
		if (nStatus == 0)
		{
			PKLog.LogMessage(PK_LOGLEVEL_NOTICE,"注册到服务端成功");
		}
		return 0;
	}
	else if(nCmdId == COMMAND_REQ_PROG_UPDATE)
	{
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "接收到更新程序命令");
		unsigned int nActOutBufLen = 0;
		memset(m_szSendBuf, 0x00, sizeof(m_szSendBuf));
		DoUpdateCmd(nStatus, pDataBuf, nBufLen, m_szSendBuf, &nActOutBufLen);

		SendResponse(nCmdId, m_szSendBuf, nActOutBufLen);
	}
	else if(nCmdId == COMMAND_REQ_CONFIG_UPDATE)
	{
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "接收到更新配置命令");
		unsigned int nActOutBufLen = 0;
		memset(m_szSendBuf, 0x00, sizeof(m_szSendBuf));
		DoUpdateCmd(nStatus, pDataBuf, nBufLen, m_szSendBuf, &nActOutBufLen);

		SendResponse(nCmdId, m_szSendBuf, nActOutBufLen);
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
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "Client(%s, %d) disconnected!", m_strPeerIP.c_str(), m_nPeerPort);
	// delete this;
	m_bConnected = false;
	return 0;	
}

int CClientSockHandler::SendResponse( unsigned char nCmdId, char *szBuff, unsigned int nBufLen )
{
	char szSendBuf[MAX_SENDBUF_LEN];
	memset(szSendBuf, 0x00, sizeof(szSendBuf));

	char *pSendBuf = szSendBuf;

	// 开始 1B
	*pSendBuf = 0xAB;
	pSendBuf++;

	// 长度 2B
	unsigned short nSendLen = nBufLen + 1;
	memcpy(pSendBuf, & nSendLen, sizeof(unsigned short));
	pSendBuf ++;
	pSendBuf ++;

	// 事务ID
	*pSendBuf = ++m_nTransId;
	pSendBuf ++;

	// 命令ID
	*pSendBuf = nCmdId ;
	pSendBuf ++;

	// 数据区数据
	memcpy(pSendBuf, szBuff, nBufLen);
	pSendBuf += nBufLen; 

	// 结束标记
	*pSendBuf = 0xED;
	pSendBuf ++;

	long lRet = SendDataToServer(szSendBuf, (pSendBuf - szSendBuf));

	if (lRet == 0 )
	{
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE,"发送应答成功");
	}

	m_nLastSendLength = (pSendBuf - szSendBuf);
	memcpy(m_szSendBuf, szSendBuf, m_nLastSendLength);

	return 0;
}

int CClientSockHandler::DoUpdateCmd(unsigned char nStatus, char *szBuff, unsigned int nBufLen, char *szOutBuf, unsigned int * pnActualOutBufLen)
{
	char *pDataBuf = szBuff;

	// 准备接收文件，文件名
	if (nStatus == STATUS_UPDATE_START)
	{
		// 一个0x12 + 文件名 ，文件名长度为 nBufLen - 1(命令) -1(标识)
		pDataBuf++;
		unsigned int nFilenameLen = nBufLen - 2;
		char* szTemp = new char[nFilenameLen + 1 + 3]; // 目标文件名加一个 .DL 标识下载的文件，所以长度+3
		memset(szTemp, 0x00, nFilenameLen + 1 + 3);
		memcpy(szTemp, pDataBuf, nFilenameLen);
		memcpy(szTemp+nFilenameLen, ".DL", 3 );

		string strRelativePath = string(szTemp);
		vector<string> vecPath;
		// 判断是否有子目录，如果有，则先新建目录，然后添加文件
		if(strRelativePath.find('/') != strRelativePath.npos)
		{
			string strToFound="/";
			Split(strRelativePath, strToFound, &vecPath);
			for (int i = 0 ; i < vecPath.size() - 1; i++) // 最后一个必须是文件名，所以这边-1
			{
				string path = SYSTEM_CONFIG->m_strGatewayPath + "/" + vecPath[i];
				//_mkdir(path.c_str());   // 编译器提供的函数，不知道能不能跨平台使用
				CVFileHelper::CreateDir(path.c_str());
			}
		}
		
		// 增加了只传文件名，不传路径的功能
		string strAbsFilename = SYSTEM_CONFIG->m_strGatewayPath + "/" + string(szTemp);

		if (!m_isSavedFile)
		{
			m_filestream.open(strAbsFilename.c_str(),ios::binary);
			m_isSavedFile = true;
		}
		// 反馈到服务 准备开始接收文件
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "准备写入文件:[%s]", szTemp);
		delete szTemp;
		// 发送继续发送的状态，使服务器继续发送文件
		char szResponse[1] = {STATUS_UPDATE_CONTINUE}; 
		*pnActualOutBufLen = 1;
		memcpy(szOutBuf, szResponse, *pnActualOutBufLen);
		//SendResponse(nCmdId, pDataBuf, nFilenameLen);
		//SendResponse(nCmdId, szResponse, 1);
	}
	// 当文件发送完成，仅会发送一个0x13的标识，标识文件保存完成
	else if (nStatus == STATUS_UPDATE_END)
	{
		m_isSavedFile = false;
		m_filestream.close();
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE,"文件接收完成");

		// 返回文件接收完成标识，准备下一个文件，服务器处理后，如果没下一个文件需要发送，则不会再发送文件
		char szResponse[1] = {STATUS_UPDATE_END}; 
		//SendResponse(nCmdId, szResponse, 1);
		*pnActualOutBufLen = 1;
		memcpy(szOutBuf, szResponse, *pnActualOutBufLen);
	}
	else 
	{
		if (m_isSavedFile)
		{
			char* pFile = new char[nBufLen];
			memset(pFile, 0x00, nBufLen);
			memcpy(pFile, pDataBuf, nBufLen - 1);
			m_filestream.write(pFile, nBufLen - 1);
			char szResponse[1] = {STATUS_UPDATE_CONTINUE};
			//SendResponse(nCmdId, szResponse, 1);
			*pnActualOutBufLen = 1;
			memcpy(szOutBuf, szResponse, *pnActualOutBufLen);
		}
	}
	return 0;
}

typedef ACE_Connector<CClientSockHandler,ACE_SOCK_CONNECTOR> MyConnector;
long CClientSockHandler::TryConnectToServer()
{	
	if(m_bConnected)
		return 1;

	// 小于2倍最小连接超时则不能重连
	time_t tmNow;
	time(&tmNow);
	int nTimeSpan = (int)labs(tmNow - m_tmLastConnect);
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
		return 0;
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

int CClientSockHandler::SendLastResponse()
{
	long lRet = SendDataToServer(m_szSendBuf, m_nLastSendLength);

	//if (lRet == 0)
	//{
	//	memset(m_szSendBuf, 0x00, sizeof(m_szSendBuf));
	//	m_nLastSendLength = 0;
	//}
	return 0;
}

int CClientSockHandler::Split( string &s, string &delim, vector<string> *ret )
{
	size_t last = 0;  
	size_t index=s.find_first_of(delim,last);  
	while (index!=std::string::npos)  
	{  
		ret->push_back(s.substr(last,index-last));  
		last=index+1;  
		index=s.find_first_of(delim,last);  
	}  
	if (index-last>0)  
	{  
		ret->push_back(s.substr(last,index-last));  
	}  
	return 0;
}
