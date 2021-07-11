#pragma once

#include <ace/ACE.h>
#include <ace/Svc_Handler.h>
#include "ace/SOCK_Stream.h"
#include <ace/Synch_Traits.h>
#include <ace/Acceptor.h>
#include <ace/SOCK_Acceptor.h>
#include <string>
#include "../pkmodbustcplib/include/mb.h"
#include "../pkmodbustcplib/include/mbconfig.h"
#include "../pkmodbustcplib/include/mbframe.h"
#include "../pkmodbustcplib/include/mbproto.h"
#include "../pkmodbustcplib/include/mbfunc.h"

using namespace std;

#define MB_TCP_BUF_SIZE ( 2012 + 7 ) 
#define MB_TCP_FUNC 7
#define MB_TCP_LEN 4
#define MB_TCP_UID 6
#define MB_TCP_PID 2
#define MB_TCP_PROTOCOL_ID 0 
#define MB_TCP_PSEUDO_ADDRESS 255
#define MB_TCP_READ_TIMEOUT 1000 
#define MB_TCP_READ_CYCLE 100 

// 本类是处理通过侦听端口接收到的命令请求处理
class CMainTask;
class CClientSockHandler : public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH>
{
public:
	typedef ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH> super;

	CClientSockHandler(ACE_Reactor *r = 0);
	virtual ~CClientSockHandler();

	virtual int open(void* p = 0);

	// 当有输入时该函数被调用.
	virtual int handle_input(ACE_HANDLE fd = ACE_INVALID_HANDLE);

	// 当有输出时该函数被调用.
	virtual int handle_output(ACE_HANDLE fd = ACE_INVALID_HANDLE);

	// 当SockHandler从ACE_Reactor中移除时该函数被调用.
	virtual int handle_close(ACE_HANDLE handle, ACE_Reactor_Mask close_mask);

private:
	string m_strPeerIP;
	int m_nPeerPort;

	USHORT m_usTCPBufPos;
	USHORT m_usTCPFrameBytesLeft;
	UCHAR m_aucTCPBuf[MB_TCP_BUF_SIZE];
	USHORT m_usLength;
	eMBException m_eException;

private:
	eMBErrorCode MBTCPReceive(UCHAR * pucRcvAddress, UCHAR ** ppucFrame, USHORT * pusLength);
	eMBErrorCode MBTCPExecute(UCHAR ucRcvAddress, UCHAR * pucFrame, USHORT usLength);
	BOOL MBTCPPortSendResponse(const UCHAR *pucMBTCPFrame, USHORT usTCPLength);
};

typedef ACE_Acceptor<CClientSockHandler, ACE_SOCK_ACCEPTOR> COMMANDACCEPTOR; 
