#include "ace/ACE.h"
#include "ClientSockHandler.h"
#include "MainTask.h"
#include "pklog/pklog.h"
#include "SystemConfig.h"
//#include "common/Quality.h"
/* ----------------------- Modbus includes ----------------------------------*/
#include "../pkmodbustcplib/include/mb.h"
#include "../pkmodbustcplib/include/mbport.h"
#include "../pkmodbustcplib/include/mbutils.h"
extern CPKLog g_logger;

#define REG_INPUT_START 1
#define REG_INPUT_END 65536
#define REG_INPUT_PREFIX 300000
#define REG_HOLDING_START 1
#define REG_HOLDING_END 65536
#define REG_HOLDING_PREFIX 400000
#define REG_COILS_START 1
#define REG_COILS_END 65536
#define REG_COILS_PREFIX 0
#define REG_DISCRETE_START 1
#define REG_DISCRETE_END 65536
#define REG_DISCRETE_PREFIX 100000

#define REG_MAX_SIZE 1000

/* ----------------------- Static variables ---------------------------------*/
/*
static ULONG    ulRegInputStart = REG_INPUT_START;
static ULONG    ulRegInputEnd = REG_INPUT_END;
static ULONG	ulRegInputPrefix = REG_INPUT_PREFIX;
static ULONG    ulRegHoldingStart = REG_HOLDING_START;
static ULONG    ulRegHoldingEnd = REG_HOLDING_END;
static ULONG	ulRegHoldingPrefix = REG_HOLDING_PREFIX;
static ULONG    ulRegDiscreteStart = REG_DISCRETE_START;
static ULONG    ulRegDiscreteEnd = REG_DISCRETE_END;
static ULONG    ulRegDiscretePrefix = REG_DISCRETE_PREFIX;
*/

#define REQUESTCMD_SOCK_INPUT_SIZE		1024		// ÿ����������ĳ��Ȳ���̫��������1024�͹���

long	g_lMaxSessionID = 0;
/* An array of Modbus functions handlers which associates Modbus function
 * codes with implementing functions.
 */
xMBFunctionHandler g_xFuncHandlers[MB_FUNC_HANDLERS_MAX] = 
{
#if MB_FUNC_OTHER_REP_SLAVEID_ENABLED > 0
    {MB_FUNC_OTHER_REPORT_SLAVEID, eMBFuncReportSlaveID},
#endif
#if MB_FUNC_READ_INPUT_ENABLED > 0
    {MB_FUNC_READ_INPUT_REGISTER, eMBFuncReadInputRegister},
#endif
#if MB_FUNC_READ_HOLDING_ENABLED > 0
    {MB_FUNC_READ_HOLDING_REGISTER, eMBFuncReadHoldingRegister},
#endif
#if MB_FUNC_WRITE_MULTIPLE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_REGISTERS, eMBFuncWriteMultipleHoldingRegister},
#endif
#if MB_FUNC_WRITE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_REGISTER, eMBFuncWriteHoldingRegister},
#endif
#if MB_FUNC_READWRITE_HOLDING_ENABLED > 0
    {MB_FUNC_READWRITE_MULTIPLE_REGISTERS, eMBFuncReadWriteMultipleHoldingRegister},
#endif
#if MB_FUNC_READ_COILS_ENABLED > 0
    {MB_FUNC_READ_COILS, eMBFuncReadCoils},
#endif
#if MB_FUNC_WRITE_COIL_ENABLED > 0
    {MB_FUNC_WRITE_SINGLE_COIL, eMBFuncWriteCoil},
#endif
#if MB_FUNC_WRITE_MULTIPLE_COILS_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_COILS, eMBFuncWriteMultipleCoils},
#endif
#if MB_FUNC_READ_DISCRETE_INPUTS_ENABLED > 0
    {MB_FUNC_READ_DISCRETE_INPUTS, eMBFuncReadDiscreteInputs},
#endif
};

/**
 *  �ֽ�˳��ת��.
 *
 *  @param  -[in, out]  char*  pOrig: [comment]
 *  @param  -[in]  int  nLength: [comment]
 *
 *  @version     07/25/2008  shijunpu  Initial Version.
 */
