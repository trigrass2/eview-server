#pragma once


typedef struct _PKTAGDATA
{
public:
	char szName[PK_NAME_MAXLEN + 1];
	int	 nDataType;
	char *szData;
	int	 nDataLen;
	int	 nQuality;
	unsigned int  nTimeSec; // 1970�꿪ʼ��ʱ���,����,time_t
	unsigned short nTimeMilSec; // ���ڵĺ�����,0...100
}PKTAGDATA;

