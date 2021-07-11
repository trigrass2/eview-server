/**************************************************************
*  Filename:    RecvThread.h
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description:     接收消息
*
*  @author:     liuqifeng
*  @version     02/22/2009  liuqifeng  Initial Version
**************************************************************/
// RecvThread.h: interface for the RecvThread class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _RECVTHREAD_H_
#define _RECVTHREAD_H_

#include <ace/Task.h>
#include "ClientSockHandler.h"
#include "DeviceConnection.h"
#include <string>
#include "../pkmodbustcplib/include/mb.h"
#include "../pkmodbustcplib/include/mbconfig.h"
#include "../pkmodbustcplib/include/mbframe.h"
#include "../pkmodbustcplib/include/mbproto.h"
#include "../pkmodbustcplib/include/mbfunc.h"
#include "SystemConfig.h"

// 该动态库导出的函数
typedef int (*PFN_ReadTags)(ModbusTagVec &modbusTagVec);		// 读取一批tag点的数据。 参数不能被覆盖
typedef int (*PFN_WriteTags)(ModbusTagVec &modbusTagVec);		// 写入一批tag点的数据
typedef int (*PFN_Init)(ModbusTagVec &modbusTagVec);		// 所有配置为要modbus转发的tag点数组
typedef int (*PFN_UnInit)(long lHandle);						// 所有配置为要modbus转发的tag点数组

class CMainTask  : public ACE_Task<ACE_MT_SYNCH>
{
	friend class ACE_Singleton<CMainTask, ACE_Thread_Mutex>;
public:
	CMainTask();
	virtual ~CMainTask();
	int Start();
	void Stop();

protected:
	virtual int svc(void);

public:
	ACE_DLL			m_dllTagDataRW;
	int				m_hTagDataRW;	// 初始化返回的句柄
	PFN_Init		m_pfnInit;
	PFN_UnInit		m_pfnUnInit;
	PFN_ReadTags	m_pfnReadTags;
	PFN_WriteTags	m_pfnWriteTags;

	int ReadTags(ModbusTagVec &modbusTagVec);
	int WriteTags(ModbusTagVec &modbusTagVec);
protected:
	COMMANDACCEPTOR	m_cmdAcceptor;	// 接收客户端请求命令连接的socket服务连接器
	ACE_Reactor* m_pReactor;
	bool	m_bSerialConn;	// socket 还是串口

	// 以下为串口通信所需要的变量
	bool	m_bSerialExit;
	CDeviceConnection *m_pDeviceConnection;
	USHORT m_usTCPBufPos;
	USHORT m_usTCPFrameBytesLeft;
	UCHAR m_aucTCPBuf[MB_TCP_BUF_SIZE];
	USHORT m_usLength;
	eMBException m_eException;

	void SvcSerial(string strConnParam);
	eMBErrorCode MBRTUExecute(UCHAR ucRcvAddress, UCHAR * pucFrame, USHORT usLength);
	BOOL MBRTUSendResponse( const UCHAR *pucMBTCPFrame, USHORT usTCPLength );

};

#define MAIN_TASK ACE_Singleton<CMainTask, ACE_Thread_Mutex>::instance()

#endif // !defined(AFX_TCPRECVTHREAD_H__520D8B7B_FC1A_46C3_9C65_A771090CEF98__INCLUDED_)
