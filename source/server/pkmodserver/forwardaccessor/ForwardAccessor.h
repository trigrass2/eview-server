#ifndef _FORWARD_ACCESSOR_H_
#define _FORWARD_ACCESSOR_H_

#include <string>
#include <map>
#include <vector>
#include <ace/Thread_Mutex.h>
#include <ace/Default_Constants.h>
#include "common/pkcomm.h"
#include "driversdk/pkdrvcmn.h"
#include <ace/DLL.h>
#include "common/TypeCast.h"
#include "common/eviewdefine.h"

using namespace std;

extern unsigned short g_tagMaxDataSize[TAG_DT_MAX + 1];
typedef struct _DRIVERINFO
{ 
	string	strDrvName;		// 驱动名称，如snmp
	string	strDrvPath;		// 驱动路径，包括EXE名称

	_DRIVERINFO()
	{ 
		strDrvName = strDrvPath = "";
	}
}DRIVERINFO;
typedef std::map<string, DRIVERINFO*> DRIVERMAP; // 驱动Map

// IODATA  IO数据记录
typedef struct _RAWDATA{
	char  nDataType;			// 硬件选项
	int   nDataSize;		// 数据长度
	char  *pszData;			// 数据缓冲区指针
	unsigned short uQuality;
	HighResTime timeStamp;
	_RAWDATA()
	{
		memset(this, 0, sizeof(_RAWDATA));
	}

	~_RAWDATA()
	{
		if(pszData != NULL)
		{
			delete []pszData;
			pszData = NULL;
		}
	}
	void AllocDataBuf()
	{
		if(nDataType >= 0 && nDataType < TAG_DT_MAX)
			nDataSize = g_tagMaxDataSize[nDataType];
		else
			nDataSize = 0;

		if(nDataSize > 0)
		{
			if(pszData)
				delete [] pszData;
			pszData = new char[nDataSize];
		}
	}
}RAWDATA;

typedef struct _CVTAG
{
	// 下面几个是配置文件中配置的TAG类型
	string		strTagName;		// TAG名称
	string		strTagAddr;// TAG地址 such as: device0:1.2.e.5.f.s.e
	char		nDataType;	// 配置的请求的数据类型，包括：模拟量/数字量/BLOB/文本量。

	// 下面是当前读取到的值的信息
	RAWDATA		rawData;
	HighResTime tmStamp;		// 时间戳
	unsigned short uQuality;	// 数据质量
	long		lServerHandle;	// 服务端返回的句柄
	DRIVERINFO	*pDrvInfo;		// 所属的驱动的指针，访问DIT错误时可以尝试解析地址

	_CVTAG()
	{
		strTagName = strTagAddr = "";
		nDataType = TAG_DT_INT16;
		memset(&tmStamp,0, sizeof(tmStamp));
		uQuality = 0;
		pDrvInfo = NULL;
		lServerHandle = 0;
	}
}CVTAG;
typedef std::map<string, CVTAG*> CVTAGMAP;

typedef struct _TAGGROUP
{
	vector<CVTAG *> vecTag;
	unsigned short	uServerGrpHandle;	// 服务端的组注册的句柄
	unsigned short	uLocalGrpId;		// 本地的组ID，用以区分不同的组
	unsigned long	lSucessWriteCount;	// 写入到服务端成功请求技术
	unsigned long	lRequestWriteCount;	// 写入服务到服务端请求技术
	time_t			tmLastRequest;
	time_t			tmLastResponse;		// 上次接收到数据的时间，用于诊断判断是否需要重连
	_TAGGROUP()
	{
		uServerGrpHandle = uLocalGrpId = 0;
		tmLastResponse = tmLastRequest = 0;
		lSucessWriteCount = lRequestWriteCount = 0;
	}
}TAGGROUP;

#endif