long SwapByteOrder(char* pOrig, int nLength, int nWordBytes)
{
	int i = 0;
	int nSwapCount = 0;
	char chTemp;
	
	if (nWordBytes == 2)
	{
		nSwapCount = nLength / 2;
		for(i = 0; i < nSwapCount; i++)
		{
			chTemp = *pOrig;
			*pOrig = *(pOrig + 1);
			*(pOrig + 1) = chTemp;
			pOrig += 2;
		}
	}
	else if (nWordBytes == 4)
	{
		nSwapCount = nLength / 4;
		for(i = 0; i < nSwapCount; i++)
		{
			// ��0�͵�3���ֽڽ���
			chTemp = *(pOrig + 3);
			*(pOrig + 3) = *pOrig; 
			*pOrig = chTemp;

			// ��1�͵�2���ֽڽ���
			chTemp = *(pOrig + 2);
			*(pOrig + 2) = *(pOrig + 1);
			*(pOrig + 1) = chTemp;

			pOrig += 4;
		}
		// ����4���ֽڵĲ���
		if (nLength - nSwapCount * 4 == 2)
		{
			// ʣ�������ֽ�
			chTemp = *pOrig;
			*pOrig = *(pOrig + 1);
			*(pOrig + 1) = chTemp;
		}
		else if (nLength - nSwapCount * 4 == 3)
		{
			// ʣ�������ֽ�
			chTemp = *pOrig;
			*pOrig = *(pOrig + 2);
			*(pOrig + 2) = chTemp;
		}
	}
	return 0;
}

