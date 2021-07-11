/*
**	FILENAME			CSerialPort.cpp
**
**	PURPOSE				This class can read, write and watch one serial port.
**						It sends messages to its owner when something happends on the port
**						The class creates a thread for reading and writing so the main
**						program is not blocked.
**
**	CREATION DATE		15-09-1997
**	LAST MODIFICATION	12-11-1997
**
**	AUTHOR				Remon Spekreijse
**
**
*/
#include <assert.h>
#include "stdio.h"
#include <string>
#include "pkdriver/pkdrvcmn.h"
#include "common/CommHelper.h"
#include "time.h"
#include "string.h"
#include <ace/OS_NS_strings.h>
#include "SerialPort.h"
#include <map>
#include "ace/DEV_Connector.h"
#include "common/gettimeofday.h"
#include "pkcomm/pkcomm.h" 
#include "pklog/pklog.h" 
extern CPKLog g_logger;

using namespace std;

#define PK_SERIALPORT_FAIL		-101
#define MAX_READBUFFER_LEN		(1024 * 1024)
#define MAX_WRITEBUFFER_LEN		(1024 * 1024)



//extern void g_logger.LogMessage(int nLogLevel, const char *fmt,... );
// contruction and destruction
CSerialPort::CSerialPort()
{
	memset(m_szPortName, 0x00, sizeof(m_szPortName));
	m_nBandRate = DEFAULT_BAUDRATE;
	m_chParity = DEFAULT_PARITY;
	m_nDataBits = DEFAULT_DATABITS;
	m_nStopBits = DEFAULT_STOPBITS;
	m_bConnected = false;
}

CSerialPort::~CSerialPort()
{
	DisConnect();
}

long CSerialPort::Start()
{
	return CheckAndConnect();
}

long CSerialPort::Stop()
{
	return DisConnect();
}

/**
 *  Set Serial CheckAndConnect Parameters. 
 *  @guide
 *      serialport=COM1;baudrate=19200;parity=N;databits=8;stopbits=1
 *        serialport - 串口名称
 *        baudrate   - 波特率，如果对应系统支持，可设置以下数值
 *                     0,50,75,110,134,150,200,300,600,1200,1800,2400
 *                     4800,9600,19200,38400,57600,76800,115200,128000,
 *                     153600,230400,307200,256000,460800,500000,576000,
 *                     921600,1000000,1152000,1500000,2000000,2500000,
 *                     3000000,3500000,4000000
 *                     默认9600
 *        parity     - 校验模式，N(无校验),O(奇校验),E(偶校验)，默认N
 *        databits   - 数据位，取值范围[5,8]，默认8
 *        stopbits   - 停止位，取值[1,2]，默认1
 *  @param  -[in]  const ACE_TCHAR*  szFuncName: [function name]
 *  @param  -[in,out]  void**  pPFN: [function pointer]
 *
 *  @version     06/26/2008  chenzhiquan  Initial Version.
 */
long CSerialPort::SetConnectParam(char *szConnParam)
{
	string strPortName = "";
	int nBandRate = -10000;
	char chParity = 'N';
	int nDataBits = 8;
	int nStopBits = 1;

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "串口配置串:%s", szConnParam);

	string strConnParam = szConnParam;
	strConnParam += ";"; // 补上一个分号
	int nPos = strConnParam.find(';');
	while(nPos != string::npos)
	{
		// 处理从字符串开始到第一个';'的字符串
		string strOneParam = strConnParam.substr(0, nPos);
		// 除去这一个待解析的参数
		strConnParam = strConnParam.substr(nPos + 1);
		nPos = strConnParam.find(';');

		// 两个';'之间无字符，continue
		if(strOneParam.empty())
			continue;

		int nPosPart = strOneParam.find('=');
		// 没有'='，不符合参数规则，continue
		if(nPosPart == string::npos)
			continue;

		// 获取到某个参数名称和值
		string strParamName = strOneParam.substr(0, nPosPart); // e.g. parity
		string strParamValue = strOneParam.substr(nPosPart + 1); // e.g. N

		// 解析串口名称,com=3或者port=3
		if (ACE_OS::strcasecmp("port", strParamName.c_str()) == 0 || ACE_OS::strcasecmp("com", strParamName.c_str()) == 0)
		{
			strPortName = strParamValue;
		}
		// 解析波特率
		else if (ACE_OS::strcasecmp("baudrate", strParamName.c_str()) == 0 || strParamName.find("rate") != string::npos || strParamName.find("Rate") != string::npos)
		{
			nBandRate = ::atoi(strParamValue.c_str());
		}
		// 解析校验位
		else if(ACE_OS::strcasecmp("parity", strParamName.c_str()) == 0)
		{
			if(strParamValue.length() > 0)
			{
				chParity = strParamValue[0];
			}
			// 此处应该进行合法性校验
		}
		// 解析数据位
		else if(ACE_OS::strcasecmp("databits", strParamName.c_str()) == 0)
			nDataBits = ::atoi(strParamValue.c_str());
		// 解析停止位
		else if(ACE_OS::strcasecmp("stopbits", strParamName.c_str()) == 0)
			nStopBits = ::atoi(strParamValue.c_str());
	}

	// 设置连接参数
	if (strPortName.empty())
	{
		g_logger.LogMessage(PK_LOGLEVEL_WARN, "串口名称配置为空");
	}


