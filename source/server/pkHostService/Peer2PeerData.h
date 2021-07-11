#ifndef _PEER_2_PEER_DATA_H_
#define _PEER_2_PEER_DATA_H_

#include <cstring>

/* unsigned long long */
typedef unsigned long long ULLong;

//Protocol
#define HEADER						0x02
#define PACKHEADERLENGTH			(1 + 4)
#define MESSAGEMAXLENGGTH			(1024 * 1024 * 128)

//Buffer Length
#define RECEIVED_BUFFER_LENGTH		(1024 * 4)

//Sleep Timespan
#define QUERY_SLEEP_TIMESTAMP_MS	1024	 
#define RECEIVE_SLEEP_MICROSECOND	32

//Timestamp
#define ELAPSEDSECONDS				10000000

//Protocol Code
#define PROTOCOL_CODE_CPU_MEM_DISK_INFO		0x0001
#define PROTOCOL_CODE_CONTROL				0x0002
#define PROTOCOL_CODE_PROCESSINFO			0x0004
//Type
#define TYPE_CPU					1
#define TYPE_MEMORY					2
#define TYPE_DRIVE					3
#define TYPE_PROCESS				4
//Contorl-Type
#define TYPE_CONTROL_RUN			0x0010
#define TYPE_CONTROL_KILL			0x0020

//Connect to HostServer Status
#define ADDR_DEVICE_CONN_STATUS		"status"

//Control Run-File Address
#define CTRL_CMD_FILE_PATH			"ctl:filepath" //控制执行文件,值为路径
#define CTRL_CMD_PROCESS_KILL		"ctl:kill" //控制进程结束,值为进程pid
//CPU Header-Address
#define ADDR_CPU_TOTAL				"cpu:total"				//CPU总值
//Memory Header-Address
#define ADDR_MEMORY_TOTAL			"memory:total"			//内存总值 
#define ADDR_MEMORY_FREE			"memory:free"			//内存空闲值 
#define ADDR_MEMORY_FREEPERCENT		"memory:freepercent"	//内存空闲率 
//Disk Header-Address
#define ADDR_DISK_TOTAL				"disk:total"			//磁盘总值 
#define ADDR_DISK_FREE				"disk:free"				//磁盘空闲值 
#define ADDR_DISK_FREEPERCENT		"disk:freepercent"		//磁盘空闲率
//Process Header-Address
#define ADDR_PROCESS_NAME			"process:name"			//进程名

//Calculator Average Sub-Address
#define CALC_AVG_A_M1				"avg:m1"	//平均一分钟时间
#define CALC_AVG_A_M10				"avg:m10"	//平均十分钟时间
#define CALC_AVG_A_M30				"avg:m30"	//平均三十分钟时间
#define CALC_AVG_A_M60				"avg:m60"	//平均六十分钟时间
#define CALC_AVG_A_H3				"avg:h3"	//平均三小时时间
#define CALC_AVG_A_H6				"avg:h6"	//平均六小时时间
#define CALC_AVG_A_H12				"avg:h12"	//平均十二小时时间
#define CALC_AVG_A_H24				"avg:h24"	//平均二十四小时时间

//CPU Combine-Address like:
//#define TAGADDR_CPU_AVG				"cpu:total,avg:m10"
//Memory Combine-Address like:
//#define TAGADDR_CPU_AVG				"memory:free,avg:h3"
//Disk Combine-Address like:
//#define TAGADDR_CPU_AVG				"disk:free,C:\\"


//Calculator Persent Address-Value
#define CALC_PRT_ALL				0x00000001	//当前总值
#define CALC_PRT_V					0x00000002	//当前空闲值
#define CALC_PRT_VP					0x00000004	//当前空闲率

//Calculator Average Address-Value						  
#define CALC_AVG_V_M1				0x00000010	//一分钟平均空闲计算值
#define CALC_AVG_VP_M1				0x00000020	//一分钟平均空闲计算值率
#define CALC_AVG_V_M10				0x00000040	//十分钟平均空闲计算值
#define CALC_AVG_VP_M10				0x00000080	//十分钟平均空闲计算值率
#define CALC_AVG_V_M30				0x00000100	//三十分钟平均空闲计算值
#define CALC_AVG_VP_M30				0x00000200	//三十分钟平均空闲计算值率
#define CALC_AVG_V_M60				0x00000400	//六十分钟平均空闲计算值
#define CALC_AVG_VP_M60				0x00000800	//六十分钟平均空闲计算值率
#define CALC_AVG_V_H3				0x00001000	//三小时平均空闲计算值
#define CALC_AVG_VP_H3				0x00002000	//三小时平均空闲计算值率
#define CALC_AVG_V_H6				0x00004000	//六小时平均空闲计算值
#define CALC_AVG_VP_H6				0x00008000	//六小时平均空闲计算值率
#define CALC_AVG_V_H12				0x00010000	//十二小时平均空闲计算值
#define CALC_AVG_VP_H12				0x00020000	//十二小时平均空闲计算值率
#define CALC_AVG_V_H24				0x00040000	//二十四小时平均空闲计算值
#define CALC_AVG_VP_H24				0x00080000	//二十四小时平均空闲计算值率


/* 挂载/盘符容量信息 */
struct CapacityInfo
{
	/* 总值(kB) */
	ULLong m_nTotal;
	/* 空闲值(kB) */
	ULLong m_nFree;
	/* 挂载/盘符名称 */
	char m_szName[128];

	CapacityInfo() : m_nTotal(0), m_nFree(0)
	{
		memset(m_szName, 0, sizeof(m_szName));
	}
};

/* 进程信息 */
struct ProcessInfo
{
	/* Start-Time Year */
	unsigned short m_nStartTime_Year;
	/* Start-Time Month */
	unsigned short m_nStartTime_Month;
	/* Start-Time Day */
	unsigned short m_nStartTime_Day;
	/* Start-Time Hour */
	unsigned short m_nStartTime_Hour;
	/* Start-Time Minute */
	unsigned short m_nStartTime_Minute;
	/* Start-Time Second */
	unsigned short m_nStartTime_Second;
	/* Pid */
	unsigned int m_nPid;
	/* Run-Time(s) */
	unsigned long long m_nRunTime;
	/* Process Name */
	char m_szName[128];
	/* Process File-Name */
	char m_szFileName[256];
#ifdef WIN32
	/* Process Describe */
	char m_szDescribe[512];
#endif

	ProcessInfo() : m_nRunTime(0), m_nPid(0)
	{
		m_nStartTime_Year = m_nStartTime_Month = m_nStartTime_Day = m_nStartTime_Hour = m_nStartTime_Minute = m_nStartTime_Second = 0;
		memset(m_szName, 0, sizeof(m_szName));
		memset(m_szFileName, 0, sizeof(m_szFileName));
#ifdef WIN32
		memset(m_szDescribe, 0, sizeof(m_szDescribe));
#endif
	}
};

#endif //_PEER_2_PEER_DATA_H_
