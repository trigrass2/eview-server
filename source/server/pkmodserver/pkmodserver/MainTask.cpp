/**************************************************************
 *  Filename:    RecvThread.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: ������Ϣ�߳�.
 *
 *  @author:     liuqifeng
 *  @version     04/13/2009  liuqifeng  Initial Version
**************************************************************/
// RecvThread.cpp: implementation of the CRecvThread class.
//
//////////////////////////////////////////////////////////////////////

#include <ace/INET_Addr.h>
#include <ace/Log_Msg.h>
#include "ace/SOCK_Acceptor.h"
#include "ace/Select_Reactor.h"
#include "ace/OS_NS_strings.h"
#include "SerialPort.h"
#include "common/eviewdefine.h"
#include "MainTask.h"
#include "SystemConfig.h"
#include "pklog/pklog.h"
#include "pkcomm/pkcomm.h"
#include "common/eviewdefine.h"
#include "common/PKMiscHelper.h"

extern CPKLog g_logger;

// ����tag��Ķ�̬�⡣DLL��Ϊpkmodbusdata.dll.�����icv·��Ϊ��modbusservice����Ҫ������icv��executableĿ¼�£�
//#define TAGDATARW_MODULE_PATH_ICV		"../../iCentroView/executable"
// PK Data���ڵ�ǰĿ¼��ִ��
#define TAGDATARW_MODULE_PATH			"."
#define TAGDATARW_MODULE_DLLNAME		"./pkmodbusdata"

#define MODBUS_DEFAULT_LISTENPORT		502

unsigned short CRC16 ( unsigned char * puchMsg, unsigned short usDataLen );

#define MODBUSRTU_STATION_LENGHT	1	// վ��
#define MODBUSRTU_CRC_LENGHT		2	// CRC�ĳ���

extern xMBFunctionHandler g_xFuncHandlers[MB_FUNC_HANDLERS_MAX];
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CMainTask::CMainTask()
{
	m_bSerialConn = false;
	ACE_Select_Reactor *pSelectReactor = new ACE_Select_Reactor();
	m_pReactor = new ACE_Reactor(pSelectReactor, 1);
	reactor(m_pReactor);

	m_bSerialExit = false;
	m_pDeviceConnection = NULL;

	m_hTagDataRW = 0;	// ��ʼ�����صľ��
	m_pfnInit = NULL;
	m_pfnUnInit = NULL;
	m_pfnReadTags = NULL;
	m_pfnWriteTags = NULL;
}

CMainTask::~CMainTask()
{
	if (m_pReactor != NULL)
	{
		delete m_pReactor;
		m_pReactor = NULL;
	}
	
}