#ifdef _WIN32
#define DEFAULT_SERIALPORT_PREFIX			"COM"
#define DEFAULT_SERIALPORT_PREFIX_LOWER		"com"
#define DEFAULT_SERIALPORT_BIGGER_PREFIX	"\\\\.\\COM"
#else
//#define DEFAULT_SERIALPORT_PREFIX			"tty"
//#define DEFAULT_SERIALPORT_PREFIX_LOWER		"TTY"
//#define DEFAULT_SERIALPORT_BIGGER_PREFIX	DEFAULT_SERIALPORT_PREFIX
#endif


#ifdef _WIN32
	string strOnlyPortNo; // 只有tty或COM后面的部分，如ttyO1的O1
	nPos = strPortName.find(DEFAULT_SERIALPORT_PREFIX);
	if (nPos != string::npos)
	{
		strOnlyPortNo = strPortName.substr(nPos);
	}
	else
		strOnlyPortNo = strPortName;

	int nPortNum = ::atoi(strOnlyPortNo.c_str()); // 串口号
	// windows下，如果COM端口大于9，则必须修改名字为"\\.\COMxx"
	if (nPortNum > 9)
	{
		sprintf(m_szPortName, "%s%d", DEFAULT_SERIALPORT_BIGGER_PREFIX, nPortNum);
	}
	else
		sprintf(m_szPortName, "%s%d", DEFAULT_SERIALPORT_PREFIX, nPortNum);
#else
	sprintf(m_szPortName, "%s", strPortName.c_str());
#endif

	// 此处应该进行合法性校验
	switch(nBandRate)
	{
	case 0:
	case 50:
	case 75:
	case 110:
	case 134:
	case 150:
	case 200:
	case 300:
	case 600:
	case 1200:
	case 1800:
	case 2400:
	case 4800:
	case 9600:
	case 19200:
	case 38400:
	case 57600:
	case 76800:
	case 115200:
	case 128000:
	case 153600:
	case 230400:
	case 307200:
	case 256000:
	case 460800:
	case 500000:
	case 576000:
	case 921600:
	case 1000000:
	case 1152000:
	case 1500000:
	case 2000000:
	case 2500000:
	case 3000000:
	case 3500000:
	case 4000000:
		m_nBandRate = nBandRate;
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "串口:%s, 波特率：%d", m_szPortName, m_nBandRate);
		break;
	default:
		m_nBandRate = DEFAULT_BAUDRATE;
		g_logger.LogMessage(PK_LOGLEVEL_WARN, "串口:%s, 解析波特率配置失败，设置为默认值%d", m_szPortName, m_nBandRate);
	}
	
	if (chParity == 'N' || chParity == 'n' || chParity == '0')
	{
		m_chParity = 'N';
	}
	else if (chParity == 'O' || chParity == 'o' || chParity == '1')
	{
		m_chParity = 'O';
	}
	else if (chParity == 'E' || chParity == 'e' || chParity == '2')
	{
		m_chParity = 'E';
	}
	else
	{
		m_chParity = DEFAULT_PARITY;
		g_logger.LogMessage(PK_LOGLEVEL_WARN, "串口:%s, 解析校验位配置失败，设置为默认值%c", m_szPortName, m_chParity);
	}

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "串口:%s, 校验位：%c,N表示无校验,E表示偶校验,O表示奇校验", m_szPortName, m_chParity);

	if (nDataBits>=5 && nDataBits<=8)
	{
		m_nDataBits = nDataBits;
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "串口:%s, 数据位：%d", m_szPortName, m_nDataBits);
	}
	else
	{
		m_nDataBits = DEFAULT_DATABITS;
		g_logger.LogMessage(PK_LOGLEVEL_WARN, "串口:%s, 解析数据位配置失败，设置为默认值%d", m_szPortName, m_nDataBits);
	}
	

	if (nStopBits == 1 || nStopBits == 2)
	{
		m_nStopBits = nStopBits;
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "串口:%s, 停止位：%d", m_szPortName, m_nStopBits);
	}
	else
	{
		m_nStopBits = DEFAULT_STOPBITS;
		g_logger.LogMessage(PK_LOGLEVEL_WARN, "串口:%s, 解析停止位配置失败，设置为默认值%d", m_szPortName, m_nStopBits);
	}
	
	m_tmLastConnect = 0; // 令马上可以进行连接
	return 0;
}

