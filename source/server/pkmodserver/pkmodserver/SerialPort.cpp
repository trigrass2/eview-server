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
 *        serialport - ��������
 *        baudrate   - �����ʣ������Ӧϵͳ֧�֣�������������ֵ
 *                     0,50,75,110,134,150,200,300,600,1200,1800,2400
 *                     4800,9600,19200,38400,57600,76800,115200,128000,
 *                     153600,230400,307200,256000,460800,500000,576000,
 *                     921600,1000000,1152000,1500000,2000000,2500000,
 *                     3000000,3500000,4000000
 *                     Ĭ��9600
 *        parity     - У��ģʽ��N(��У��),O(��У��),E(żУ��)��Ĭ��N
 *        databits   - ����λ��ȡֵ��Χ[5,8]��Ĭ��8
 *        stopbits   - ֹͣλ��ȡֵ[1,2]��Ĭ��1
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

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "�������ô�:%s", szConnParam);

	string strConnParam = szConnParam;
	strConnParam += ";"; // ����һ���ֺ�
	int nPos = strConnParam.find(';');
	while(nPos != string::npos)
	{
		// ������ַ�����ʼ����һ��';'���ַ���
		string strOneParam = strConnParam.substr(0, nPos);
		// ��ȥ��һ���������Ĳ���
		strConnParam = strConnParam.substr(nPos + 1);
		nPos = strConnParam.find(';');

		// ����';'֮�����ַ���continue
		if(strOneParam.empty())
			continue;

		int nPosPart = strOneParam.find('=');
		// û��'='�������ϲ�������continue
		if(nPosPart == string::npos)
			continue;

		// ��ȡ��ĳ���������ƺ�ֵ
		string strParamName = strOneParam.substr(0, nPosPart); // e.g. parity
		string strParamValue = strOneParam.substr(nPosPart + 1); // e.g. N

		// ������������,com=3����port=3
		if (ACE_OS::strcasecmp("port", strParamName.c_str()) == 0 || ACE_OS::strcasecmp("com", strParamName.c_str()) == 0)
		{
			strPortName = strParamValue;
		}
		// ����������
		else if (ACE_OS::strcasecmp("baudrate", strParamName.c_str()) == 0 || strParamName.find("rate") != string::npos || strParamName.find("Rate") != string::npos)
		{
			nBandRate = ::atoi(strParamValue.c_str());
		}
		// ����У��λ
		else if(ACE_OS::strcasecmp("parity", strParamName.c_str()) == 0)
		{
			if(strParamValue.length() > 0)
			{
				chParity = strParamValue[0];
			}
			// �˴�Ӧ�ý��кϷ���У��
		}
		// ��������λ
		else if(ACE_OS::strcasecmp("databits", strParamName.c_str()) == 0)
			nDataBits = ::atoi(strParamValue.c_str());
		// ����ֹͣλ
		else if(ACE_OS::strcasecmp("stopbits", strParamName.c_str()) == 0)
			nStopBits = ::atoi(strParamValue.c_str());
	}

	// �������Ӳ���
	if (strPortName.empty())
	{
		g_logger.LogMessage(PK_LOGLEVEL_WARN, "������������Ϊ��");
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
	string strOnlyPortNo; // ֻ��tty��COM����Ĳ��֣���ttyO1��O1
	nPos = strPortName.find(DEFAULT_SERIALPORT_PREFIX);
	if (nPos != string::npos)
	{
		strOnlyPortNo = strPortName.substr(nPos);
	}
	else
		strOnlyPortNo = strPortName;

	int nPortNum = ::atoi(strOnlyPortNo.c_str()); // ���ں�
	// windows�£����COM�˿ڴ���9��������޸�����Ϊ"\\.\COMxx"
	if (nPortNum > 9)
	{
		sprintf(m_szPortName, "%s%d", DEFAULT_SERIALPORT_BIGGER_PREFIX, nPortNum);
	}
	else
		sprintf(m_szPortName, "%s%d", DEFAULT_SERIALPORT_PREFIX, nPortNum);
#else
	sprintf(m_szPortName, "%s", strPortName.c_str());
#endif

	// �˴�Ӧ�ý��кϷ���У��
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
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "����:%s, �����ʣ�%d", m_szPortName, m_nBandRate);
		break;
	default:
		m_nBandRate = DEFAULT_BAUDRATE;
		g_logger.LogMessage(PK_LOGLEVEL_WARN, "����:%s, ��������������ʧ�ܣ�����ΪĬ��ֵ%d", m_szPortName, m_nBandRate);
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
		g_logger.LogMessage(PK_LOGLEVEL_WARN, "����:%s, ����У��λ����ʧ�ܣ�����ΪĬ��ֵ%c", m_szPortName, m_chParity);
	}

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "����:%s, У��λ��%c,N��ʾ��У��,E��ʾżУ��,O��ʾ��У��", m_szPortName, m_chParity);

	if (nDataBits>=5 && nDataBits<=8)
	{
		m_nDataBits = nDataBits;
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "����:%s, ����λ��%d", m_szPortName, m_nDataBits);
	}
	else
	{
		m_nDataBits = DEFAULT_DATABITS;
		g_logger.LogMessage(PK_LOGLEVEL_WARN, "����:%s, ��������λ����ʧ�ܣ�����ΪĬ��ֵ%d", m_szPortName, m_nDataBits);
	}
	

	if (nStopBits == 1 || nStopBits == 2)
	{
		m_nStopBits = nStopBits;
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "����:%s, ֹͣλ��%d", m_szPortName, m_nStopBits);
	}
	else
	{
		m_nStopBits = DEFAULT_STOPBITS;
		g_logger.LogMessage(PK_LOGLEVEL_WARN, "����:%s, ����ֹͣλ����ʧ�ܣ�����ΪĬ��ֵ%d", m_szPortName, m_nStopBits);
	}
	
	m_tmLastConnect = 0; // �����Ͽ��Խ�������
	return 0;
}

