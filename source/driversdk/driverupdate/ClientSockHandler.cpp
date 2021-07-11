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

#define REQUESTCMD_SOCK_INPUT_SIZE		1024		// ÿ����������ĳ��Ȳ���̫��������1024�͹���
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

	m_nTransId = 0; // ���������	
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

// ���пͻ�������ʱ
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

	// ��ȡ�Զ˵�IP��Port��Ϣ
	m_strPeerIP = addr.get_host_addr();
	m_nPeerPort = addr.get_port_number();
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "Accept a link from client(%s, %d)!", m_strPeerIP.c_str(), m_nPeerPort);
	m_bConnected = true;
	return 0;
}

// ���ڽ��ܷ�����������
int CClientSockHandler::handle_input( ACE_HANDLE fd /*= ACE_INVALID_HANDLE*/ )
{
	//// ��������
	//ssize_t nRecvBytes = this->peer().recv(m_szRecvBuf + m_nRecvBufLen, MAX_RECVBUF_LEN - m_nRecvBufLen);
	//if (nRecvBytes <= 0)
	//{
	//	PKLog.LogMessage(PK_LOGLEVEL_ERROR, "���յ�(%s:%d)���������Ϊ %d!", m_strPeerIP.c_str(), m_nPeerPort, nRecvBytes);
	//	return -1; // �Ͽ����ӡ���Ϊ������Ͽ��Ļ���handle_input�᲻�ϵ���
	//}
	//m_nRecvBufLen += nRecvBytes;

	ssize_t nRecvBytes;
	do
	{
		nRecvBytes = this->peer().recv(m_szRecvBuf + m_nRecvBufLen, MAX_RECVBUF_LEN - m_nRecvBufLen);
		if (nRecvBytes <= 0)
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "���յ�(%s:%d)���������Ϊ %d!", m_strPeerIP.c_str(), m_nPeerPort, nRecvBytes);
			return -1; // �Ͽ����ӡ���Ϊ������Ͽ��Ļ���handle_input�᲻�ϵ���
		}
		m_nRecvBufLen += nRecvBytes;
		// ��ʾ�Ѿ���ȡ������  // ��ô���ж��Ѿ���ȡ����ˣ�
		if (nRecvBytes < MAX_RECVBUF_LEN - m_nRecvBufLen - nRecvBytes)
		{
			break;
		}
	}while(nRecvBytes > 0 && m_nRecvBufLen < MAX_RECVBUF_LEN);

	if(m_nRecvBufLen < PACK_MIN_LENGTH) // ������������ֱ�Ӽ���
		return 0;
	
	do 
	{
		if((unsigned char)m_szRecvBuf[0] != PACK_HEADER_FLAG)
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "���յ�(%s:%d)�������ݲ���ȷ���Ͽ�����");
			return -1;
		}

		unsigned short nPackLen = *((unsigned short *) (m_szRecvBuf + 1));
		if(m_nRecvBufLen < (nPackLen + PACK_FIXED_LENGTH)) // ���յ��İ����ܳ��Ȼ�����
		{
			return 0;
		}

		if((unsigned char)m_szRecvBuf[nPackLen + PACK_FIXED_LENGTH - 1] != PACK_FOOTER_FLAG) //��β��ʶ������
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "���յ�(%s:%d)���������Ϊ %d!", m_strPeerIP.c_str(), m_nPeerPort, nRecvBytes);
			return -1; // �Ͽ����ӡ���Ϊ������Ͽ��Ļ���handle_input�᲻�ϵ���
		}

		int nRet = ParseAPackage(m_szRecvBuf + PACK_HEADER_LENGTH, nPackLen);
		if(nRet != 0) // ˵�������ǰ���δ����Ȩ������
			return -1; // �Ͽ�����
		m_nRecvBufLen -= (nPackLen + 5);
		memcpy(m_szRecvBuf, m_szRecvBuf + nPackLen + 5, m_nRecvBufLen);
	} while (m_nRecvBufLen > PACK_MIN_LENGTH);

	return 0;
}

