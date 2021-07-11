#ifndef _PK_DRVCOMMON_H_
#define _PK_DRVCOMMON_H_

#include <string.h>
#include <iostream>
#include <iomanip>
#include "eviewcomm/eviewcomm.h"
using namespace std;

#ifdef _WIN32
#define  PKDRIVER_EXPORTS extern "C" __declspec(dllexport)
#else //LINUX,UNIX
#define  PKDRIVER_EXPORTS extern "C" __attribute__ ((visibility ("default")))
#endif //LINUX,UNIX

typedef struct _PKTAG
{
	char	*szName;		//�������
	char	*szAddress;		//��ĵ�ַ����eg.400001#10 or 400001:1.������豸���ݵ�ַҲ���������snmp��int32��uint32��counter32��gauge32��timeticks ��octet��oid
	
	int		nDataType;		//�����������
	int		nDataLen;		// ����tag���ֵ�ĳ��ȣ���nLenBits��ͬ��nLenBits��ʵ�ʳ��ȣ�������tag�洢���������͵ĳ��ȣ���λ���ֽڡ�����Ǹ����������Ͷ������������в��ܱ��޸ģ�
	int		nByteOrder;		// ���ڼ�¼��ֵ���͵Ľ�����ʽ��44332211,11223344��
	int		nPollRate;		//���ɨ������
	char	*szParam;		// ���ܴ洢վ�ŵ�, �������ж�ȡ

	// ���������������в����޸�, ����ı������������п����޸�
	int		nStartBit;		// ��������ʹ�ã����ڴ洢�ñ����������ݿ�Ŀ���ƫ����ʼλ
	int		nEndBit;		// ��������ʹ�ã����ڴ洢�ñ����������ݿ�Ŀ��ڽ���λ
	int		nLenBits;		// �ַ���/blob�ĳ���,bitΪ��λ, �����õ�tag�����������ȷ����ʼֵ�����ڰ��ռ�λȡ�ĵ�ַ��:40001:4-7���ó��Ȼ��ڽ���ʱ���ı�

	// �������ʱ��ֵ��������ʱ�����ֵ�����ͣ��ַ������Ǿ������ͣ�
	char	*szData;		// ����tag���ֵ, �����Ƕ���������, Ҳ������ת��Ϊascii���ֵ, ����ΪnDataLen+1
	int		nQuality;
	unsigned int  nTimeSec;	// 1970�꿪ʼ��ʱ���,����,time_t, ʵ����unsigned int, 4���ֽ�
	unsigned short nTimeMilSec; // ���ڵĺ�����,0...100 2015-09-24 09:20:38.851

	int		nData1;			// Ԥ��, ��������ʹ��
	int		nData2;			// Ԥ��, ��������ʹ��
	int		nData3;			// Ԥ��, ��������ʹ��
	int		nData4;			// Ԥ��, ��������ʹ��
	void *	pData1;			// Ԥ������������ʹ�ã�����ָ��DataBlock, ��python������ָ��pyTag
	void *	pData2;			// Ԥ������������ʹ�ã�����ָ��DataBlock
}PKTAG;

struct _PKDRIVER;
typedef struct _PKDEVICE
{
	char *		szName;
	char *		szDesc;
	char *		szConnType;		// tcpclient/tcpserver/serial/udpserver/other
	char *		szConnParam;
	char *		szParam1;		// ���õ��豸����1
	char *		szParam2;		// ���õ��豸����2
	char *		szParam3;		// ���õ��豸����3
	char *		szParam4;		// ���õ��豸����4
	char *		szParam5;		// ���õ��豸����5
	char *		szParam6;		// ���õ��豸����6
	char *		szParam7;		// ���õ��豸����7
	char *		szParam8;		// ���õ��豸����8

	_PKDRIVER *	pDriver;		// ָ��������ָ��
	PKTAG **	ppTags;			// ���豸�����е�����tag�������
	int			nTagNum;		// ���豸��tag�����
	void *		_pInternalRef;	// ָ���ڲ��������ã�����ʹ��

	int			nConnTimeout;	// ���ӳ�ʱ, ����Ϊ��λ
	int			nRecvTimeout;	// ���ճ�ʱ, ����Ϊ��λ
	bool		bEnableConnect; // �豸�Ƿ���������

	// ����������������޸�, ���������Ԥ����������, ���Ա�����ʹ�ú͸ı�
	void *		pUserData[PK_USERDATA_MAXNUM];	// �����ֶΣ�������������Ա����ʹ��
	int			nUserData[PK_USERDATA_MAXNUM];	// �����ֶΣ�������������Ա����ʹ��
	char		szUserData[PK_USERDATA_MAXNUM][PK_DESC_MAXLEN];// �����ֶΣ�������������Ա����ʹ��
}PKDEVICE;

typedef struct _PKDRIVER
{
	char	*	szName;
	char	*	szDesc;
	char	*	szParam1;
	char	*	szParam2;
	char	*	szParam3;
	char	*	szParam4;

	PKDEVICE **	ppDevices;		// �������������������豸��Ŀ
	int			nDeviceNum;		// �豸����
	void *		_pInternalRef;	// ָ���ڲ��������ã�����ʹ��

	// �������Բ����޸�, ��������Ԥ������������ʹ��
	int			nUserData[PK_USERDATA_MAXNUM];		
	char		szUserData[PK_USERDATA_MAXNUM][PK_PARAM_MAXLEN];
	void *		pUserData[PK_USERDATA_MAXNUM];
}PKDRIVER;