//�̷߳���
int CMainTask::svc()
{
	chdir(TAGDATARW_MODULE_PATH);
	long nErr = m_dllTagDataRW.open(TAGDATARW_MODULE_DLLNAME);
	if (nErr != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "load dll: %s failed!", TAGDATARW_MODULE_DLLNAME);
		// return -1;
	}
	else
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "load dll: %s success!", TAGDATARW_MODULE_DLLNAME);

		m_pfnInit = (PFN_Init)m_dllTagDataRW.symbol("Init"); 
		m_pfnUnInit = (PFN_UnInit)m_dllTagDataRW.symbol("UnInit"); 
		m_pfnReadTags = (PFN_ReadTags)m_dllTagDataRW.symbol("ReadTags"); 
		m_pfnWriteTags = (PFN_WriteTags)m_dllTagDataRW.symbol("WriteTags"); 
		if(m_pfnInit == NULL || m_pfnUnInit == NULL || m_pfnReadTags == NULL || m_pfnWriteTags == NULL)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "DLL:%s, exportedFunc:Init/UnInit/ReadTags/WriteTags, some not exported!", TAGDATARW_MODULE_DLLNAME);
		}
	}

	if(m_pfnInit) // �е�DLLҲ����Ҫ�˲�����name�Ͳ���Ҫʵ��Init����
		m_hTagDataRW = m_pfnInit(SYSTEM_CONFIG->m_vecAllTags);

	// ��ʽ��Serial;.... or  502
	m_bSerialConn = false;
	std::transform(SYSTEM_CONFIG->m_pForwardInfo->strConnType.begin(), SYSTEM_CONFIG->m_pForwardInfo->strConnType.end(), SYSTEM_CONFIG->m_pForwardInfo->strConnType.begin(), ::tolower); // ��ΪСд��ĸ
	// �����serial��ͷ���Ǵ���
	if(ACE_OS::strcasecmp(SYSTEM_CONFIG->m_pForwardInfo->strConnType.c_str(), "serial") == 0)
	{
		m_bSerialConn = true;
	}
	else
	{
		m_bSerialConn = false;
	}
	// socketͨ�ţ������˿�
	if(!m_bSerialConn)
	{
		unsigned short uListenPort = MODBUS_DEFAULT_LISTENPORT;
		uListenPort = ::atoi(SYSTEM_CONFIG->m_pForwardInfo->strConnParam.c_str()); // ȱʡΪ502
		if(uListenPort <= 0)
			uListenPort = 502;

		this->reactor()->owner(ACE_OS::thr_self ());

		ACE_INET_Addr listenAddr(uListenPort);
		int nResult = m_cmdAcceptor.open (listenAddr, this->reactor());
		if(nResult != 0)
		{
			g_logger.LogMessage(PK_LOGLEVEL_CRITICAL, "�޷��򿪶Զ˿�%d�����������޷����տͻ��˵�����������Ϣ!!!!", uListenPort);
			return -1;
		}
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "�򿪶�tcp�˿�%d������", uListenPort);

		this->reactor()->reset_reactor_event_loop();
		this->reactor()->run_reactor_event_loop();
	}
	else
	{
		m_pDeviceConnection = new CSerialPort();
		m_bSerialExit = false;
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "�򿪶Դ���, ����: %s", SYSTEM_CONFIG->m_pForwardInfo->strConnParam.c_str());
		// ����ͨ��,��һ������Ȼ�󲻶ϵĶ�ȡ��һֱ��ֹͣ�źŷ���
		SvcSerial(SYSTEM_CONFIG->m_pForwardInfo->strConnParam);

		delete m_pDeviceConnection;
		m_pDeviceConnection = NULL;
	}

	if(m_pfnUnInit && m_hTagDataRW != 0)
		m_pfnUnInit(m_hTagDataRW);
	m_dllTagDataRW.close();
	return 0;
}

// ����ʱ��������
void CMainTask::SvcSerial(string strConnParam)
{
	m_pDeviceConnection->SetConnectParam((char *)strConnParam.c_str());
	m_pDeviceConnection->Start(); // ��������
	// ��ʼѭ���Ķ�ȡ���ڵ�����
	char szReadBuf[1024] = {0};
	long lTimeOut = 300; // 500ms
	eMBErrorCode errCodeLast = MB_ENOERR; // �ϴ�ModbusRTU��Ӧ����
	while(!m_bSerialExit)
	{
		// ����ϴ�ʧ�ܣ������ǰ��֡������֡����ʧ�ܵ����Ρ���ʱͨ����ȡ������е�����
		if(errCodeLast != MB_ENOERR)
		{
			long lRecvLen = m_pDeviceConnection->Recv(szReadBuf, sizeof(szReadBuf), 0);	// �Ӷ˿ڶ�ȡ����������
		}

		// ��ȡ���ڵ�����
		long lRecvLen = m_pDeviceConnection->Recv(szReadBuf, sizeof(szReadBuf), lTimeOut);	// �Ӷ˿ڶ�ȡ����������
		if(lRecvLen > 0) // ��ȡ���˻����������ݣ����濪ʼ����У���modbusЭ��Ĵ���
		{
			unsigned char *pCurBuf = (unsigned char*)szReadBuf;
			// Modbus RTU��Э���ʽ��վ��(1�ֽڣ�����������CRC16У�飨2�ֽڣ�����ModbusTCP����6���ֽڵ�����2��CRCУ���ֽ�
			lTimeOut = 0; // �Ժ�Ͳ��ٵȳ�ʱ��
			unsigned char nStationNo = 0;
			// վ��
			memcpy(&nStationNo, pCurBuf, 1);
			pCurBuf++;
			UCHAR ucRcvAddress = 255; // �Ƿ��ǹ㲥��ַ
			errCodeLast =	MBRTUExecute(ucRcvAddress, pCurBuf, lRecvLen - MODBUSRTU_STATION_LENGHT - MODBUSRTU_CRC_LENGHT); // ��ȥվ��1���ֽں�CRC2���ֽ�
		}
	}

}

