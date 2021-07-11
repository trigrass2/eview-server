#ifndef _PK_PKCOM_HELPER_H_
#define _PK_PKCOM_HELPER_H_

#ifdef _WIN32
#define  PKDRIVER_EXPORTS extern "C" __declspec(dllexport)
#else //_WIN32
#define  PKDRIVER_EXPORTS extern "C"
#endif //_WIN32

typedef void * PKCONNECTHANDLE;
/*
	connparam值为下面中的一种:tcpclient/serial/udp
	connParam值，根据连接方式而定
	tcpclient时:ip=192.168.10.111;port=37777
	serial时：port=COM1;baudrate=19200;parity=N;databits=8;stopbits=1
	udp时：localport=M;ip=xx.xx.xx.xx;port=N
*/

PKDRIVER_EXPORTS PKCONNECTHANDLE PKCom_Init(const char *szConnType, const char *szConnParam, const char *szConnectName);// 初始化连接.szConnName仅做打印log用
PKDRIVER_EXPORTS void	PKCom_UnInit(PKCONNECTHANDLE hConnect);
PKDRIVER_EXPORTS int	PKCom_Send(PKCONNECTHANDLE hConnect, const char *szBuffer, int nBufLen, int nTimeOutMS);	// 向设备发送nBufLen长度的内容，返回发送的字节个数
PKDRIVER_EXPORTS int	PKCom_Recv(PKCONNECTHANDLE hConnect, char *szBuffer, int nBufLen, int nTimeOutMS);	// 从设备接收最多nBufLen长度的内容，返回接收的实际字节个数
PKDRIVER_EXPORTS void	PKCom_ClearRecvBuffer(PKCONNECTHANDLE hConnect);
#endif // _PK_PKCOM_HELPER_H_