// �����豸
long CSerialPort::CheckAndConnect()
{
	if (!GetEnableConnect())
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "��Ϊ�豸:%s ������Ϊ��ֹ����, ��������, ���Ӳ���: %s", this->m_strConnName.c_str(), this->m_strConnParam.c_str());
		return -1;
	}

	if (IsConnected())
	{
		return PK_SUCCESS;
	}

	time_t tmNow;
	time(&tmNow);
	if(labs(tmNow - m_tmLastConnect) < 10)
		return -1; // С��15������Ҫ����

	m_tmLastConnect = tmNow; // �����ϴ�������ʱ��

	ACE_DEV_Addr serialAddr(m_szPortName);
	ACE_DEV_Connector con;
	// �򿪴���
	if (con.connect(m_ioHandle, serialAddr) == -1)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "�豸:%s,�򿪴��� %s ʧ��", this->m_strConnName.c_str(), m_szPortName);
		return PK_SERIALPORT_FAIL;
	}

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "�豸:%s,�򿪴��� %s �ɹ�", this->m_strConnName.c_str(), m_szPortName);
	if (m_pfnConnCallback)
		m_pfnConnCallback(m_pConCallbackParam, false);

	// ���ô��ڲ���
	ACE_TTY_IO::Serial_Params params;
	params.baudrate = m_nBandRate;
	params.databits = m_nDataBits;
	params.stopbits = m_nStopBits;
	// ������Ҫ���ã������ֶ��������������ҵĲ����豸��Զ�̽ӿ�������
	params.xonlim = 0;
	params.xofflim = 0;
	params.readmincharacters = 0;
	params.readtimeoutmsec = 10*1000; // 10 seconds
	params.ctsenb = false;
	params.rtsenb = 1; // �������1�����Ϊ0�Ļ��ֶ����������豸Զ�̽ӿ��޷����յ����ݣ�20160413��sjp
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

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "���ڲ�������,��żУ��:%s", params.paritymode);
	// ��������ʹ�õ�Ĭ��ֵ�����Բ��ܣ�ֻע�����ó�ʱ����
#ifdef _WIN32_TEMP
	// windows�£����õȴ���ʱӰ�����ݽ��ճɹ��ʣ����Բ��ȴ�
	params.readmincharacters = 0;
	params.readtimeoutmsec = 0;
