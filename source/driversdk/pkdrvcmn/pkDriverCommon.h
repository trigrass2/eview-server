#pragma once


typedef struct _PKTAGDATA
{
public:
	char szName[PK_NAME_MAXLEN + 1];
	int	 nDataType;
	char *szData;
	int	 nDataLen;
	int	 nQuality;
	unsigned int  nTimeSec; // 1970年开始的时间戳,到秒,time_t
	unsigned short nTimeMilSec; // 秒内的毫秒数,0...100
}PKTAGDATA;

