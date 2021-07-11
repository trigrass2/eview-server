#ifndef _PK_PKCOM_HELPER_H_
#define _PK_PKCOM_HELPER_H_

#ifdef _WIN32
#define  PKDRIVER_EXPORTS extern "C" __declspec(dllexport)
#else //_WIN32
#define  PKDRIVER_EXPORTS extern "C"
#endif //_WIN32

typedef void * PKCONNECTHANDLE;
/*
	connparamֵΪ�����е�һ��:tcpclient/serial/udp
	connParamֵ���������ӷ�ʽ����
	tcpclientʱ:ip=192.168.10.111;port=37777
	serialʱ��port=COM1;baudrate=19200;parity=N;databits=8;stopbits=1
	udpʱ��localport=M;ip=xx.xx.xx.xx;port=N
*/

PKDRIVER_EXPORTS PKCONNECTHANDLE PKCom_Init(const char *szConnType, const char *szConnParam, const char *szConnectName);// ��ʼ������.szConnName������ӡlog��
PKDRIVER_EXPORTS void	PKCom_UnInit(PKCONNECTHANDLE hConnect);
PKDRIVER_EXPORTS int	PKCom_Send(PKCONNECTHANDLE hConnect, const char *szBuffer, int nBufLen, int nTimeOutMS);	// ���豸����nBufLen���ȵ����ݣ����ط��͵��ֽڸ���
PKDRIVER_EXPORTS int	PKCom_Recv(PKCONNECTHANDLE hConnect, char *szBuffer, int nBufLen, int nTimeOutMS);	// ���豸�������nBufLen���ȵ����ݣ����ؽ��յ�ʵ���ֽڸ���
PKDRIVER_EXPORTS void	PKCom_ClearRecvBuffer(PKCONNECTHANDLE hConnect);
#endif // _PK_PKCOM_HELPER_H_

