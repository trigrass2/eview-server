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
#define REC_ATTR_VALUESET			"vs"

#define MONGO_DB_NAME					"peak"
#define MONGO_DB_TABLE_TREND_NEWEST		"peak.trend_newest"
#define MONGO_DB_TABLE_TREND_PREFIX		"peak.trend_"
#define MONGO_DB_TABLE_RECORD_PREFIX	"peak.record_"

#define NULLASSTRING(x)					x==NULL?"":x

//#define TIMELEN_YYYYMMDD_HHMMSSXXX			23	// yyyy-MM-dd hh:mm:ss  2017-06-28 15:30:33.123
#define TIMELEN_YYYYMMDD_HHMMSS				19	// yyyy-MM-dd hh:mm:ss  2017-06-28 15:30:33
#define TIMELEN_YYYYMMDD_HHMM				16	// yyyy-MM-dd hh:mm  2017-06-28 15:30
#define TIMELEN_YYYYMMDD_HHM				15	// yyyy-MM-dd hh:m  2017-06-28 10:5
#define TIMELEN_YYYYMMDD_HH					13  // yyyy-MM-dd hh  2017-06-28 10
#define TIMELEN_YYYYMMDD					10	// yyyy-MM-dd hh:mm:ss  2017-06-28

#define MONTABLE_INDEX_CREATED_ALREADY		1
#define MONTABLE_INDEX_NEW_CREATED			0
#define MONTABLE_INDEX_CREATE_FAILED		-1

//temp表
class CTagRawRecord
{
public:
	string	strTagName;
	double	dbValue;
	string	strTime;
	int		nQuality;
};

// 保存tag点的所有相关信息,供数据更新值使用;
class CTagInfo
{		   
public:
	string strTagName;     // tag点名字;
	unsigned int nTrendIntervalSec;	// 趋势间隔
	unsigned int nRecordIntervalSec;	// 记录间隔
	// unsigned int nSavePeriod;			// 保存周期，秒为单位
	vector<CTagRawRecord *> vecTempRec;	//该tag点保存的temp中的临时数据  
	CTagInfo()
	{
		nTrendIntervalSec = 0;
		nRecordIntervalSec = 0;
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

