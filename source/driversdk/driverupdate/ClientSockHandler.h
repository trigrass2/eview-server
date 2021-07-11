#pragma once

#include <ace/ACE.h>
#include <ace/Svc_Handler.h>
#include "ace/SOCK_Stream.h"
#include <ace/Synch_Traits.h>
#include <ace/Acceptor.h>
#include <ace/SOCK_Acceptor.h>
#include <string>
#include <vector>
#include <fstream>

using namespace std;

#define MAX_RECVBUF_LEN					1024 * 2		// ÿ�������ֻ��1K�������������Ļ����� 
#define MAX_SENDBUF_LEN					1024 * 2		// ÿ�������ֻ��1K�������������Ļ����� 

#define COMMAND_NOTIFY_SOURCE			0x01		// ͨ���Լ�����Դ��1Byte: 1, websvr; 2,���أ�
#define COMMAND_REQ_PROG_GETVER			0x10		// ��ѯ����汾��
#define COMMAND_REQ_PROG_UPDATE			0x11		// ���³���
#define COMMAND_REQ_CONFIG_GETVER		0x20		// ��ѯ���ð汾��
#define COMMAND_REQ_CONFIG_UPDATE		0x21		// ��������
#define COMMAND_REQ_DIAG_GETTOTAL		0x30		// ��ȡ���������Ϣ���豸��д���������ʹ�����ʱ��
#define COMMAND_REQ_DIAG_GETDEVICE		0x31		// ��ȡĳ���豸�������Ϣ���豸��д���������ʹ�����ʱ��

#define CONN_FROM_UNKNOWN				0			// ��δ��ȷ������������ӡ����ӽ����󣬱��������Լ�����Դ
#define CONN_FROM_WEBSRV				1			// ����WebSvr������
#define CONN_FROM_GATEWAY				2			// �������ص�����

#define	STATUS_COMMAND_SUCCESS			0x00		// ����ִ�гɹ�
#define STATUS_COMMAND_FAIL				0x01		// ����ִ��ʧ��

#define STATUS_UPDATE_START				0x12		// ׼�������ļ�
#define	STATUS_UPDATE_CONTINUE			0x13		// �ѽ��ռ�������
#define STATUS_UPDATE_END				0x14		// �ļ����ս���
#define	STATUS_UPDATE_DIR				0x15		// ����Ŀ¼�������ļ�
#define	STATUS_UPDATE_FILE				0x16		// ���͵�һ�ļ�

// ����ͨ�ø�ʽ����ͷ��ʶ(0xAB)+����(2Byte)+�����(1B)+����ID(1B)+������(N)+��������ʶ(0xED)�����ĳ��Ȳ�������ͷ��β�����ȱ�������СΪ5���ֽ�
#define PACK_MIN_LENGTH					6			// ��������Ҫ5���ֽ�
#define PACK_HEADER_FLAG				0xAB		// ��ͷ��ʶ
#define PACK_FOOTER_FLAG				0xED		// ��β��ʶ
#define PACK_FIXED_LENGTH				5
#define PACK_HEADER_LENGTH				3

// �����Ǵ���ͨ�������˿ڽ��յ�������������
class CMainTask;
class CCycleReader;
class CClientSockHandler : public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH>
{
public:
	typedef ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH> super;

	CClientSockHandler(ACE_Reactor *r = 0);
	virtual ~CClientSockHandler();

	virtual int open(void* p = 0);
 
	// ��������ʱ�ú���������.
	virtual int handle_input(ACE_HANDLE fd = ACE_INVALID_HANDLE);

	// �������ʱ�ú���������.
	virtual int handle_output(ACE_HANDLE fd = ACE_INVALID_HANDLE);

	// ��SockHandler��ACE_Reactor���Ƴ�ʱ�ú���������.
	virtual int handle_close(ACE_HANDLE handle, ACE_Reactor_Mask close_mask);

private:
	string m_strPeerIP;
	int m_nPeerPort;

	char m_szRecvBuf[MAX_RECVBUF_LEN];
	unsigned int m_nRecvBufLen; // ���ջ������е���Ч����
	char m_szSendBuf[MAX_SENDBUF_LEN];
	unsigned int m_nSendBufLen; // ���ջ������е���Ч����
	unsigned int m_nConnSource; // ������Դ
	unsigned char m_nTransId;
	 
	char m_szGateWayName[17];	// �������ص�ͨ��ʱ��ʾ���ص����֣�webserver��Ϊwebserver
	
	int ParseAPackage(char *szBuff, unsigned int nBufLen);
	int SendResponse(unsigned char nCmdId, char *szBuff, unsigned int nBufLen); 
	int SendLastResponse();

	int DoUpdateCmd(unsigned char nStatus, char *szBuff, unsigned int nBufLen, char *szOutBuf, unsigned int * pnActualOutBufLen);

	int Split(string &s, string &delim, vector<string> *ret);

	time_t				m_tmLastConnect;

	ofstream m_filestream;
	bool m_isSavedFile;

	unsigned int m_nLastSendLength; // ���ջ������е���Ч����
public:
	bool	m_bConnected;
	long TryConnectToServer(); // �������ӻ�������������
	long SendDataToServer( char * szRegisterBuf, long lBufLen );
	CMainTask		*	m_pMainTask;
	CCycleReader	*	m_pCycleReader;
};

typedef ACE_Acceptor<CClientSockHandler, ACE_SOCK_ACCEPTOR> COMMANDACCEPTOR; 