// ���Ѿ�������ͷ�����ȺͰ�β��ʶ���� nBufLen �� ������ ����+������ �ĳ���
int CClientSockHandler::ParseAPackage(char *szBuff, unsigned int nBufLen)
{
	char *pDataBuf = szBuff;

	// ���ص������
	unsigned char nTransId = *pDataBuf;
	pDataBuf ++ ;

	// ���ص�����
	unsigned char nCmdId = *pDataBuf;
	pDataBuf ++ ;

	// ��ԭ����
	nCmdId -=  0x80;

	unsigned char nStatus = *pDataBuf;
	if (nCmdId != COMMAND_NOTIFY_SOURCE && nStatus == 0x01) // status Ϊ 01 ��ʶִ�в��ɹ�
	{
		SendLastResponse();
		return 0;
	}

	if(nCmdId == COMMAND_NOTIFY_SOURCE)
	{
		if (nStatus == 0)
		{
			PKLog.LogMessage(PK_LOGLEVEL_NOTICE,"ע�ᵽ����˳ɹ�");
		}
		return 0;
	}
	else if(nCmdId == COMMAND_REQ_PROG_UPDATE)
	{
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "���յ����³�������");
		unsigned int nActOutBufLen = 0;
		memset(m_szSendBuf, 0x00, sizeof(m_szSendBuf));
		DoUpdateCmd(nStatus, pDataBuf, nBufLen, m_szSendBuf, &nActOutBufLen);

		SendResponse(nCmdId, m_szSendBuf, nActOutBufLen);
	}
	else if(nCmdId == COMMAND_REQ_CONFIG_UPDATE)
	{
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "���յ�������������");
		unsigned int nActOutBufLen = 0;
		memset(m_szSendBuf, 0x00, sizeof(m_szSendBuf));
		DoUpdateCmd(nStatus, pDataBuf, nBufLen, m_szSendBuf, &nActOutBufLen);

		SendResponse(nCmdId, m_szSendBuf, nActOutBufLen);
	}

	return 0;
}

// �������ݰ�ʱ����
int CClientSockHandler::handle_output( ACE_HANDLE fd /*= ACE_INVALID_HANDLE*/ )
{
	return 0;
}

// ���ӶϿ�ʱ����
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

	// ��ʼ 1B
	*pSendBuf = 0xAB;
	pSendBuf++;

	// ���� 2B
	unsigned short nSendLen = nBufLen + 1;
	memcpy(pSendBuf, & nSendLen, sizeof(unsigned short));
	pSendBuf ++;
	pSendBuf ++;

	// ����ID
	*pSendBuf = ++m_nTransId;
	pSendBuf ++;

	// ����ID
	*pSendBuf = nCmdId ;
	pSendBuf ++;

	// ����������
	memcpy(pSendBuf, szBuff, nBufLen);
	pSendBuf += nBufLen; 

	// �������
	*pSendBuf = 0xED;
	pSendBuf ++;

	long lRet = SendDataToServer(szSendBuf, (pSendBuf - szSendBuf));

	if (lRet == 0 )
	{
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE,"����Ӧ��ɹ�");
	}

	m_nLastSendLength = (pSendBuf - szSendBuf);
	memcpy(m_szSendBuf, szSendBuf, m_nLastSendLength);

	return 0;
}