#else //_WIN32
	// VMIN=0,VTIME>0ʱ��read�ᴫ�ض�������Ԫ�����ߵȴ�VTIME�󷵻�
	params.readmincharacters = 0;
	params.readtimeoutmsec = MIN_READ_TIMEOUT_MS;
#endif //_WIN32

	if (m_ioHandle.control(ACE_TTY_IO::SETPARAMS, &params) == -1)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "���ڲ�������ʧ��", m_szPortName);
		// �رմ���
		m_ioHandle.close();
		return PK_SERIALPORT_FAIL; 
	}

	g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "�򿪴��� %s �ɹ�", m_szPortName);
	m_bConnected = true;
	return 0;
}

// �Ͽ�����
long CSerialPort::DisConnect()
{
	if (IsConnected())
	{
		m_ioHandle.close();
		//ClearDeviceRecvBuffer();
		m_bConnected = false; // �������Clear�ĺ��棬����Clear�л���������
		if (m_pfnConnCallback)
			m_pfnConnCallback(m_pConCallbackParam, false);
	}
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "�Ͽ����� %s ����", m_szPortName);
	m_tmLastConnect = 0;
	return 0;
}

bool CSerialPort::IsConnected()
{ 
	return m_bConnected;
}

long CSerialPort::Send(const char* szToWriteBuf, long lWriteBufLen, long lTimeOutMS)
{
	if (!GetEnableConnect()) // �����ֹ���ӣ�ά��ģʽ��
		return -10000;

	// �������ʧ�ܣ����ش���
	if (!IsConnected())
	{
		// ��������һ��
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

		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "���ͳ���Ϊ%d�ĵ���:\n%s", lSendBytes, pTemp);
		delete[] pTemp;

	}
	else
	{
		if (lSendBytes == -1 && errno == ETIME)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "���ͳ�ʱ", lSendBytes);
		}
		else
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "����ʧ�ܣ�ret=%d, errno=%d", lSendBytes, errno);

			// ������δ֪�쳣�����������ʱ������ʱ�������Ƿ���-1����Ҫ�Ͽ�����
			DisConnect();
		}
	}

	return lSendBytes;
}