// ģ�����Ķ�д����������AO��AI��
// pucRegBuffer Ϊ���ص����ݻ�������ʼ��ַ, usAddress Ϊ��ʼ�Ĵ�����ַ, usNRegsΪ�Ĵ�������, nBlockType:AI/AO, eModeΪ����д
eMBErrorCode RWAnalogyBlockData(UCHAR * pucRegBuffer, USHORT nBeginRegisterNo, USHORT usNRegs, int nBlockType, eMBRegisterMode eMode )
{
	eMBErrorCode    eStatus = MB_EINVAL;
	if (usNRegs <= 0 || usNRegs > REG_MAX_SIZE)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Invalid count(count:%u)!", usNRegs);
		return MB_EINVAL;
	}

	if ( (nBeginRegisterNo < REG_HOLDING_START)|| (nBeginRegisterNo + usNRegs > REG_HOLDING_END))
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Invalid address(start:%u, count:%u)!", nBeginRegisterNo, usNRegs);
		eStatus = MB_ENOREG;
		return eStatus;
	}

	if(eMode != MB_REG_READ && eMode != MB_REG_WRITE)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "eMode = %d, not read or write(start:%u, count:%u)!", eMode, nBeginRegisterNo, usNRegs);
		eStatus = MB_ENOREG;
		return eStatus;
	}

	if(eMode == MB_REG_READ)
	{
		// ���㻺�����ĳ��ȣ���Ӧ�ö�ȡ�����ֽ���
		memset(pucRegBuffer, 0, usNRegs * 2);	// ����0
	}
	else
	{
		int a = 2;
	}

	// ��������ÿһ��tag�㣬��ȡtag�����ֵ����ȡ���˵Ļ���ֱ�ӿ�����Ŀ�껺����������һֱ��tag����ʼ��ַ����Ŀ��Ĵ����Ľ�����ַ!
	ModbusServerInfo *pForwardInfo = SYSTEM_CONFIG->m_pForwardInfo;
	unsigned short nCurRegisterNo = nBeginRegisterNo; // ��ǰ���ڲ����ļĴ�����ַ
	unsigned short nEndRegisterNo = nCurRegisterNo + usNRegs - 1; // Ҫ��д�����һ���Ĵ�����ַ
	UCHAR *szRegBufferBegin = pucRegBuffer;		// Ҫ��������Ŀ��Ĵ�����ʼ��ַ
	UCHAR *szRegBufferEnd = pucRegBuffer + usNRegs * 2 -1; // Ҫ��������Ŀ��Ĵ���������ַ
	// ����ǽ��ж�ȡ�����Ƚ�����������Ϊ0
	vector<ModbusServiceTag *> &vecServicTag = (nBlockType == MODBUS_REGISTERTYPE_AO ? pForwardInfo->m_vecAOAddrToDit: pForwardInfo->m_vecAIAddrToDit);

	// ���ҳ����е�tag�㣬�Ա���������
	ModbusServiceTagVec modbusServiceTags;
	ModbusTagVec modbusTags;
	vector<ModbusServiceTag *>::iterator itServiceTag = vecServicTag.begin(); // �ǰ���ÿ��tag���Ӧ����ʼ��ַ����ġ�
	for(; itServiceTag !=  vecServicTag.end(); itServiceTag ++)
	{
		ModbusServiceTag *pModbusServiceTag = *itServiceTag;
		unsigned short nTagStartRegisterNo = pModbusServiceTag->nStartRegisterNo;	// ���ڵ���ʼ����λ
		unsigned short nTagEndRegisterNo = pModbusServiceTag->nEndRegisterNo; // TAG��Ӧ�Ľ����Ĵ���
		// tag����ʼ�Ĵ���������Ĵ����� Ŀ�귵�ؼĴ�������ʼ������Χ�ڣ���Ҫ��ȡ��ֵ
		if(nTagStartRegisterNo >= nBeginRegisterNo && nTagStartRegisterNo <= nEndRegisterNo || nTagEndRegisterNo >= nBeginRegisterNo && nTagEndRegisterNo <= nEndRegisterNo) 
		{
			modbusTags.push_back(pModbusServiceTag->pModbusTag);
			modbusServiceTags.push_back(pModbusServiceTag);
			// ����ǿ��ƣ���Ҫ��ֵ���ο�����ÿ��tag����
			if(eMode == MB_REG_WRITE) // ���ƵĻ�������tag��ʼ��ַ�ͽ�����ַ���ڼĴ���ֵ�ڣ�������ư�������
			{
				if(nTagStartRegisterNo >= nBeginRegisterNo && nTagEndRegisterNo <= nEndRegisterNo)
				{
					memset(pModbusServiceTag->szBinData, 0, pModbusServiceTag->nDataTypeLen); // ����0
					int nTagRegisterOffset = nTagStartRegisterNo - nBeginRegisterNo;
					int nCpyOffsetByte = nTagRegisterOffset * 2;
					int nCpyLen = pModbusServiceTag->nDataTypeLen;  // tag��ĳ���
					if(nCpyOffsetByte + nCpyLen > usNRegs * 2) // tag�㳤�Ȳ��ܴ��ڿ��õ����ݳ��ȣ�
						nCpyLen = usNRegs * 2 - nCpyOffsetByte;
					if(nCpyLen > 0)
						memcpy(pModbusServiceTag->szBinData, (char *)pucRegBuffer + nCpyOffsetByte, nCpyLen);
					else
					{
						g_logger.LogMessage(PK_LOGLEVEL_ERROR, "tag:%s, ��ʼ�Ĵ���:%d,���ƻ��������õļĴ�����Ŀ��%d, tag��Ҫ���������ݳ��ȳ����Ĵ�������", 
							pModbusServiceTag->pModbusTag->szName, nTagStartRegisterNo, usNRegs);
					}
				}
			}
		}

	}

	if(modbusTags.size() <= 0) // Ҫ��ȡ�ļĴ�������������һ��tag��
	{
        const char *szRWMode = eMode == MB_REG_READ?"��ȡ":"д��";
        const char *szRegisterType = nBlockType == MODBUS_REGISTERTYPE_AO ?"AO":"AI";
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "�Ĵ��� %s%s(startaddr:%u, registernum:%u)��Χû�ҵ����õ��κ�ƽ̨tag��!", szRWMode, szRegisterType, nBeginRegisterNo, usNRegs);
		return MB_EIO;
	}
	
	if(eMode == MB_REG_WRITE)
	{
		int nRet = MAIN_TASK->WriteTags(modbusTags);
		if(nRet != 0) // �����ȡʧ���򷵻ش���
			return MB_EIO;
		return MB_ENOERR;
	}

	int nRet = MAIN_TASK->ReadTags(modbusTags);
	if(nRet != 0) // �����ȡʧ���򷵻ش���
		return MB_EIO;

	// ��ȡ�ɹ�����ô���θ���
	itServiceTag = modbusServiceTags.begin();
	for(; itServiceTag != modbusServiceTags.end(); itServiceTag ++)
	{
		ModbusServiceTag *pModbusServiceTag = *itServiceTag;
		// ������������ֵ���Ĵ�������
		int nCpyLen = pModbusServiceTag->nDataTypeLen;
		int nCpyStartBytes = (pModbusServiceTag->nStartRegisterNo - nBeginRegisterNo) * 2; // ������ʼ�Ĵ����Ĳ�ֵ
		char *pSrcBuffBegin = pModbusServiceTag->szBinData;
		if(nCpyStartBytes < 0) // ���tag�����ʼ��ַ���ڼĴ�����ַ����ô��Ҫ�ƶ�Щ
		{
			pSrcBuffBegin -= nCpyStartBytes;
			nCpyStartBytes = 0;
			nCpyLen += nCpyStartBytes;
		}
		char *szDestBuff = (char *)pucRegBuffer + nCpyStartBytes;
		if(szDestBuff + nCpyLen > (char *)szRegBufferEnd) // �����ķ�Χ����
			nCpyLen = (char *)szRegBufferEnd - szDestBuff; // ֻ�ܿ����������ٵĻ�����
		memcpy(szDestBuff, pSrcBuffBegin, pModbusServiceTag->nDataTypeLen);
	}

	return MB_ENOERR; // ִ�гɹ�		
}