int CClientSockHandler::DoUpdateCmd(unsigned char nStatus, char *szBuff, unsigned int nBufLen, char *szOutBuf, unsigned int * pnActualOutBufLen)
{
	char *pDataBuf = szBuff;

	// ׼�������ļ����ļ���
	if (nStatus == STATUS_UPDATE_START)
	{
		// һ��0x12 + �ļ��� ���ļ�������Ϊ nBufLen - 1(����) -1(��ʶ)
		pDataBuf++;
		unsigned int nFilenameLen = nBufLen - 2;
		char* szTemp = new char[nFilenameLen + 1 + 3]; // Ŀ���ļ�����һ�� .DL ��ʶ���ص��ļ������Գ���+3
		memset(szTemp, 0x00, nFilenameLen + 1 + 3);
		memcpy(szTemp, pDataBuf, nFilenameLen);
		memcpy(szTemp+nFilenameLen, ".DL", 3 );

		string strRelativePath = string(szTemp);
		vector<string> vecPath;
		// �ж��Ƿ�����Ŀ¼������У������½�Ŀ¼��Ȼ������ļ�
		if(strRelativePath.find('/') != strRelativePath.npos)
		{
			string strToFound="/";
			Split(strRelativePath, strToFound, &vecPath);
			for (int i = 0 ; i < vecPath.size() - 1; i++) // ���һ���������ļ������������-1
			{
				string path = SYSTEM_CONFIG->m_strGatewayPath + "/" + vecPath[i];
				//_mkdir(path.c_str());   // �������ṩ�ĺ�������֪���ܲ��ܿ�ƽ̨ʹ��
				CVFileHelper::CreateDir(path.c_str());
			}
		}
		
		// ������ֻ���ļ���������·���Ĺ���
		string strAbsFilename = SYSTEM_CONFIG->m_strGatewayPath + "/" + string(szTemp);

		if (!m_isSavedFile)
		{
			m_filestream.open(strAbsFilename.c_str(),ios::binary);
			m_isSavedFile = true;
		}
		// ���������� ׼����ʼ�����ļ�
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "׼��д���ļ�:[%s]", szTemp);
		delete szTemp;
		// ���ͼ������͵�״̬��ʹ���������������ļ�
		char szResponse[1] = {STATUS_UPDATE_CONTINUE}; 
		*pnActualOutBufLen = 1;
		memcpy(szOutBuf, szResponse, *pnActualOutBufLen);
		//SendResponse(nCmdId, pDataBuf, nFilenameLen);
		//SendResponse(nCmdId, szResponse, 1);
	}
	// ���ļ�������ɣ����ᷢ��һ��0x13�ı�ʶ����ʶ�ļ��������
	else if (nStatus == STATUS_UPDATE_END)
	{
		m_isSavedFile = false;
		m_filestream.close();
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE,"�ļ��������");

		// �����ļ�������ɱ�ʶ��׼����һ���ļ�����������������û��һ���ļ���Ҫ���ͣ��򲻻��ٷ����ļ�
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

	// С��2����С���ӳ�ʱ��������
	time_t tmNow;
	time(&tmNow);
	int nTimeSpan = (int)labs(tmNow - m_tmLastConnect);
	if(nTimeSpan < 5 * 2)
		return - 1;

	time(&m_tmLastConnect);

	// ��������
	MyConnector connector(m_pMainTask->reactor());
	ACE_INET_Addr serverAddr(SYSTEM_CONFIG->m_nServerPort, SYSTEM_CONFIG->m_strServerIp.c_str());

	CClientSockHandler *pSockHandler = this;
	// ����Ϊͬ����ʽ���Ա�Ϊ�����������׼��
	peer().disable(ACE_NONBLOCK);

	// ����Ĭ�����ӳ�ʱ
	ACE_Time_Value timeout(5);
	ACE_Synch_Options synch_option(ACE_Synch_Options::USE_TIMEOUT, timeout);
	int nRet = connector.connect(pSockHandler, serverAddr, synch_option);
	if(nRet != 0)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "���ӷ��� %s:%dʧ��, ���� %d", SYSTEM_CONFIG->m_strServerIp.c_str(), SYSTEM_CONFIG->m_nServerPort, nRet);
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
	int nSent = this->peer().send(pszSendBuf, lBufLen, &tvTimeout); // �������������˲�������ķ���������
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
