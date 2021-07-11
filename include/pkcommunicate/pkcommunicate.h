#ifndef _PK_PKCOMMNICATION_H_
#define _PK_PKCOMMNICATION_H_

#ifdef _WIN32
#define  PKDRIVER_EXPORTS extern "C" __declspec(dllexport)
#else //_WIN32
#define  PKDRIVER_EXPORTS __attribute__ ((visibility ("default")))
#endif //_WIN32

#define PK_COMMUNICATE_TYPE_NOTSUPPORT		0		// 不支持的通讯类型, other
#define PK_COMMUNICATE_TYPE_TCPCLIENT		1		// TCP Client, 当为tcpclient时, ConnParam:		ip=192.168.10.111;port=37777
#define PK_COMMUNICATE_TYPE_TCPSERVER		2		// TCP Server, 当为tcpserver时, ConnParam:		listenport=37777;clientip=192.168.10.111,xx.xx.xx.xx,xx.xx.xx.xx,xx.xx.xx.xx
#define PK_COMMUNICATE_TYPE_UDP				3		// UDP, 当为udp时,				ConnParam:		localport=NNN;ip=xx.xx.xx.xx;port=MMMM
#define PK_COMMUNICATE_TYPE_SERIAL			4		// 串口, 当为serial时,			ConnParam:		port=COM1;baudrate=19200;parity=N;databits=8;stopbits=1

typedef void * PKCONNECTHANDLE; 
typedef int(*PFN_OnConnChange)(PKCONNECTHANDLE hConnect, int bConnected, char *szConnExtra, int nCallbackParam, void *pCallbackParam); // szExtra为其他附加信息，如TCPServer时的客户端IP

PKDRIVER_EXPORTS int	PKCommunicate_Init();// 初始化模块
PKDRIVER_EXPORTS int	PKCommunicate_UnInit(); // 模块初始化结束

//	nConnType值为下面中的一种:tcpclient/serial/udp/tcpserver, 具体数值见上面宏定义。ConnParam值, 格式根据连接方式而定, 见上面注释
PKDRIVER_EXPORTS PKCONNECTHANDLE PKCommunicate_InitConn(int nConnType, const char *szConnParam, const char *szConnectName, PFN_OnConnChange pfnCallback, int nCallbackParam, void *pCallbackParam);// 此时会建立起连接或者打开侦听断开
PKDRIVER_EXPORTS void	PKCommunicate_UnInitConn(PKCONNECTHANDLE hConnect);
PKDRIVER_EXPORTS int	PKCommunicate_Send(PKCONNECTHANDLE hConnect, const char *szBuffer, int nBufLen, int nTimeOutMS);	// 向设备发送nBufLen长度的内容，返回发送的字节个数
PKDRIVER_EXPORTS int	PKCommunicate_Recv(PKCONNECTHANDLE hConnect, char *szBuffer, int nBufLen, int nTimeOutMS);	// 从设备接收最多nBufLen长度的内容，返回接收的实际字节个数
PKDRIVER_EXPORTS void	PKCommunicate_ClearRecvBuffer(PKCONNECTHANDLE hConnect);

PKDRIVER_EXPORTS int	PKCommunicate_Connect(PKCONNECTHANDLE hConnect, int nTimeOutMS);	// 主动与设备的连接。该函数通常不需要调用
PKDRIVER_EXPORTS int	PKCommunicate_Disconnect(PKCONNECTHANDLE hConnect);	// 主动断开与设备的连接
PKDRIVER_EXPORTS int	PKCommunicate_IsConnected(PKCONNECTHANDLE hConnect); // 当前是否处于连接状态
PKDRIVER_EXPORTS int	PKCommunicate_EnableConnect(PKCONNECTHANDLE hConnect, int bEnableConnect); // 是否允许连接
PKDRIVER_EXPORTS int	PKCommunicate_GetConnectType(PKCONNECTHANDLE hConnect); // 返回连接类型


#endif // _PK_PKCOM_HELPER_H_