// 连接设备
long CSerialPort::CheckAndConnect()
{
	if (!GetEnableConnect())
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "因为设备:%s 被设置为禁止连接, 不能连接, 连接参数: %s", this->m_strConnName.c_str(), this->m_strConnParam.c_str());
		return -1;
	}

	if (IsConnected())
	{
		return PK_SUCCESS;
	}

	time_t tmNow;
	time(&tmNow);
	if(labs(tmNow - m_tmLastConnect) < 10)
		return -1; // 小于15秒则不需要重连

	m_tmLastConnect = tmNow; // 设置上次重连的时间

	ACE_DEV_Addr serialAddr(m_szPortName);
	ACE_DEV_Connector con;
	// 打开串口
	if (con.connect(m_ioHandle, serialAddr) == -1)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "设备:%s,打开串口 %s 失败", this->m_strConnName.c_str(), m_szPortName);
		return PK_SERIALPORT_FAIL;
	}

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "设备:%s,打开串口 %s 成功", this->m_strConnName.c_str(), m_szPortName);
	if (m_pfnConnCallback)
		m_pfnConnCallback(m_pConCallbackParam, false);

	// 设置串口参数
	ACE_TTY_IO::Serial_Params params;
	params.baudrate = m_nBandRate;
	params.databits = m_nDataBits;
	params.stopbits = m_nStopBits;
	// 下面需要设置，否则浦东机场导航技术室的测试设备的远程接口连不上
	params.xonlim = 0;
	params.xofflim = 0;
	params.readmincharacters = 0;
	params.readtimeoutmsec = 10*1000; // 10 seconds
	params.ctsenb = false;
	params.rtsenb = 1; // 这里必须1，如果为0的话浦东机场的新设备远程接口无法接收到数据，20160413，sjp
	params.xinenb = false;
	params.xoutenb = false;
	params.modem = false;
	params.rcvenb = true;
	params.dsrenb = false;
	params.dtrenb = 1;
	
	switch(m_chParity)
	{
	case 'N':
		params.paritymode = "none";
		break;
	case 'O':
		params.paritymode = "odd";
		break;
	case 'E':
		params.paritymode = "even";
		break;
	}

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "串口参数设置,奇偶校验:%s", params.paritymode);
	// 其它参数使用的默认值，可以不管，只注意设置超时参数
#ifdef _WIN32_TEMP
	// windows下，设置等待超时影响数据接收成功率，所以不等待
	params.readmincharacters = 0;
	params.readtimeoutmsec = 0;
#else //_WIN32
	// VMIN=0,VTIME>0时，read会传回读到的字元，或者等待VTIME后返回
	params.readmincharacters = 0;
	params.readtimeoutmsec = MIN_READ_TIMEOUT_MS;
#endif //_WIN32

	if (m_ioHandle.control(ACE_TTY_IO::SETPARAMS, &params) == -1)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "串口参数设置失败", m_szPortName);
		// 关闭串口
		m_ioHandle.close();
		return PK_SERIALPORT_FAIL; 
	}

	g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "打开串口 %s 成功", m_szPortName);
	m_bConnected = true;
	return 0;
}