// ����Ĵ�������AI�Ķ�д��������
eMBErrorCode eMBRegInputCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs )
{
	eMBErrorCode eCode = RWAnalogyBlockData(pucRegBuffer, usAddress, usNRegs, MODBUS_REGISTERTYPE_AI, MB_REG_READ);
	// Э�鷢�͹�������BigEndian��������Ҫת��Ϊlittle endian
	SwapByteOrder((char *)pucRegBuffer, usNRegs*2, 2);
	return eCode;
}

// ���ּĴ�������AO�Ķ�д��������
eMBErrorCode eMBRegHoldingCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs, eMBRegisterMode eMode )
{
	// Э�鷢�͹�������BigEndian��������Ҫת��Ϊlittle endian
	if(eMode == MB_REG_WRITE)	// д֮ǰ�����ֽ��򽻻�
		SwapByteOrder((char *)pucRegBuffer, usNRegs*2, 2);

	eMBErrorCode eCode = RWAnalogyBlockData(pucRegBuffer, usAddress, usNRegs, MODBUS_REGISTERTYPE_AO, eMode);
	if(eMode == MB_REG_READ)
		SwapByteOrder((char *)pucRegBuffer, usNRegs*2, 2);
	return eCode;
}

//  ����һЩλ
void BitCopy(char *szDest, char *szSrc, unsigned short nDestStartBit, unsigned short nSrcStartBit, unsigned short nBitNum)
{
	// �ȿ����������ֽ�
	int i =  0;
	for(; i < nBitNum/8; i ++)
	{
		unsigned short uBitsVal = xMBUtilGetBits((unsigned char *)szSrc, nSrcStartBit,8);
		xMBUtilSetBits((unsigned char *)szDest, nDestStartBit, 8, uBitsVal);
		nDestStartBit += 8;
		nSrcStartBit += 8;
	}
	// ����ʣ���С��8��λ��
	int nRd = nBitNum % 8;
	if(nRd > 0)
	{
		unsigned short uBitsVal = xMBUtilGetBits((unsigned char *)szSrc, nSrcStartBit,nRd);
		xMBUtilSetBits((unsigned char *)szDest, nDestStartBit, nRd, uBitsVal);
	}
}

