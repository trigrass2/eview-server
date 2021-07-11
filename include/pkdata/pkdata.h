#ifndef _PK_DATA_H_
#define _PK_DATA_H_

#ifdef _WIN32
#define  PKDATA_EXPORTS extern "C" __declspec(dllexport)
#else //_WIN32
#define  PKDATA_EXPORTS extern "C" __attribute__ ((visibility ("default")))
#endif //_WIN32

#include "memory.h"
#include "eviewcomm/eviewcomm.h"
#include "pkcomm/pkcomm.h"

typedef void * PKHANDLE;
typedef struct _PKDATA
{		
public:
	char	szObjectPropName[PK_NAME_MAXLEN * 2 + 1];		// tempdevice1.temporature��������������ƣ�������.������������tagName��tag1
	char	szFieldName[PK_NAME_MAXLEN + 1];				// Fieldname���������Ե���ֵ��������v,t,q,ml�ȣ��ַ�����ʽ�����Ϊ���ַ�����ʾȡ��ȫ������json��ʽ
	char	*szData;					// Data��2K, ���ַ���. blob���Ǳ�ʾΪbase64���ַ���
	int		nDataBufLen;				// ���ݵ���󻺳�������
	int		nStatus;											// ��ȡ�������ݵķ���ֵ. 0��ʾ�ɹ�
	_PKDATA(){
		szData = new char[PK_NAME_MAXLEN + 1];
		szObjectPropName[0] = szFieldName[0] = szData[0] = '\0';
		nDataBufLen = PK_NAME_MAXLEN + 1;
		nStatus = -1000;
	}
	~_PKDATA(){
		delete []szData;
		nDataBufLen = 0;
	}

	int SetData(const char *szNewData, int nNewDataLen)
	{
		if (nNewDataLen >= nDataBufLen) // ���С�ڵ���֮ǰ�����������ʹ��֮ǰ�Ļ�����
		{
			// ��������µĻ�������
			delete[] szData;
			szData = new char[nNewDataLen + 1];
			nDataBufLen = nNewDataLen + 1;
		}

		PKStringHelper::Safe_StrNCpy(szData, szNewData, nNewDataLen + 1);
		szData[nDataBufLen - 1] = 0; 
		return 0;
	}

	int SetDataLen(unsigned int nNewDataLen)
	{
		if (nNewDataLen <= nDataBufLen - 1) // ���С�ڵ���֮ǰ�����������ʹ��֮ǰ�Ļ�����
			return -1;

		// ��������µĻ�������
		delete[] szData;
		szData = new char[nNewDataLen + 1];
		nDataBufLen = nNewDataLen + 1;
		return 0;
	}
}PKDATA;

PKDATA_EXPORTS PKHANDLE pkInit(char *szIp, char *szUserName, char *szPassword);
PKDATA_EXPORTS int pkExit(PKHANDLE hHandle);

PKDATA_EXPORTS int pkControl(PKHANDLE hHandle, const char *szObjectPropName, const char *szFieldName, const char *szData);	// �������ƣ�ֵ��ʽ��100����json��
PKDATA_EXPORTS int pkGet(PKHANDLE hHandle, const char *szObjectPropName, const char *szFieldName, char *szDataBuff, int nDataBufLen, int *pnOutDataLen); // ֵ��ʽ��{"v":"","t":"2015-12-02 19:14:04.035","q":"1","m":"read failcount:4"}
PKDATA_EXPORTS int pkMGet(PKHANDLE hHandle, PKDATA tagData[], int nTagNum);		    // ������ȡ��ֵ��ʽ��{"v":"","t":"2015-12-02 19:14:04.035","q":"1","m":"read failcount:4"}
PKDATA_EXPORTS int pkMControl(PKHANDLE hHandle, PKDATA tagData[], int nTagNum);	    // �������ƣ�ֵ��ʽ��100����json��

// ���������ڴ����ݿ����ֵ��ͨ������Ҫ���ã���Ϊ������Ϻ����ϻᱻ���»���. szFieldName��ʱû�ã������ÿ�(ȱʡΪvalue)
PKDATA_EXPORTS int pkUpdate(PKHANDLE hHandle, char *szObjectPropName, char *szTagVal, int nQuality); // "object1.prop1", "1234", 0---good quality
#endif // _PK_DATA_H_
