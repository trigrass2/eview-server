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
	string	strDrvName;		// �������ƣ���snmp
	string	strDrvPath;		// ����·��������EXE����

	_DRIVERINFO()
	{ 
		strDrvName = strDrvPath = "";
	}
}DRIVERINFO;
typedef std::map<string, DRIVERINFO*> DRIVERMAP; // ����Map

// IODATA  IO���ݼ�¼
typedef struct _RAWDATA{
	char  nDataType;			// Ӳ��ѡ��
	int   nDataSize;		// ���ݳ���
	char  *pszData;			// ���ݻ�����ָ��
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
	// ���漸���������ļ������õ�TAG����
	string		strTagName;		// TAG����
	string		strTagAddr;// TAG��ַ such as: device0:1.2.e.5.f.s.e
	char		nDataType;	// ���õ�������������ͣ�������ģ����/������/BLOB/�ı�����

	// �����ǵ�ǰ��ȡ����ֵ����Ϣ
	RAWDATA		rawData;
	HighResTime tmStamp;		// ʱ���
	unsigned short uQuality;	// ��������
	long		lServerHandle;	// ����˷��صľ��
	DRIVERINFO	*pDrvInfo;		// ������������ָ�룬����DIT����ʱ���Գ��Խ�����ַ

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
	unsigned short	uServerGrpHandle;	// ����˵���ע��ľ��
	unsigned short	uLocalGrpId;		// ���ص���ID���������ֲ�ͬ����
	unsigned long	lSucessWriteCount;	// д�뵽����˳ɹ�������
	unsigned long	lRequestWriteCount;	// д����񵽷����������
	time_t			tmLastRequest;
	time_t			tmLastResponse;		// �ϴν��յ����ݵ�ʱ�䣬��������ж��Ƿ���Ҫ����
	_TAGGROUP()
	{
		uServerGrpHandle = uLocalGrpId = 0;
		tmLastResponse = tmLastRequest = 0;
		lSucessWriteCount = lRequestWriteCount = 0;
	}
}TAGGROUP;

#endif