// �������Ķ�д����������DO��DI��
eMBErrorCode RWDigitalBlockData( UCHAR * pucRegBuffer, USHORT nBeginRegisterNo, USHORT usNCoils, int nBlockType, eMBRegisterMode eMode )
{
    eMBErrorCode    eStatus = MB_EINVAL;
	if (usNCoils <= 0 || usNCoils > REG_MAX_SIZE)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Invalid count(count:%u)!", usNCoils);
		return MB_EINVAL;
	}
	
	if ( (nBeginRegisterNo < REG_COILS_START)|| (nBeginRegisterNo + usNCoils > REG_COILS_END))
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Invalid address(start:%u, count:%u)!", nBeginRegisterNo, usNCoils);
		eStatus = MB_ENOREG;
		return eStatus;
	}
	if(eMode != MB_REG_READ && eMode != MB_REG_WRITE)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "eMode = %d, not read or write(start:%u, count:%u)!", eMode, nBeginRegisterNo, usNCoils);
		eStatus = MB_ENOREG;
		return eStatus;
	}

	// ����ǽ��ж�ȡ�����Ƚ�����������Ϊ0
	if(eMode == MB_REG_READ)
	{
		// ���㻺�����ĳ��ȣ���Ӧ�ö�ȡ�����ֽ���
		int nLengthBytes = (nBeginRegisterNo + usNCoils - REG_COILS_START) / 8 - (nBeginRegisterNo - REG_COILS_START)/8 + 1;	// ���صĶ���д�������ܳ��ȣ��ֽ�Ϊ��λ
		memset(pucRegBuffer, 0, nLengthBytes);	// ����0
	}
	else
	{
		int a = 1;
	}

	// ��������ÿһ��tag�㣬��ȡtag�����ֵ����ȡ���˵Ļ���ֱ�ӿ�����Ŀ�껺����������һֱ��tag����ʼ��ַ����Ŀ��Ĵ����Ľ�����ַ!
	ModbusServerInfo *pForwardInfo = SYSTEM_CONFIG->m_pForwardInfo;
	unsigned short nEndRegisterNo = nBeginRegisterNo + usNCoils - 1; // Ҫ��д�����һ���Ĵ�����ַ
	UCHAR *szRegBufferBegin = pucRegBuffer;		// Ҫ��������Ŀ��Ĵ�����ʼ��ַ
	UCHAR *szRegBufferEnd = pucRegBuffer + usNCoils/8 -1; // Ҫ��������Ŀ��Ĵ���������ַ
	// ����ǽ��ж�ȡ�����Ƚ�����������Ϊ0
	vector<ModbusServiceTag *> &vecServiceTag = (nBlockType == MODBUS_REGISTERTYPE_DO ? pForwardInfo->m_vecDOAddrToDit: pForwardInfo->m_vecDIAddrToDit);

	// ���ҳ����е�tag�㣬�Ա���������
	ModbusServiceTagVec modbusServiceTags;
	ModbusTagVec modbusTags;
	vector<ModbusServiceTag *>::iterator itServiceTag = vecServiceTag.begin(); // �ǰ���ÿ��tag���Ӧ����ʼ��ַ����ġ�
	for(; itServiceTag !=  vecServiceTag.end(); itServiceTag ++)
	{
		ModbusServiceTag *pModbusServiceTag = *itServiceTag;
		char *szTagField = pModbusServiceTag->pModbusTag->szName;
		unsigned short nTagStartRegisterNo = pModbusServiceTag->nStartRegisterNo;	// ���ڵ���ʼ����λ
		unsigned short nTagEndRegisterNo = pModbusServiceTag->nEndRegisterNo; // TAG��Ӧ�Ľ����Ĵ���
		// tag����ʼ�Ĵ���������Ĵ����� Ŀ�귵�ؼĴ�������ʼ������Χ�ڣ���Ҫ��ȡ��ֵ
		if(nTagStartRegisterNo >= nBeginRegisterNo && nTagStartRegisterNo <= nEndRegisterNo || nTagEndRegisterNo >= nBeginRegisterNo && nTagEndRegisterNo <= nEndRegisterNo) 
		{
			modbusServiceTags.push_back(pModbusServiceTag);
			modbusTags.push_back(pModbusServiceTag->pModbusTag);
			if(eMode == MB_REG_WRITE ) // ���ƵĻ�������tag��ʼ��ַ�ͽ�����ַ���ڼĴ���ֵ�ڣ�������ư�������
			{
				if(nTagStartRegisterNo >= nBeginRegisterNo && nTagEndRegisterNo <= nEndRegisterNo)
				{
					// ����ǿ��ƣ���Ҫ��ֵ���ο�����ÿ��tag����
					memset(pModbusServiceTag->szBinData, 0, pModbusServiceTag->nDataTypeLen); // ����0
					// �����Ĵ�������ֵ��tag
					int nSrcStartBits = (pModbusServiceTag->nStartRegisterNo - nBeginRegisterNo); // ������ʼ�Ĵ����Ĳ�ֵ
					int nSrcStartByte = nSrcStartBits / 8;
					int nSrcStartBit = nSrcStartBits % 8;
					// ÿ�����ǿ���1λ����
					BitCopy(pModbusServiceTag->szBinData, (char *)pucRegBuffer + nSrcStartByte,  0, nSrcStartBit, 1); // ����һЩλ��tag,���ǿ�������0λ�����ǿ���1λ
				}
			}
		}
	}

	if(modbusTags.size() <= 0) // Ҫ��ȡ�ļĴ�������������һ��tag��
	{
        const char *szRWMode = eMode == MB_REG_READ?"��ȡ":"д��";
        const char *szRegisterType = nBlockType == MODBUS_REGISTERTYPE_DO ?"DO":"DI";
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "�Ĵ��� %s%s(startaddr:%u, registernum:%u)��Χû�ҵ����õ��κ�ƽ̨tag��!", szRWMode, szRegisterType, nBeginRegisterNo, usNCoils);
		return MB_EIO;
	}

	if(eMode == MB_REG_WRITE)
	{
		int nRet = MAIN_TASK->WriteTags(modbusTags);
		if(nRet != 0) // �����ȡʧ���򷵻ش���
			return MB_EIO;
		return MB_ENOERR;
	}

	int nRet = MAIN_TASK->ReadTags(modbusTags);
	if(nRet != 0) // �����ȡʧ���򷵻ش���
		return MB_EIO;

	// ��ȡ�ɹ�����ô���θ���
	itServiceTag = modbusServiceTags.begin();
	for(; itServiceTag != modbusServiceTags.end(); itServiceTag ++)
	{
		ModbusServiceTag *pModbusServiceTag = *itServiceTag;
		// ������������ֵ���Ĵ�������
		int nCpyLenBits = 1; // ÿ�����ǿ���1λ����
		int nDestStartBits = (pModbusServiceTag->nStartRegisterNo - nBeginRegisterNo); // ������ʼ�Ĵ����Ĳ�ֵ
		int nDestStartByte = nDestStartBits / 8;
		int nDestStartBit = nDestStartBits % 8;
		// ÿ�����ǿ���1λ����
		BitCopy((char *)pucRegBuffer + nDestStartByte, pModbusServiceTag->szBinData, nDestStartBit, 0, 1); // ����һЩλ��Ŀ�껺����
	}

	return MB_ENOERR; // ִ�гɹ�
}