long CSerialPort::Recv(char *szReadBuf, long lMaxReadBufLen, long lTimeOutMS)
{
	if (!GetEnableConnect()) // �����ֹ���ӣ�ά��ģʽ��
		return -10000;

	// �������ʧ�ܣ����ش���
	if (!IsConnected())
	{
		CheckAndConnect();
		if (!IsConnected())
			return -1;
	}

	ACE_Time_Value tvTimeout(0, lTimeOutMS*1000);
	ACE_Time_Value tvAbsoluteTimeout = CVLibHead::gettimeofday_tickcnt() + tvTimeout;

	// �������ݣ�ֱ����ȡ��1�ֽڣ����߳�ʱ
	long lPreReadBytes = 1;  // ���ٶ�ȡ���ֽ��������������ȴ�
	long lReadBytes = 0;

	// ѭ����ȡ���ݣ�ÿ�εĳ�ʱʱ��ΪMIN_READ_TIMEOUT_MS
	// ֱ����ȡ���󡢶�ȡ�����ݻ����ۼƶ�ȡʱ���ѳ�ʱ
	do 
	{
		lReadBytes = m_ioHandle.recv(szReadBuf, lPreReadBytes);	
		// �������ʱ���ѳ�ʱ���˳�
		if (CVLibHead::gettimeofday_tickcnt() >= tvAbsoluteTimeout)
		{
			break;
		}
	} while (lReadBytes == 0); // �����ȡ��ʱ��������ȡ

	if (lReadBytes > 0)
	{
		// ����ʣ�µ�����
		lReadBytes = m_ioHandle.recv(szReadBuf+lPreReadBytes, lMaxReadBufLen-lPreReadBytes);
		if (lReadBytes >= 0)
		{
			lReadBytes += lPreReadBytes;
			//unsigned int lTempLen = lReadBytes*3;
			//char* pTemp = new char[lTempLen];
			//memset(pTemp, 0x00, lTempLen);
			//ACE::format_hexdump(szReadBuf, lReadBytes, pTemp, lTempLen);

			//g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "���ճ���Ϊ%d�ĵ���:\n%s", lReadBytes, pTemp);
			//delete[] pTemp;
		}
		else
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "���ڽ���ʧ�ܣ�ret=%d", lReadBytes);
		}
	}
	else if (lReadBytes == 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "���ճ�ʱ����ʱʱ�� %d ms", lTimeOutMS);
	}
	else
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "���ڽ���ʧ�ܣ�ret=%d", lReadBytes);
	}
	return lReadBytes;



	// ѭ�����Գ�ʱ����Ϊhandle������Non-Block�ģ����Կ���tvTimeout��������
	// ��ʱʹ�õĳ�ʱʱ����Connect()�е� params.readtimeoutmsec
	do 
	{
		lReadBytes = m_ioHandle.recv_n(szReadBuf, 1, &tvTimeout);
		// ���handle��non-block�ģ���Ҫ�ۻ��ȴ�ֱ����ʱ
		if (lReadBytes == 0 && errno == EWOULDBLOCK &&  // EWOULDBLOCK��ʾio��non-block��
			tvAbsoluteTimeout > CVLibHead::gettimeofday_tickcnt()) // ����ʱ��û�г�ʱ
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
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "CSerialPort::Recv���ճ�ʱ����ʱʱ�� %d ms", lTimeOutMS);
		}
		else
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "CSerialPort::Recv����ʧ�ܣ�ret=%d, errno=%d", lReadBytes, errno);
		}
		return lReadBytes;
	}

	// ����ʣ�µ�����
	lReadBytes = m_ioHandle.recv(szReadBuf+lPreReadBytes, lMaxReadBufLen-lPreReadBytes);
	lReadBytes += lPreReadBytes;

	if (lReadBytes >= 1)
	{
		unsigned int lTempLen = lReadBytes*3;
		char* pTemp = new char[lTempLen];
		memset(pTemp, 0x00, lTempLen);
		PKStringHelper::HexDumpBuf(szReadBuf, lReadBytes, pTemp, lTempLen, &lTempLen);

		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "CSerialPort::Recv���ճ���Ϊ%d�ĵ���:%s", lReadBytes, pTemp);
		delete[] pTemp;
	}
	else 
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "CSerialPort::Recv����ʧ�ܣ�ret=%d", lReadBytes-lPreReadBytes);
	}

	return lReadBytes;
}	

// �Ӷ˿ڶ�ȡ����������
long CSerialPort::Recv_n( char *szReadBuf, long lExactReadBufLen, long lTimeOutMS )
{
	// �������ʧ�ܣ����ش���
	if (!IsConnected())
	{
		return -1;
	}

	ACE_Time_Value tvTimeout(0, lTimeOutMS*1000);
	ACE_Time_Value tvAbsoluteTimeout = CVLibHead::gettimeofday_tickcnt() + tvTimeout;
	// �������ݣ�ֱ����ȡ��lReadBufLen�ֽڣ����߳�ʱ
	long lReadBytes = 0;
	// ѭ�����Գ�ʱ����Ϊhandle������Non-Block�ģ����Կ���tvTimeout��������
	// ��ʱʹ�õĳ�ʱʱ����Connect()�е� params.readtimeoutmsec
	do 
	{
		lReadBytes = m_ioHandle.recv_n(szReadBuf, lExactReadBufLen, &tvTimeout);
		// ���handle��non-block�ģ���Ҫ�ۻ��ȴ�ֱ����ʱ
		if (lReadBytes == 0 && errno == EWOULDBLOCK &&  // EWOULDBLOCK��ʾio��non-block��
			tvAbsoluteTimeout > CVLibHead::gettimeofday_tickcnt()) // ����ʱ��û�г�ʱ
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

		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "CSerialPort::RecvNBytes���ճ���Ϊ%d�ĵ���:\n%s", lReadBytes, pTemp);
		delete[] pTemp;
	}
	else 
	{
		if (lReadBytes == -1 && errno == ETIME)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "CSerialPort::RecvNBytes���ճ�ʱ����ʱʱ�� %d ms", lTimeOutMS);
		}
		else
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "CSerialPort::RecvNBytes����ʧ�ܣ�ret=%d, errno=%d", lReadBytes, errno);
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
