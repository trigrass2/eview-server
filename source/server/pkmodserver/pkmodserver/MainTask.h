/**************************************************************
*  Filename:    RecvThread.h
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description:     ������Ϣ
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

// �ö�̬�⵼���ĺ���
typedef int (*PFN_ReadTags)(ModbusTagVec &modbusTagVec);		// ��ȡһ��tag������ݡ� �������ܱ�����
typedef int (*PFN_WriteTags)(ModbusTagVec &modbusTagVec);		// д��һ��tag�������
typedef int (*PFN_Init)(ModbusTagVec &modbusTagVec);		// ��������ΪҪmodbusת����tag������
typedef int (*PFN_UnInit)(long lHandle);						// ��������ΪҪmodbusת����tag������

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
	int				m_hTagDataRW;	// ��ʼ�����صľ��
	PFN_Init		m_pfnInit;
	PFN_UnInit		m_pfnUnInit;
	PFN_ReadTags	m_pfnReadTags;
	PFN_WriteTags	m_pfnWriteTags;

	int ReadTags(ModbusTagVec &modbusTagVec);
	int WriteTags(ModbusTagVec &modbusTagVec);
protected:
	COMMANDACCEPTOR	m_cmdAcceptor;	// ���տͻ��������������ӵ�socket����������
	ACE_Reactor* m_pReactor;
	bool	m_bSerialConn;	// socket ���Ǵ���

	// ����Ϊ����ͨ������Ҫ�ı���
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