// ��Ȧ�Ĵ�������DO�Ķ�д��������
eMBErrorCode eMBRegCoilsCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNCoils, eMBRegisterMode eMode )
{
	return RWDigitalBlockData(pucRegBuffer, usAddress, usNCoils, MODBUS_REGISTERTYPE_DO, eMode);
}

// ��ɢ�Ĵ�������DI�Ķ�д��������
eMBErrorCode eMBRegDiscreteCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNDiscrete )
{
	return RWDigitalBlockData(pucRegBuffer, usAddress, usNDiscrete, MODBUS_REGISTERTYPE_DI, MB_REG_READ);
}

CClientSockHandler::CClientSockHandler(ACE_Reactor *r)
{
	m_nPeerPort = 0;
	m_strPeerIP = "";

	m_usTCPBufPos = 0;
	m_usTCPFrameBytesLeft = MB_TCP_FUNC;
	memset(m_aucTCPBuf, 0x00, MB_TCP_BUF_SIZE);
	m_usLength = 0;
}

CClientSockHandler::~CClientSockHandler()
{
}

// ���пͻ�������ʱ
int CClientSockHandler::open( void* p /*= 0*/ )
{
	if (super::open (p) == -1)
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "Accept a link from, super::open (p) failed!");
		return -1;	
	}

	ACE_INET_Addr addr;
	if (this->peer().get_remote_addr (addr) != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "this->peer().get_remote_addr (addr) failed!");
		return -1;
	}

	// ��ȡ�Զ˵�IP��Port��Ϣ
	m_strPeerIP = addr.get_host_addr();
	m_nPeerPort = addr.get_port_number();
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "Accept a link from client(%s, %d)!", m_strPeerIP.c_str(), m_nPeerPort);

	return 0;
}