// 断开连接
long CSerialPort::DisConnect()
{
	if (IsConnected())
	{
		m_ioHandle.close();
		//ClearDeviceRecvBuffer();
		m_bConnected = false; // 必须放在Clear的后面，否则Clear中会重新连接
		if (m_pfnConnCallback)
			m_pfnConnCallback(m_pConCallbackParam, false);
	}
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "断开串口 %s 连接", m_szPortName);
	m_tmLastConnect = 0;
	return 0;
}

bool CSerialPort::IsConnected()
{ 
	return m_bConnected;
}

long CSerialPort::Send(const char* szToWriteBuf, long lWriteBufLen, long lTimeOutMS)
{
	if (!GetEnableConnect()) // 如果禁止连接（维护模式）
		return -10000;

	// 如果重连失败，返回错误
	if (!IsConnected())
	{
		// 尝试重练一次
		CheckAndConnect();
		if (!IsConnected())
			return -1;
	}

	long lSendBytes = m_ioHandle.send_n(szToWriteBuf, lWriteBufLen);

	if (lSendBytes > 0)
	{
		unsigned int lTempLen = lWriteBufLen*3;
		char* pTemp = new char[lTempLen];
		memset(pTemp, 0x00, lTempLen);
		PKStringHelper::HexDumpBuf(szToWriteBuf, lWriteBufLen, pTemp, lTempLen, &lTempLen);

		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "发送长度为%d的电文:\n%s", lSendBytes, pTemp);
		delete[] pTemp;

	}
	else
	{
		if (lSendBytes == -1 && errno == ETIME)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "发送超时", lSendBytes);
		}
		else
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "发送失败，ret=%d, errno=%d", lSendBytes, errno);

			// 发生了未知异常（如机器休眠时），此时发送总是返回-1，需要断开重连
			DisConnect();
		}
	}

	return lSendBytes;
}

long CSerialPort::Recv(char *szReadBuf, long lMaxReadBufLen, long lTimeOutMS)
{
	if (!GetEnableConnect()) // 如果禁止连接（维护模式）
		return -10000;

	// 如果重连失败，返回错误
	if (!IsConnected())
	{
		CheckAndConnect();
		if (!IsConnected())
			return -1;
	}

	ACE_Time_Value tvTimeout(0, lTimeOutMS*1000);
	ACE_Time_Value tvAbsoluteTimeout = CVLibHead::gettimeofday_tickcnt() + tvTimeout;

	// 接收数据，直到读取了1字节，或者超时
	long lPreReadBytes = 1;  // 至少读取的字节数，用于阻塞等待
	long lReadBytes = 0;

	// 循环读取数据，每次的超时时间为MIN_READ_TIMEOUT_MS
	// 直到读取错误、读取到数据或者累计读取时间已超时
	do 
	{
		lReadBytes = m_ioHandle.recv(szReadBuf, lPreReadBytes);	
		// 如果绝对时间已超时，退出
		if (CVLibHead::gettimeofday_tickcnt() >= tvAbsoluteTimeout)
		{
			break;
		}
	} while (lReadBytes == 0); // 如果读取超时，继续读取

	if (lReadBytes > 0)
	{
		// 接收剩下的数据
		lReadBytes = m_ioHandle.recv(szReadBuf+lPreReadBytes, lMaxReadBufLen-lPreReadBytes);
		if (lReadBytes >= 0)
		{
			lReadBytes += lPreReadBytes;
			//unsigned int lTempLen = lReadBytes*3;
			//char* pTemp = new char[lTempLen];
			//memset(pTemp, 0x00, lTempLen);
			//ACE::format_hexdump(szReadBuf, lReadBytes, pTemp, lTempLen);

			//g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "接收长度为%d的电文:\n%s", lReadBytes, pTemp);
			//delete[] pTemp;
		}
		else
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "串口接收失败，ret=%d", lReadBytes);
		}
	}
	else if (lReadBytes == 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "接收超时，超时时间 %d ms", lTimeOutMS);
	}
	else
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "串口接收失败，ret=%d", lReadBytes);
	}
	return lReadBytes;



	// 循环测试超时，因为handle可能是Non-Block的，所以可能tvTimeout不起作用
	// 此时使用的超时时间是Connect()中的 params.readtimeoutmsec
	do 
	{
		lReadBytes = m_ioHandle.recv_n(szReadBuf, 1, &tvTimeout);
		// 如果handle是non-block的，需要累积等待直到超时
		if (lReadBytes == 0 && errno == EWOULDBLOCK &&  // EWOULDBLOCK表示io是non-block的
			tvAbsoluteTimeout > CVLibHead::gettimeofday_tickcnt()) // 绝对时间没有超时
		{
			continue;
		}
		else
			break;
	} while (1);

	if (lReadBytes <= 0)
	{
		if (errno == ETIME)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "CSerialPort::Recv接收超时，超时时间 %d ms", lTimeOutMS);
		}
		else
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "CSerialPort::Recv接收失败，ret=%d, errno=%d", lReadBytes, errno);
		}
		return lReadBytes;
	}

	// 接收剩下的数据
	lReadBytes = m_ioHandle.recv(szReadBuf+lPreReadBytes, lMaxReadBufLen-lPreReadBytes);
	lReadBytes += lPreReadBytes;

	if (lReadBytes >= 1)
	{
		unsigned int lTempLen = lReadBytes*3;
		char* pTemp = new char[lTempLen];
		memset(pTemp, 0x00, lTempLen);
		PKStringHelper::HexDumpBuf(szReadBuf, lReadBytes, pTemp, lTempLen, &lTempLen);

		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "CSerialPort::Recv接收长度为%d的电文:%s", lReadBytes, pTemp);
		delete[] pTemp;
	}
	else 
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "CSerialPort::Recv接收失败，ret=%d", lReadBytes-lPreReadBytes);
	}

	return lReadBytes;
}	

