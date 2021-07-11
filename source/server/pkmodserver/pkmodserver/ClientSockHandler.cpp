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

#define REQUESTCMD_SOCK_INPUT_SIZE		1024		// 每次请求命令的长度不会太长，所以1024就够了

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
 *  字节顺序转换.
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
			// 第0和第3个字节交换
			chTemp = *(pOrig + 3);
			*(pOrig + 3) = *pOrig; 
			*pOrig = chTemp;

			// 第1和第2个字节交换
			chTemp = *(pOrig + 2);
			*(pOrig + 2) = *(pOrig + 1);
			*(pOrig + 1) = chTemp;

			pOrig += 4;
		}
		// 不足4个字节的部分
		if (nLength - nSwapCount * 4 == 2)
		{
			// 剩余两个字节
			chTemp = *pOrig;
			*pOrig = *(pOrig + 1);
			*(pOrig + 1) = chTemp;
		}
		else if (nLength - nSwapCount * 4 == 3)
		{
			// 剩余三个字节
			chTemp = *pOrig;
			*pOrig = *(pOrig + 2);
			*(pOrig + 2) = chTemp;
		}
	}
	return 0;
}

// 模拟量的读写请求处理函数（AO、AI）
// pucRegBuffer 为返回的数据缓冲区起始地址, usAddress 为起始寄存器地址, usNRegs为寄存器个数, nBlockType:AI/AO, eMode为读或写
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
		// 计算缓冲区的长度，即应该读取到的字节数
		memset(pucRegBuffer, 0, usNRegs * 2);	// 先清0
	}
	else
	{
		int a = 2;
	}

	// 方法：对每一个tag点，读取tag点的数值，读取到了的话则直接拷贝到目标缓冲区中区，一直到tag的起始地址大于目标寄存器的结束地址!
	ModbusServerInfo *pForwardInfo = SYSTEM_CONFIG->m_pForwardInfo;
	unsigned short nCurRegisterNo = nBeginRegisterNo; // 当前正在操作的寄存器地址
	unsigned short nEndRegisterNo = nCurRegisterNo + usNRegs - 1; // 要读写的最后一个寄存器地址
	UCHAR *szRegBufferBegin = pucRegBuffer;		// 要拷贝到的目标寄存器起始地址
	UCHAR *szRegBufferEnd = pucRegBuffer + usNRegs * 2 -1; // 要拷贝到的目标寄存器结束地址
	// 如果是进行读取，就先将缓冲区都置为0
	vector<ModbusServiceTag *> &vecServicTag = (nBlockType == MODBUS_REGISTERTYPE_AO ? pForwardInfo->m_vecAOAddrToDit: pForwardInfo->m_vecAIAddrToDit);

	// 先找出所有的tag点，以便批量查找
	ModbusServiceTagVec modbusServiceTags;
	ModbusTagVec modbusTags;
	vector<ModbusServiceTag *>::iterator itServiceTag = vecServicTag.begin(); // 是按照每个tag点对应的起始地址排序的。
	for(; itServiceTag !=  vecServicTag.end(); itServiceTag ++)
	{
		ModbusServiceTag *pModbusServiceTag = *itServiceTag;
		unsigned short nTagStartRegisterNo = pModbusServiceTag->nStartRegisterNo;	// 块内的起始拷贝位
		unsigned short nTagEndRegisterNo = pModbusServiceTag->nEndRegisterNo; // TAG对应的结束寄存器
		// tag点起始寄存器或结束寄存器在 目标返回寄存器的起始结束范围内，就要获取其值
		if(nTagStartRegisterNo >= nBeginRegisterNo && nTagStartRegisterNo <= nEndRegisterNo || nTagEndRegisterNo >= nBeginRegisterNo && nTagEndRegisterNo <= nEndRegisterNo) 
		{
			modbusTags.push_back(pModbusServiceTag->pModbusTag);
			modbusServiceTags.push_back(pModbusServiceTag);
			// 如果是控制，需要将值依次拷贝到每个tag点中
			if(eMode == MB_REG_WRITE) // 控制的话，必须tag起始地址和结束地址都在寄存器值内！否则控制半个会出错！
			{
				if(nTagStartRegisterNo >= nBeginRegisterNo && nTagEndRegisterNo <= nEndRegisterNo)
				{
					memset(pModbusServiceTag->szBinData, 0, pModbusServiceTag->nDataTypeLen); // 先清0
					int nTagRegisterOffset = nTagStartRegisterNo - nBeginRegisterNo;
					int nCpyOffsetByte = nTagRegisterOffset * 2;
					int nCpyLen = pModbusServiceTag->nDataTypeLen;  // tag点的长度
					if(nCpyOffsetByte + nCpyLen > usNRegs * 2) // tag点长度不能大于可用的数据长度！
						nCpyLen = usNRegs * 2 - nCpyOffsetByte;
					if(nCpyLen > 0)
						memcpy(pModbusServiceTag->szBinData, (char *)pucRegBuffer + nCpyOffsetByte, nCpyLen);
					else
					{
						g_logger.LogMessage(PK_LOGLEVEL_ERROR, "tag:%s, 起始寄存器:%d,控制缓冲区可用的寄存器数目：%d, tag需要拷贝的数据长度超出寄存器长度", 
							pModbusServiceTag->pModbusTag->szName, nTagStartRegisterNo, usNRegs);
					}
				}
			}
		}

	}

	if(modbusTags.size() <= 0) // 要获取的寄存器不包含任意一个tag点
	{
        const char *szRWMode = eMode == MB_REG_READ?"读取":"写入";
        const char *szRegisterType = nBlockType == MODBUS_REGISTERTYPE_AO ?"AO":"AI";
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "寄存器 %s%s(startaddr:%u, registernum:%u)范围没找到配置的任何平台tag点!", szRWMode, szRegisterType, nBeginRegisterNo, usNRegs);
		return MB_EIO;
	}
	
	if(eMode == MB_REG_WRITE)
	{
		int nRet = MAIN_TASK->WriteTags(modbusTags);
		if(nRet != 0) // 如果读取失败则返回错误
			return MB_EIO;
		return MB_ENOERR;
	}

	int nRet = MAIN_TASK->ReadTags(modbusTags);
	if(nRet != 0) // 如果读取失败则返回错误
		return MB_EIO;

	// 读取成功，那么依次复制
	itServiceTag = modbusServiceTags.begin();
	for(; itServiceTag != modbusServiceTags.end(); itServiceTag ++)
	{
		ModbusServiceTag *pModbusServiceTag = *itServiceTag;
		// 拷贝缓冲区的值到寄存器中区
		int nCpyLen = pModbusServiceTag->nDataTypeLen;
		int nCpyStartBytes = (pModbusServiceTag->nStartRegisterNo - nBeginRegisterNo) * 2; // 两个起始寄存器的差值
		char *pSrcBuffBegin = pModbusServiceTag->szBinData;
		if(nCpyStartBytes < 0) // 如果tag点的起始地址大于寄存器地址，那么需要移动些
		{
			pSrcBuffBegin -= nCpyStartBytes;
			nCpyStartBytes = 0;
			nCpyLen += nCpyStartBytes;
		}
		char *szDestBuff = (char *)pucRegBuffer + nCpyStartBytes;
		if(szDestBuff + nCpyLen > (char *)szRegBufferEnd) // 拷贝的范围超了
			nCpyLen = (char *)szRegBufferEnd - szDestBuff; // 只能拷贝尽可能少的缓冲区
		memcpy(szDestBuff, pSrcBuffBegin, pModbusServiceTag->nDataTypeLen);
	}

	return MB_ENOERR; // 执行成功		
}