// ���յ�socket���ݵ���Ϣ������
int CClientSockHandler::handle_input( ACE_HANDLE fd /*= ACE_INVALID_HANDLE*/ )
{
	char szRecvBuf[REQUESTCMD_SOCK_INPUT_SIZE] = {0};
	char *pRecvBuf = szRecvBuf;

	//g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "handle_input 1");
	// ��������
	ssize_t nRecvBytes = this->peer().recv(m_aucTCPBuf + m_usTCPBufPos, m_usTCPFrameBytesLeft);
	if (nRecvBytes <= 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "���յ�(%s:%d)���������Ϊ %d!", m_strPeerIP.c_str(), m_nPeerPort, nRecvBytes);
		return -1; // �Ͽ����ӡ���Ϊ������Ͽ��Ļ���handle_input�᲻�ϵ���
	}

	//g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "handle_input 2");
	// g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "���յ�(%s:%d)���������Ϊ %d!", m_strPeerIP.c_str(), m_nPeerPort, nRecvBytes);
	m_usTCPBufPos += nRecvBytes;
	m_usTCPFrameBytesLeft -= nRecvBytes;
	if (m_usTCPBufPos >= MB_TCP_FUNC)
	{
		/* Length is a byte count of Modbus PDU (function code + data) and the
		* unit identifier. */
		m_usLength = m_aucTCPBuf[MB_TCP_LEN] << 8U;
		m_usLength |= m_aucTCPBuf[MB_TCP_LEN + 1];
				
		/* Is the frame is not complete. */
		if( m_usTCPBufPos < ( MB_TCP_UID + m_usLength ) ) // ���������İ�����Ҫ���㻹��Ҫ���յĳ���
		{
			m_usTCPFrameBytesLeft = m_usLength + MB_TCP_UID - m_usTCPBufPos;
		}
		/* The frame is complete. */
		else if( m_usTCPBufPos == ( MB_TCP_UID + m_usLength ) )
		{
			UCHAR ucRcvAddress;
			UCHAR *ucMBFrame;
			USHORT usLength;
			eMBErrorCode eStatus = MB_ENOERR;

			eStatus = MBTCPReceive(&ucRcvAddress, &ucMBFrame, &usLength);
			if (eStatus == MB_ENOERR)
			{
				eStatus = MBTCPExecute(ucRcvAddress, ucMBFrame, usLength);
				if (eStatus != MB_ENOERR)
					g_logger.LogMessage(PK_LOGLEVEL_ERROR, "WorkerThread(%d) MBTCPExecute failed(%d).", (int)this->thr_mgr_->thr_self(), eStatus);
			}
			else
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "WorkerThread(%d) MBTCPReceive failed(%d).", (int)this->thr_mgr_->thr_self(), eStatus);
			
			// ����Ϊֹ�����Ѿ���������!!��Ҫ��ʼ��������ֵ
			m_usTCPBufPos = 0;
			m_usTCPFrameBytesLeft = MB_TCP_FUNC;
			memset(m_aucTCPBuf, 0x00, MB_TCP_BUF_SIZE);
			m_usLength = 0;
		}
		/* This can not happend because we always calculate the number of bytes
		* to receive. */
		else
		{
			assert ( m_usTCPBufPos <= ( MB_TCP_UID + m_usLength ) );
		}
	}
	
	//g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "handle_input 3");
	return 0;
}

// �������ݰ�ʱ����
int CClientSockHandler::handle_output( ACE_HANDLE fd /*= ACE_INVALID_HANDLE*/ )
{
	return 0;
}

// ���ӶϿ�ʱ����
int CClientSockHandler::handle_close( ACE_HANDLE handle, ACE_Reactor_Mask close_mask )
{
	m_usTCPBufPos = 0;
	m_usTCPFrameBytesLeft = MB_TCP_FUNC;
	memset(m_aucTCPBuf, 0x00, MB_TCP_BUF_SIZE);
	m_usLength = 0;

	g_logger.LogMessage(PK_LOGLEVEL_INFO, "Client(%s, %d) disconnected!", m_strPeerIP.c_str(), m_nPeerPort);
	// delete this;
	return 0;	
}

// ��ȡ��8���ֽڿ�ʼ�Ļ����������ܳ���-7��ǰ7���ǣ������2��Э���2������2��վ��1
eMBErrorCode CClientSockHandler::MBTCPReceive(UCHAR * pucRcvAddress, UCHAR ** ppucFrame, USHORT * pusLength)
{
	eMBErrorCode eStatus = MB_EIO;
    USHORT usPID;

	if (pucRcvAddress == NULL || ppucFrame == NULL || pusLength == NULL)
		return eStatus;

	usPID = m_aucTCPBuf[MB_TCP_PID] << 8U;
	usPID |= m_aucTCPBuf[MB_TCP_PID + 1];
	
	if( usPID == MB_TCP_PROTOCOL_ID )
	{	
		*ppucFrame = &m_aucTCPBuf[MB_TCP_FUNC];
		*pusLength = m_usTCPBufPos - MB_TCP_FUNC;
        eStatus = MB_ENOERR;
		
		/* Modbus TCP does not use any addresses. Fake the source address such
		* that the processing part deals with this frame.
		*/
		*pucRcvAddress = MB_TCP_PSEUDO_ADDRESS;		
	}
	return eStatus;
}

