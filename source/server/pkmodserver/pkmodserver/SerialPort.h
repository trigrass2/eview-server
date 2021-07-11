/*
**	FILENAME			CSerialPort.h
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

#ifndef __SERIALPORT_H__
#define __SERIALPORT_H__


#define MAX_SERIAL_PORT_LEN 32
#define MAX_BUFFER_SIZE		1024*1024
#define DEFAULT_SERIAL_BLOCK_SIZE  1024
#define MIN_READ_TIMEOUT_MS	100
#define DEFAULT_BAUDRATE	9600
#define DEFAULT_PARITY		'N'
#define DEFAULT_DATABITS	8
#define DEFAULT_STOPBITS	1

#include "ace/TTY_IO.h"
#include "DeviceConnection.h"

class CSerialPort: public CDeviceConnection
{														 
public:
	// contruction and destruction
	CSerialPort();
	virtual		~CSerialPort();
	virtual long Start();
	virtual long Stop();
	virtual long SetConnectParam(char *szConnParam);
	// 连接设备
	virtual long CheckAndConnect();
	// 断开连接
	virtual long DisConnect();
	virtual bool IsConnected();

	virtual long Send(const char* szToWriteBuf, long lWriteBufLen, long lTimeOutMS);
	virtual long Recv(char *szReadBuf, long lMaxReadBufLen, long lTimeOutMS);	// 从端口读取缓冲区内容
	virtual long Recv_n(char *szReadBuf, long lExactReadBufLen, long lTimeOutMS);
	virtual void ClearDeviceRecvBuffer();
protected:
	unsigned int m_nBandRate;
	char			m_chParity;
	unsigned int	m_nDataBits;
	unsigned int	m_nStopBits;
	time_t m_tmLastConnect;

	char m_szPortName[MAX_SERIAL_PORT_LEN];
	ACE_TTY_IO m_ioHandle;
	bool m_bConnected;
};

#endif //__SERIALPORT_H__


