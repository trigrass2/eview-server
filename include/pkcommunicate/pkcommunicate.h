#ifndef _PK_PKCOMMNICATION_H_
#define _PK_PKCOMMNICATION_H_

#ifdef _WIN32
#define  PKDRIVER_EXPORTS extern "C" __declspec(dllexport)
#else //_WIN32
#define  PKDRIVER_EXPORTS __attribute__ ((visibility ("default")))
#endif //_WIN32

#define PK_COMMUNICATE_TYPE_NOTSUPPORT		0		// ��֧�ֵ�ͨѶ����, other
#define PK_COMMUNICATE_TYPE_TCPCLIENT		1		// TCP Client, ��Ϊtcpclientʱ, ConnParam:		ip=192.168.10.111;port=37777
#define PK_COMMUNICATE_TYPE_TCPSERVER		2		// TCP Server, ��Ϊtcpserverʱ, ConnParam:		listenport=37777;clientip=192.168.10.111,xx.xx.xx.xx,xx.xx.xx.xx,xx.xx.xx.xx
#define PK_COMMUNICATE_TYPE_UDP				3		// UDP, ��Ϊudpʱ,				ConnParam:		localport=NNN;ip=xx.xx.xx.xx;port=MMMM
#define PK_COMMUNICATE_TYPE_SERIAL			4		// ����, ��Ϊserialʱ,			ConnParam:		port=COM1;baudrate=19200;parity=N;databits=8;stopbits=1

typedef void * PKCONNECTHANDLE; 
typedef int(*PFN_OnConnChange)(PKCONNECTHANDLE hConnect, int bConnected, char *szConnExtra, int nCallbackParam, void *pCallbackParam); // szExtraΪ����������Ϣ����TCPServerʱ�Ŀͻ���IP

PKDRIVER_EXPORTS int	PKCommunicate_Init();// ��ʼ��ģ��
PKDRIVER_EXPORTS int	PKCommunicate_UnInit(); // ģ���ʼ������

//	nConnTypeֵΪ�����е�һ��:tcpclient/serial/udp/tcpserver, ������ֵ������궨�塣ConnParamֵ, ��ʽ�������ӷ�ʽ����, ������ע��
PKDRIVER_EXPORTS PKCONNECTHANDLE PKCommunicate_InitConn(int nConnType, const char *szConnParam, const char *szConnectName, PFN_OnConnChange pfnCallback, int nCallbackParam, void *pCallbackParam);// ��ʱ�Ὠ�������ӻ��ߴ������Ͽ�
PKDRIVER_EXPORTS void	PKCommunicate_UnInitConn(PKCONNECTHANDLE hConnect);
PKDRIVER_EXPORTS int	PKCommunicate_Send(PKCONNECTHANDLE hConnect, const char *szBuffer, int nBufLen, int nTimeOutMS);	// ���豸����nBufLen���ȵ����ݣ����ط��͵��ֽڸ���
PKDRIVER_EXPORTS int	PKCommunicate_Recv(PKCONNECTHANDLE hConnect, char *szBuffer, int nBufLen, int nTimeOutMS);	// ���豸�������nBufLen���ȵ����ݣ����ؽ��յ�ʵ���ֽڸ���
PKDRIVER_EXPORTS void	PKCommunicate_ClearRecvBuffer(PKCONNECTHANDLE hConnect);

PKDRIVER_EXPORTS int	PKCommunicate_Connect(PKCONNECTHANDLE hConnect, int nTimeOutMS);	// �������豸�����ӡ��ú���ͨ������Ҫ����
PKDRIVER_EXPORTS int	PKCommunicate_Disconnect(PKCONNECTHANDLE hConnect);	// �����Ͽ����豸������
PKDRIVER_EXPORTS int	PKCommunicate_IsConnected(PKCONNECTHANDLE hConnect); // ��ǰ�Ƿ�������״̬
PKDRIVER_EXPORTS int	PKCommunicate_EnableConnect(PKCONNECTHANDLE hConnect, int bEnableConnect); // �Ƿ���������
PKDRIVER_EXPORTS int	PKCommunicate_GetConnectType(PKCONNECTHANDLE hConnect); // ������������


#endif // _PK_PKCOM_HELPER_H_