// ִ��modbus��������������
eMBErrorCode CClientSockHandler::MBTCPExecute( UCHAR ucRcvAddress, UCHAR * pucFrame, USHORT usLength )
{
	eMBErrorCode    eStatus = MB_ENOERR;
	UCHAR ucFunctionCode = pucFrame[MB_PDU_FUNC_OFF];
    m_eException = MB_EX_ILLEGAL_FUNCTION;
    for( int i = 0; i < MB_FUNC_HANDLERS_MAX; i++ )
    {
		/* No more function handlers registered. Abort. */
        if( g_xFuncHandlers[i].ucFunctionCode == 0 )
        {
			break;
        }
        else if( g_xFuncHandlers[i].ucFunctionCode == ucFunctionCode )
        {
			m_eException = g_xFuncHandlers[i].pxHandler( pucFrame, &usLength );
            break;
        }
	}
	
    /* If the request was not sent to the broadcast address we
    * return a reply. */
    if( ucRcvAddress != MB_ADDRESS_BROADCAST )
    {
        if( m_eException != MB_EX_NONE )
        {
			/* An exception occured. Build an error frame. */
            usLength = 0;
            pucFrame[usLength++] = ( UCHAR )( ucFunctionCode | MB_FUNC_ERROR );
            pucFrame[usLength++] = m_eException;
        }       
		
		UCHAR *pucMBTCPFrame = ( UCHAR * ) pucFrame - MB_TCP_FUNC;
		USHORT usTCPLength = usLength + MB_TCP_FUNC;
		
		/* The MBAP header is already initialized because the caller calls this
		* function with the buffer returned by the previous call. Therefore we 
		* only have to update the length in the header. Note that the length 
		* header includes the size of the Modbus PDU and the UID Byte. Therefore 
		* the length is usLength plus one.
		*/
		pucMBTCPFrame[MB_TCP_LEN] = ( usLength + 1 ) >> 8U;
		pucMBTCPFrame[MB_TCP_LEN + 1] = ( usLength + 1 ) & 0xFF;
		if( MBTCPPortSendResponse( pucMBTCPFrame, usTCPLength ) == FALSE )
		{
			eStatus = MB_EIO;
		}	
	}
	return eStatus;
}

// ����modbustcpӦ���������
BOOL CClientSockHandler::MBTCPPortSendResponse( const UCHAR *pucMBTCPFrame, USHORT usTCPLength )
{
	int nRet = 0;
    BOOL bFrameSent = FALSE;
    BOOL bAbort = FALSE;
	int nByteSent = 0;
    int nTimeOut = MB_TCP_READ_TIMEOUT;
	do
	{
		nRet = this->peer().send_n(pucMBTCPFrame, usTCPLength); // ���ͷ��ؽ����
		switch (nRet)
		{
		case -1:
			if (nTimeOut > 0)
			{
				nTimeOut -= MB_TCP_READ_CYCLE;
				ACE_Time_Value tv(0, MB_TCP_READ_CYCLE);
				ACE_OS::sleep(tv);
				g_logger.LogMessage(PK_LOGLEVEL_INFO, "WorkerThread(%d) sends not completed.", (int)this->thr_mgr_->thr_self());
			}
			else
			{
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "WorkerThread(%d) sending timeout.", (int)this->thr_mgr_->thr_self());
				bAbort = TRUE;
			}
			break;
		case 0:
			g_logger.LogMessage(PK_LOGLEVEL_INFO, "WorkerThread(%d) sent 0 byte.", (int)this->thr_mgr_->thr_self());
			bAbort = TRUE;
			break;
		default:
			nByteSent += nRet;
			break;
		}
	}while(nByteSent != usTCPLength && !bAbort);

	bFrameSent = nByteSent == usTCPLength ? TRUE : FALSE;

	return bFrameSent;
}