eMBErrorCode CMainTask::MBRTUExecute(UCHAR ucRcvAddress, UCHAR * pucFrame, USHORT usLength)
{
	eMBErrorCode    eStatus = MB_ENOERR;
	UCHAR ucFunctionCode = pucFrame[MB_PDU_FUNC_OFF];
    m_eException = MB_EX_ILLEGAL_FUNCTION;
    for( int i = 0; i < MB_FUNC_HANDLERS_MAX; i++ )
    {
		// No more function handlers registered. Abort. 
        if( g_xFuncHandlers[i].ucFunctionCode == 0 )
        {
			break;
        }
        
		if( g_xFuncHandlers[i].ucFunctionCode == ucFunctionCode )
        {
			m_eException = g_xFuncHandlers[i].pxHandler(pucFrame, &usLength );
            break;
        }
	}
	
    // If the request was not sent to the broadcast address we return a reply. 
    if( ucRcvAddress != MB_ADDRESS_BROADCAST )
    {
        if( m_eException != MB_EX_NONE ) // �������쳣�������밴�쳣�����봦��
        {
			// An exception occured. Build an error frame.
            usLength = 0;
            pucFrame[usLength++] = ( UCHAR )( ucFunctionCode | MB_FUNC_ERROR );
            pucFrame[usLength++] = m_eException;
        }       
		
		UCHAR *pucMBRTUFrame = ( UCHAR * ) pucFrame - MODBUSRTU_STATION_LENGHT; // ModbusRTU֡������
		USHORT usRTULength = usLength + MODBUSRTU_STATION_LENGHT; // Modbus RTU֡����
		
		// The MBAP header is already initialized because the caller calls this
		// function with the buffer returned by the previous call. Therefore we 
		// only have to update the length in the header. Note that the length 
		// header includes the size of the Modbus PDU and the UID Byte. Therefore 
		// the length is usLength plus one.

		unsigned short nCRCValue = CRC16((unsigned char *)pucMBRTUFrame, usRTULength);
		memcpy(pucMBRTUFrame + usRTULength, &nCRCValue, sizeof(unsigned short));
		usRTULength += MODBUSRTU_CRC_LENGHT;

		if( MBRTUSendResponse( pucMBRTUFrame, usRTULength ) == FALSE )
		{
			eStatus = MB_EIO;
		}	
	}
	return eStatus;
}


BOOL CMainTask::MBRTUSendResponse( const UCHAR *pucMBTCPFrame, USHORT usTCPLength )
{
	long lSentBytes = m_pDeviceConnection->Send((const char *)pucMBTCPFrame, usTCPLength, 500);
	return (lSentBytes == usTCPLength);
}

void CMainTask::Stop()
{
	if(m_bSerialConn)
	{
		m_bSerialExit = true;
	}
	else
	{
		// ֹͣ����
		m_cmdAcceptor.close();
		// �����ر���Ӧ�������
		this->reactor()->end_reactor_event_loop();
	}
	
	// �ȴ���Ӧ���رպ��˳�
	this->wait();
}

int CMainTask::Start()
{
	return this->activate();
}