// 输入寄存器，即AI的读写请求处理函数
eMBErrorCode eMBRegInputCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs )
{
	eMBErrorCode eCode = RWAnalogyBlockData(pucRegBuffer, usAddress, usNRegs, MODBUS_REGISTERTYPE_AI, MB_REG_READ);
	// 协议发送过来的是BigEndian，我们需要转换为little endian
	SwapByteOrder((char *)pucRegBuffer, usNRegs*2, 2);
	return eCode;
}

// 保持寄存器，即AO的读写请求处理函数
eMBErrorCode eMBRegHoldingCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs, eMBRegisterMode eMode )
{
	// 协议发送过来的是BigEndian，我们需要转换为little endian
	if(eMode == MB_REG_WRITE)	// 写之前进行字节序交换
		SwapByteOrder((char *)pucRegBuffer, usNRegs*2, 2);

	eMBErrorCode eCode = RWAnalogyBlockData(pucRegBuffer, usAddress, usNRegs, MODBUS_REGISTERTYPE_AO, eMode);
	if(eMode == MB_REG_READ)
		SwapByteOrder((char *)pucRegBuffer, usNRegs*2, 2);
	return eCode;
}

//  拷贝一些位
void BitCopy(char *szDest, char *szSrc, unsigned short nDestStartBit, unsigned short nSrcStartBit, unsigned short nBitNum)
{
	// 先拷贝整数个字节
	int i =  0;
	for(; i < nBitNum/8; i ++)
	{
		unsigned short uBitsVal = xMBUtilGetBits((unsigned char *)szSrc, nSrcStartBit,8);
		xMBUtilSetBits((unsigned char *)szDest, nDestStartBit, 8, uBitsVal);
		nDestStartBit += 8;
		nSrcStartBit += 8;
	}
	// 拷贝剩余的小于8的位数
	int nRd = nBitNum % 8;
	if(nRd > 0)
	{
		unsigned short uBitsVal = xMBUtilGetBits((unsigned char *)szSrc, nSrcStartBit,nRd);
		xMBUtilSetBits((unsigned char *)szDest, nDestStartBit, nRd, uBitsVal);
	}
}

