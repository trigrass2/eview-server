#pragma  once

#include <string>
#include <vector>

using namespace std;
#if defined (_WIN32)
// Define the pathname separator characters for Win32 (ugh).
#  define PK_DIRECTORY_SEPARATOR_STR "\\"
#  define PK_DIRECTORY_SEPARATOR_CHAR '\\'
#else
// Define the pathname separator characters for UNIX.
#  define PK_DIRECTORY_SEPARATOR_STR "/"
#  define PK_DIRECTORY_SEPARATOR_CHAR '/'
#endif /* _WIN32 */

#define  CHECK_NULL_RETURN_ERR(X)  { if(!X) return EC_ICV_DA_CONFIG_FILE_STRUCTURE_ERROR; }

#define REC_ATTR_TAGNAME			"n"	
#define REC_ATTR_QUALITY			"q"
#define REC_ATTR_VALUE				"v"
#define REC_ATTR_TIME				"t"
#define REC_ATTR_FIRST_VALUE		"fv"
#define REC_ATTR_FIRST_TIME			"ft"
#define REC_ATTR_FIRST_QUALITY		"fq"
#define REC_ATTR_MAX_VALUE			"maxv"
#define REC_ATTR_MAX_TIME			"maxt"
#define REC_ATTR_MAX_QUALITY		"maxq"
#define REC_ATTR_MIN_VALUE			"minv"
#define REC_ATTR_MIN_TIME			"mint"
#define REC_ATTR_MIN_QUALITY		"minq"
#define REC_ATTR_AVERAGE_VALUE		"avg"
#define REC_ATTR_DATA_COUNT			"cnt"
#define REC_ATTR_VALUESET			"vs"

#define MONGO_DB_NAME				"peak"
#define MONGO_DB_TEMP				"peak.temp"
#define MONGO_DB_RAW_PREFIX			"peak.raw_"
#define MONGO_DB_MINUTE_PREFIX		"peak.minute_"
#define MONGO_DB_HOUR_PREFIX		"peak.hour_"
#define MONGO_DB_DAY				"peak.day"
#define MONGO_DB_MONTH				"peak.month"
#define MONGO_DB_YEAR				"peak.year"

#define TIMELEN_YYYYMMDD_HHMMSSXXX			23	// yyyy-MM-dd hh:mm:ss  2017-06-28 15:30:33.123
#define TIMELEN_YYYYMMDD_HHMMSS				19	// yyyy-MM-dd hh:mm:ss  2017-06-28 15:30:33
#define TIMELEN_YYYYMMDD_HHMM				16	// yyyy-MM-dd hh:mm  2017-06-28 15:30
#define TIMELEN_YYYYMMDD_HHM				15	// yyyy-MM-dd hh:m  2017-06-28 10:5
#define TIMELEN_YYYYMMDD_HH					13  // yyyy-MM-dd hh  2017-06-28 10
#define TIMELEN_YYYYMMDD					10	// yyyy-MM-dd hh:mm:ss  2017-06-28
#define TIMELEN_YYYYMM						7	// yyyy-MM 2017-06
#define TIMELEN_YYYY						4	// yyyy-MM 2017


//temp��
class CTagRawRecord
{
public:
	string	strTagName;
	double	dbValue;
	string	strTime;
	int		nQuality;
};

//���ӱ�, Сʱ��, �ձ�, �±�, ���
class CTagStatRecord
{
public:
	string	strTagName;
	double	dbValue;
	string	strTime;
	string	strFirstTime;	// ��ʱ����ڵ�һ��tag���ʱ��
	int		nQuality;

	double	dbAvg;			// ���ƽ��ֵ
	unsigned nDataCount;	// ��ĸ���

	double	dbMaxValue;
	string	strMaxTime;
	int		nMaxQuality;

	double	dbMinValue;
	string	strMinTime;
	int		nMinQuality;
	CTagStatRecord()
	{
		// ����vec1MinuteRec�еĵ�һ��ֵ��ƽ��ֵ�����ֵ����Сֵ������
		dbMaxValue = -1e10;
		dbMinValue = 1e10;
		nDataCount = 0;
		dbAvg = 0.f;
	}
};

// ����tag������������Ϣ,�����ݸ���ֵʹ��;
class CTagInfo
{		   
public:
	string strTagName;     // tag������;
	unsigned int nSavePeriod;			// �������ڣ���Ϊ��λ
	vector<CTagRawRecord *> vecTempRec;	//��tag�㱣���temp�е���ʱ����  
	CTagInfo()
	{
		nSavePeriod = 0;
		strTagName = "";
	}
	~CTagInfo()
	{
		for (vector<CTagRawRecord *>::iterator itRec = vecTempRec.begin(); itRec != vecTempRec.end();  itRec++)
		{
			CTagRawRecord *pRec = *itRec;
			delete pRec;
			pRec = NULL;
		}
		vecTempRec.clear();
	}
};