int CMainTask::ReadTags(ModbusTagVec &modbusTagVec )
{
	if(m_pfnReadTags == NULL)
		return -1;
	int nRet = m_pfnReadTags(modbusTagVec);
	if(nRet == 0) // �����ȡ�ɹ�������Ҫ����ȡ�����ַ������͵�szValueת��Ϊ������szBinData��ȥ
	{
		ModbusTagVec::iterator itTag = modbusTagVec.begin();
		for(; itTag != modbusTagVec.end(); itTag ++)
		{
			ModbusTag *pModbusTag = *itTag;
			ModbusServiceTag *pServiceTag = (ModbusServiceTag *)pModbusTag->pInternalRef;
			// ���ַ������͵�ֵ������tag���ʵ���������ͣ�ת��Ϊ���־����������ͣ�int��bool�ȣ�
			int nRealLen = 0;
			int nCastRet = PKMiscHelper::CastDataFromASCII2Bin(pModbusTag->szValue, pServiceTag->nDataType, pServiceTag->szBinData, sizeof(pServiceTag->szBinData), &nRealLen);
		}
		
	}
	return nRet;
}

// ���Ƶ�ֵ�Ǵ���Ķ����ƣ���Ҫת��ΪASCII�����modbusdata.dll��writetags�ӿ�
int CMainTask::WriteTags(ModbusTagVec &modbusTagVec )
{
	if(m_pfnWriteTags == NULL)
		return MB_EIO;

	// ��д��tag��ǰ����Ҫ��ֵ������������ת��Ϊ�ַ�������
	// ���ң�����Ͷ�ȡ��ֵ��ͻ������Ҫ�����Ƶ���Щ������������һ�Σ�����
	ModbusTagVec tagVec2Write;
	ModbusTagVec::iterator itTag = modbusTagVec.begin();
	for(; itTag != modbusTagVec.end(); itTag ++)
	{
		ModbusTag *pModbusTag = *itTag;
		ModbusServiceTag *pServiceTag = (ModbusServiceTag *)pModbusTag->pInternalRef;
		// ���ַ������͵�ֵ������tag���ʵ���������ͣ�ת��Ϊ���־����������ͣ�int��bool�ȣ�
		int nCastRet = PKMiscHelper::CastDataFromBin2ASCII(pServiceTag->szBinData, pServiceTag->nDataTypeLen, pServiceTag->nDataType, pModbusTag->szValue, sizeof(pModbusTag->szValue));
	}
	int nRet = m_pfnWriteTags(modbusTagVec);
	if(nRet != 0) // �����ȡʧ���򷵻ش���
		return MB_EIO;
	return MB_ENOERR;
}


static unsigned char auchCRCHi[] =
{
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
	0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
	0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
	0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
	0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
	0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
	0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
	0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
	0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
	0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
	0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
	0x40
} ;

static unsigned char auchCRCLo[] =
{
	0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4,
	0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
	0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD,
	0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
	0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7,
	0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
	0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE,
	0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
	0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2,
	0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
	0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB,
	0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
	0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0, 0x50, 0x90, 0x91,
	0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
	0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88,
	0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
	0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80,
	0x40
};

/******************************************************************************************************
* FunName : CRC16                                                                                                
* Function : The function returns the CRC16 as a unsigned short type
* Input    : puchMsg - message to calculate CRC upon; usDataLen - quantity of bytes in message
* Output   : CRC value
******************************************************************************************************/
unsigned short CRC16 ( unsigned char * puchMsg, unsigned short usDataLen )
{         
	unsigned char uchCRCHi = 0xFF ; /* high byte of CRC initialized */
	unsigned char uchCRCLo = 0xFF ; /* low byte of CRC initialized */
	unsigned uIndex ; /* will index into CRC lookup table */
	while (usDataLen--) /* pass through message buffer */
	{
		uIndex = uchCRCLo ^ *puchMsg++ ; /* calculate the CRC */
		uchCRCLo = uchCRCHi ^ auchCRCHi[uIndex] ;
		uchCRCHi = auchCRCLo[uIndex] ;
	}
	return (uchCRCHi << 8 | uchCRCLo) ;
} 