// 数字量的读写请求处理函数（DO、DI）
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

	// 如果是进行读取，就先将缓冲区都置为0
	if(eMode == MB_REG_READ)
	{
		// 计算缓冲区的长度，即应该读取到的字节数
		int nLengthBytes = (nBeginRegisterNo + usNCoils - REG_COILS_START) / 8 - (nBeginRegisterNo - REG_COILS_START)/8 + 1;	// 返回的读或写的数据总长度，字节为单位
		memset(pucRegBuffer, 0, nLengthBytes);	// 先清0
	}
	else
	{
		int a = 1;
	}

	// 方法：对每一个tag点，读取tag点的数值，读取到了的话则直接拷贝到目标缓冲区中区，一直到tag的起始地址大于目标寄存器的结束地址!
	ModbusServerInfo *pForwardInfo = SYSTEM_CONFIG->m_pForwardInfo;
	unsigned short nEndRegisterNo = nBeginRegisterNo + usNCoils - 1; // 要读写的最后一个寄存器地址
	UCHAR *szRegBufferBegin = pucRegBuffer;		// 要拷贝到的目标寄存器起始地址
	UCHAR *szRegBufferEnd = pucRegBuffer + usNCoils/8 -1; // 要拷贝到的目标寄存器结束地址
	// 如果是进行读取，就先将缓冲区都置为0
	vector<ModbusServiceTag *> &vecServiceTag = (nBlockType == MODBUS_REGISTERTYPE_DO ? pForwardInfo->m_vecDOAddrToDit: pForwardInfo->m_vecDIAddrToDit);

	// 先找出所有的tag点，以便批量查找
	ModbusServiceTagVec modbusServiceTags;
	ModbusTagVec modbusTags;
	vector<ModbusServiceTag *>::iterator itServiceTag = vecServiceTag.begin(); // 是按照每个tag点对应的起始地址排序的。
	for(; itServiceTag !=  vecServiceTag.end(); itServiceTag ++)
	{
		ModbusServiceTag *pModbusServiceTag = *itServiceTag;
		char *szTagField = pModbusServiceTag->pModbusTag->szName;
		unsigned short nTagStartRegisterNo = pModbusServiceTag->nStartRegisterNo;	// 块内的起始拷贝位
		unsigned short nTagEndRegisterNo = pModbusServiceTag->nEndRegisterNo; // TAG对应的结束寄存器
		// tag点起始寄存器或结束寄存器在 目标返回寄存器的起始结束范围内，就要获取其值
		if(nTagStartRegisterNo >= nBeginRegisterNo && nTagStartRegisterNo <= nEndRegisterNo || nTagEndRegisterNo >= nBeginRegisterNo && nTagEndRegisterNo <= nEndRegisterNo) 
		{
			modbusServiceTags.push_back(pModbusServiceTag);
			modbusTags.push_back(pModbusServiceTag->pModbusTag);
			if(eMode == MB_REG_WRITE ) // 控制的话，必须tag起始地址和结束地址都在寄存器值内！否则控制半个会出错！
			{
				if(nTagStartRegisterNo >= nBeginRegisterNo && nTagEndRegisterNo <= nEndRegisterNo)
				{
					// 如果是控制，需要将值依次拷贝到每个tag点中
					memset(pModbusServiceTag->szBinData, 0, pModbusServiceTag->nDataTypeLen); // 先清0
					// 拷贝寄存器区的值到tag
					int nSrcStartBits = (pModbusServiceTag->nStartRegisterNo - nBeginRegisterNo); // 两个起始寄存器的差值
					int nSrcStartByte = nSrcStartBits / 8;
					int nSrcStartBit = nSrcStartBits % 8;
					// 每次总是拷贝1位数据
					BitCopy(pModbusServiceTag->szBinData, (char *)pucRegBuffer + nSrcStartByte,  0, nSrcStartBit, 1); // 拷贝一些位到tag,总是拷贝到第0位，总是拷贝1位
				}
			}
		}
	}

	if(modbusTags.size() <= 0) // 要获取的寄存器不包含任意一个tag点
	{
        const char *szRWMode = eMode == MB_REG_READ?"读取":"写入";
        const char *szRegisterType = nBlockType == MODBUS_REGISTERTYPE_DO ?"DO":"DI";
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "寄存器 %s%s(startaddr:%u, registernum:%u)范围没找到配置的任何平台tag点!", szRWMode, szRegisterType, nBeginRegisterNo, usNCoils);
		return MB_EIO;
	}

	if(eMode == MB_REG_WRITE)
	{
		int nRet = MAIN_TASK->WriteTags(modbusTags);
		if(nRet != 0) // 如果读取失败则返回错误
			return MB_EIO;
		return MB_ENOERR;
	}

	int nRet = MAIN_TASK->ReadTags(modbusTags);
	if(nRet != 0) // 如果读取失败则返回错误
		return MB_EIO;

	// 读取成功，那么依次复制
	itServiceTag = modbusServiceTags.begin();
	for(; itServiceTag != modbusServiceTags.end(); itServiceTag ++)
	{
		ModbusServiceTag *pModbusServiceTag = *itServiceTag;
		// 拷贝缓冲区的值到寄存器中区
		int nCpyLenBits = 1; // 每次总是拷贝1位数据
		int nDestStartBits = (pModbusServiceTag->nStartRegisterNo - nBeginRegisterNo); // 两个起始寄存器的差值
		int nDestStartByte = nDestStartBits / 8;
		int nDestStartBit = nDestStartBits % 8;
		// 每次总是拷贝1位数据
		BitCopy((char *)pucRegBuffer + nDestStartByte, pModbusServiceTag->szBinData, nDestStartBit, 0, 1); // 拷贝一些位到目标缓冲区
	}

	return MB_ENOERR; // 执行成功
}

