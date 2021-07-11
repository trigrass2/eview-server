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
	char	szObjectPropName[PK_NAME_MAXLEN * 2 + 1];		// tempdevice1.temporature，对象和属性名称：对象名.属性名。或者tagName：tag1
	char	szFieldName[PK_NAME_MAXLEN + 1];				// Fieldname，对象属性的域值，包括：v,t,q,ml等，字符串格式。如果为空字符串表示取出全部的域，json格式
	char	*szData;					// Data，2K, 空字符串. blob则是表示为base64的字符窜
	int		nDataBufLen;				// 数据的最大缓冲区长度
	int		nStatus;											// 读取到的数据的返回值. 0表示成功
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
		if (nNewDataLen >= nDataBufLen) // 如果小于等于之前缓冲区则继续使用之前的缓冲区
		{
			// 否则分配新的缓冲区！
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
		if (nNewDataLen <= nDataBufLen - 1) // 如果小于等于之前缓冲区则继续使用之前的缓冲区
			return -1;

		// 否则分配新的缓冲区！
		delete[] szData;
		szData = new char[nNewDataLen + 1];
		nDataBufLen = nNewDataLen + 1;
		return 0;
	}
}PKDATA;

PKDATA_EXPORTS PKHANDLE pkInit(char *szIp, char *szUserName, char *szPassword);
PKDATA_EXPORTS int pkExit(PKHANDLE hHandle);

PKDATA_EXPORTS int pkControl(PKHANDLE hHandle, const char *szObjectPropName, const char *szFieldName, const char *szData);	// 单个控制，值形式：100（非json）
PKDATA_EXPORTS int pkGet(PKHANDLE hHandle, const char *szObjectPropName, const char *szFieldName, char *szDataBuff, int nDataBufLen, int *pnOutDataLen); // 值形式：{"v":"","t":"2015-12-02 19:14:04.035","q":"1","m":"read failcount:4"}
PKDATA_EXPORTS int pkMGet(PKHANDLE hHandle, PKDATA tagData[], int nTagNum);		    // 批量获取，值形式：{"v":"","t":"2015-12-02 19:14:04.035","q":"1","m":"read failcount:4"}
PKDATA_EXPORTS int pkMControl(PKHANDLE hHandle, PKDATA tagData[], int nTagNum);	    // 批量控制，值形式：100（非json）

// 仅仅更新内存数据库的数值。通常不需要调用，因为更新完毕后，马上会被更新回来. szFieldName暂时没用，可以置空(缺省为value)
PKDATA_EXPORTS int pkUpdate(PKHANDLE hHandle, char *szObjectPropName, char *szTagVal, int nQuality); // "object1.prop1", "1234", 0---good quality
#endif // _PK_DATA_H_
