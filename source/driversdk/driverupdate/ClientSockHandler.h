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

#define MAX_RECVBUF_LEN					1024 * 2		// 每个包最大只有1K，开辟两个包的缓冲区 
#define MAX_SENDBUF_LEN					1024 * 2		// 每个包最大只有1K，开辟两个包的缓冲区 

#define COMMAND_NOTIFY_SOURCE			0x01		// 通告自己的来源（1Byte: 1, websvr; 2,网关）
#define COMMAND_REQ_PROG_GETVER			0x10		// 查询程序版本号
#define COMMAND_REQ_PROG_UPDATE			0x11		// 更新程序
#define COMMAND_REQ_CONFIG_GETVER		0x20		// 查询配置版本号
#define COMMAND_REQ_CONFIG_UPDATE		0x21		// 更新配置
#define COMMAND_REQ_DIAG_GETTOTAL		0x30		// 获取总体诊断信息，设备读写次数、发送次数、时间
#define COMMAND_REQ_DIAG_GETDEVICE		0x31		// 获取某个设备的诊断信息，设备读写次数、发送次数、时间

#define CONN_FROM_UNKNOWN				0			// 还未明确来自哪里的连接。连接建立后，必须声明自己的来源
#define CONN_FROM_WEBSRV				1			// 来自WebSvr的连接
#define CONN_FROM_GATEWAY				2			// 来自网关的连接

#define	STATUS_COMMAND_SUCCESS			0x00		// 命令执行成功
#define STATUS_COMMAND_FAIL				0x01		// 命令执行失败

#define STATUS_UPDATE_START				0x12		// 准备接收文件
#define	STATUS_UPDATE_CONTINUE			0x13		// 已接收继续发送
#define STATUS_UPDATE_END				0x14		// 文件接收结束
#define	STATUS_UPDATE_DIR				0x15		// 发送目录中所有文件
#define	STATUS_UPDATE_FILE				0x16		// 发送单一文件

// 包的通用格式：包头标识(0xAB)+长度(2Byte)+事务号(1B)+命令ID(1B)+数据区(N)+包结束标识(0xED)。包的长度不包括包头包尾、长度本身。包最小为5个字节
#define PACK_MIN_LENGTH					6			// 包最少需要5个字节
#define PACK_HEADER_FLAG				0xAB		// 包头标识
#define PACK_FOOTER_FLAG				0xED		// 包尾标识
#define PACK_FIXED_LENGTH				5
#define PACK_HEADER_LENGTH				3

// 本类是处理通过侦听端口接收到的命令请求处理
class CMainTask;
class CCycleReader;
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

	char m_szRecvBuf[MAX_RECVBUF_LEN];
	unsigned int m_nRecvBufLen; // 接收缓冲区中的有效数据
	char m_szSendBuf[MAX_SENDBUF_LEN];
	unsigned int m_nSendBufLen; // 接收缓冲区中的有效数据
	unsigned int m_nConnSource; // 连接来源
	unsigned char m_nTransId;
	 
	char m_szGateWayName[17];	// 当是网关的通信时表示网关的名字，webserver则为webserver
	
	int ParseAPackage(char *szBuff, unsigned int nBufLen);
	int SendResponse(unsigned char nCmdId, char *szBuff, unsigned int nBufLen); 
	int SendLastResponse();

	int DoUpdateCmd(unsigned char nStatus, char *szBuff, unsigned int nBufLen, char *szOutBuf, unsigned int * pnActualOutBufLen);

	int Split(string &s, string &delim, vector<string> *ret);

	time_t				m_tmLastConnect;

	ofstream m_filestream;
	bool m_isSavedFile;

	unsigned int m_nLastSendLength; // 接收缓冲区中的有效数据
public:
	bool	m_bConnected;
	long TryConnectToServer(); // 尝试连接或者重连服务器
	long SendDataToServer( char * szRegisterBuf, long lBufLen );
	CMainTask		*	m_pMainTask;
	CCycleReader	*	m_pCycleReader;
};

typedef ACE_Acceptor<CClientSockHandler, ACE_SOCK_ACCEPTOR> COMMANDACCEPTOR; 