typedef struct _PKTIMER
{
	int			nPeriodMS;	// ��ʱ���ڣ���λ����
	int			nPhaseMS;	// ��λ, ��ʱ������ʼʱ�䣬���Ժ�������������������
	void *		_pInternalRef; // �ڲ����ã�����ʹ��

	// �������Ԥ��������ʹ��
	int			nUserData[PK_USERDATA_MAXNUM];		// Ԥ���û�����
	char		szUserData[PK_USERDATA_MAXNUM][PK_PARAM_MAXLEN];// Ԥ���û�����
	void *		pUserData[PK_USERDATA_MAXNUM];// Ԥ���û�����
}PKTIMER;

PKDRIVER_EXPORTS int	Drv_Connect(PKDEVICE *pDevice, int nTimeOutMS);	// �������豸�������ӡ�ͨ������Ҫ��ʾ���ã���д����ʱ������ܻ��Զ����豸�������ӡ���������ʱ�����ֹ��Ͽ�����������
PKDRIVER_EXPORTS int	Drv_Disconnect(PKDEVICE *pDevice);	// �������豸�Ͽ����ӡ�ͨ������Ҫ��ʾ���ã������ڷ�������ʱ�����Ͽ����豸�����ӣ��Ա���������ʱ����
PKDRIVER_EXPORTS int	Drv_Send(PKDEVICE *pDevice, const char *szBuffer, int nBufLen, int nTimeOutMS);	// ���豸����nBufLen���ȵ����ݣ����ط��͵��ֽڸ���
PKDRIVER_EXPORTS int	Drv_Recv(PKDEVICE *pDevice, char *szBuffer, int nBufLen, int nTimeOutMS);	// ���豸�������nBufLen���ȵ����ݣ����ؽ��յ�ʵ���ֽڸ���
PKDRIVER_EXPORTS void	Drv_ClearRecvBuffer(PKDEVICE *pDevice);

PKDRIVER_EXPORTS int	Drv_SetTagData_Text(PKTAG *pTag, const char *szValue, unsigned int nTagSec = 0, unsigned short nTagMSec = 0, int nTagQuality = 0); // ����ĳ��TAG��ֵ(��Ϊ�ı��ַ�����ʽ)��ʱ��������������޸�pTag��VTQ, ��δд���ڴ����ݿ�
PKDRIVER_EXPORTS int	Drv_SetTagData_Binary(PKTAG *pTag, void *szData, int nValueLen, unsigned int nTagSec = 0, unsigned short nTagMSec = 0, int nTagQuality = 0); // ����ĳ��TAG��ֵ(������ԭʼֵ)��ʱ��������������޸�pTag��VTQ, ��δд���ڴ����ݿ�
PKDRIVER_EXPORTS int	Drv_UpdateTagsData(PKDEVICE *pDevice, PKTAG **pTags, int nTagNum); // �������ɸ�TAG�����ݡ�״̬��ʱ������ڴ����ݿ�
PKDRIVER_EXPORTS int	Drv_UpdateTagsDataByAddress(PKDEVICE *pDevice, const char *szTagAddress, const char *szValue, int nValueLen = 0, unsigned int nTimeSec = 0, unsigned short nTimeMilSec = 0, int nQuality = 0); // �������ݿ�����ݡ�״̬��ʱ������ڴ����ݿ⣬���ظ��µ�tag�����

PKDRIVER_EXPORTS void	Drv_LogMessage(int nLogLevel, const char *fmt, ...);	// ��ӡ��־���ļ��Ϳ���̨

PKDRIVER_EXPORTS void *	Drv_CreateTimer(PKDEVICE *pDevice, PKTIMER *timerInfo);
PKDRIVER_EXPORTS int	Drv_DestroyTimer(PKDEVICE *pDevice, void *pTimerHandle);

PKDRIVER_EXPORTS int	Drv_GetTagsByAddr(PKDEVICE *pDevice, const char *szTagAddress, PKTAG **pTags, int nMaxTagNum);// ����tag��ַ�ҵ�tag������ṹ��ָ��. �����ҵ���tag����
PKDRIVER_EXPORTS int	Drv_GetTagData(PKDEVICE *pDevice, PKTAG *pTag, char *szValue, int nValueBufLen, int *pnRetValueLen = NULL, unsigned int *pnTimeSec = NULL, unsigned short *pnTimeMilSec = NULL, int *pnQuality = NULL); // ��ȡ���ݿ���Ϣ�����ڿ��豸���а�λ����ʱ���Ȼ�ȡtag���Ӧ�Ŀ��ֵ��Ȼ���tag������λ����
PKDRIVER_EXPORTS int	Drv_TagValStr2Bin(PKTAG *pTag, const char *szTagStrVal, char *szTagValBuff, int nTagValBuffLen, int *pnRetValBuffLenBytes, int *pnRetValBuffLenBits = NULL); // tag��ֵ���ַ���ת��Ϊ������
																																													 // �����豸��Ч����������ͨѶ����״̬��ʱ�� ������ʱ�仹δ�յ���Ϣ�����Ϊ�豸�Ͽ�
PKDRIVER_EXPORTS int	Drv_SetConnectOKTimeout(PKDEVICE *pDevice, int nConnBadTimeoutSec); // ��������ͨѶ����״̬��ʱ�� ������ʱ�仹δ�յ���Ϣ�����Ϊ�豸�Ͽ�
PKDRIVER_EXPORTS int	Drv_SetConnectOK(PKDEVICE *pDevice); // ���õ�ǰ����״̬Ϊ��OK
#endif // _PK_DRVCOMMON_H_