// 线圈寄存器，即DO的读写请求处理函数
eMBErrorCode eMBRegCoilsCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNCoils, eMBRegisterMode eMode )
{
	return RWDigitalBlockData(pucRegBuffer, usAddress, usNCoils, MODBUS_REGISTERTYPE_DO, eMode);
}

// 离散寄存器，即DI的读写请求处理函数
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

// 当有客户端连接时
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

	// 获取对端的IP和Port信息
	m_strPeerIP = addr.get_host_addr();
	m_nPeerPort = addr.get_port_number();
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "Accept a link from client(%s, %d)!", m_strPeerIP.c_str(), m_nPeerPort);

	return 0;
}

// 接收到socket数据的消息处理函数
int CClientSockHandler::handle_input( ACE_HANDLE fd /*= ACE_INVALID_HANDLE*/ )
{
	char szRecvBuf[REQUESTCMD_SOCK_INPUT_SIZE] = {0};
	char *pRecvBuf = szRecvBuf;

	//g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "handle_input 1");
	// 接收数据
	ssize_t nRecvBytes = this->peer().recv(m_aucTCPBuf + m_usTCPBufPos, m_usTCPFrameBytesLeft);
	if (nRecvBytes <= 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "接收到(%s:%d)请求命令长度为 %d!", m_strPeerIP.c_str(), m_nPeerPort, nRecvBytes);
		return -1; // 断开连接。因为如果不断开的话，handle_input会不断调用
	}

	//g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "handle_input 2");
	// g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "接收到(%s:%d)请求命令长度为 %d!", m_strPeerIP.c_str(), m_nPeerPort, nRecvBytes);
	m_usTCPBufPos += nRecvBytes;
	m_usTCPFrameBytesLeft -= nRecvBytes;
	if (m_usTCPBufPos >= MB_TCP_FUNC)
	{
		/* Length is a byte count of Modbus PDU (function code + data) and the
		* unit identifier. */
		m_usLength = m_aucTCPBuf[MB_TCP_LEN] << 8U;
		m_usLength |= m_aucTCPBuf[MB_TCP_LEN + 1];
				
		/* Is the frame is not complete. */
		if( m_usTCPBufPos < ( MB_TCP_UID + m_usLength ) ) // 不是完整的包，需要计算还需要接收的长度
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
			
			// 至此为止，包已经处理完了!!需要初始化变脸的值
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

// 发送数据包时调用
int CClientSockHandler::handle_output( ACE_HANDLE fd /*= ACE_INVALID_HANDLE*/ )
{
	return 0;
}

// 连接断开时调用
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

// 获取第8个字节开始的缓冲区、及总长度-7。前7个是：事物号2、协议号2、长度2、站号1
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

// 执行modbus各类请求的总入口
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

// 发送modbustcp应答给请求者
BOOL CClientSockHandler::MBTCPPortSendResponse( const UCHAR *pucMBTCPFrame, USHORT usTCPLength )
{
	int nRet = 0;
    BOOL bFrameSent = FALSE;
    BOOL bAbort = FALSE;
	int nByteSent = 0;
    int nTimeOut = MB_TCP_READ_TIMEOUT;
	do
	{
		nRet = this->peer().send_n(pucMBTCPFrame, usTCPLength); // 发送返回结果先
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