// 从端口读取缓冲区内容
long CSerialPort::Recv_n( char *szReadBuf, long lExactReadBufLen, long lTimeOutMS )
{
	// 如果重连失败，返回错误
	if (!IsConnected())
	{
		return -1;
	}

	ACE_Time_Value tvTimeout(0, lTimeOutMS*1000);
	ACE_Time_Value tvAbsoluteTimeout = CVLibHead::gettimeofday_tickcnt() + tvTimeout;
	// 接收数据，直到读取了lReadBufLen字节，或者超时
	long lReadBytes = 0;
	// 循环测试超时，因为handle可能是Non-Block的，所以可能tvTimeout不起作用
	// 此时使用的超时时间是Connect()中的 params.readtimeoutmsec
	do 
	{
		lReadBytes = m_ioHandle.recv_n(szReadBuf, lExactReadBufLen, &tvTimeout);
		// 如果handle是non-block的，需要累积等待直到超时
		if (lReadBytes == 0 && errno == EWOULDBLOCK &&  // EWOULDBLOCK表示io是non-block的
			tvAbsoluteTimeout > CVLibHead::gettimeofday_tickcnt()) // 绝对时间没有超时
			continue;
		else
			break;
	} while (1);


	if (lReadBytes > 0)
	{
		unsigned int lTempLen = lReadBytes*16;
		char* pTemp = new char[lTempLen];
		memset(pTemp, 0x00, lTempLen);
		PKStringHelper::HexDumpBuf(szReadBuf, lReadBytes, pTemp, lTempLen, &lTempLen);

		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "CSerialPort::RecvNBytes接收长度为%d的电文:\n%s", lReadBytes, pTemp);
		delete[] pTemp;
	}
	else 
	{
		if (lReadBytes == -1 && errno == ETIME)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "CSerialPort::RecvNBytes接收超时，超时时间 %d ms", lTimeOutMS);
		}
		else
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "CSerialPort::RecvNBytes接收失败，ret=%d, errno=%d", lReadBytes, errno);
		}
	}
	return lReadBytes;
}


void CSerialPort::ClearDeviceRecvBuffer()
{
	char szReadBuffer[1024] = {'\0'};
	long lRecvBytes = 0;
	while((lRecvBytes = Recv(szReadBuffer, sizeof(szReadBuffer), 1000)) > 0)
	{
		if(lRecvBytes < sizeof(szReadBuffer))
			break;
	}
}
