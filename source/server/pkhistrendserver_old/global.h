#define MAXNUMOFTAG	 256
bool bIsFirstTime = true;

//当前时间节点
//typedef struct _NOWTIME
//{
	int g_nowYear[MAXNUMOFTAG];
	int g_nowMonth[MAXNUMOFTAG];
	int g_nowDay[MAXNUMOFTAG];
	int g_nowHour[MAXNUMOFTAG];
	int g_nowMinute[MAXNUMOFTAG];
	int g_nowSecond[MAXNUMOFTAG];
//}NOWTIME;


//实时表
//typedef struct _NOWTIME
//{
	char* g_realTimeValue[MAXNUMOFTAG];
	char* g_realMaxValue[MAXNUMOFTAG];
	char* g_realMinValue[MAXNUMOFTAG];
	char* g_realTime[MAXNUMOFTAG];
	char* g_realMaxValueTime[MAXNUMOFTAG];
	char* g_realMinValueTime[MAXNUMOFTAG];
//}NOWTIME;


//秒表
//typedef struct _SECONDTIME
//{
	char* g_maxSecValue[MAXNUMOFTAG];
	char* g_minSecValue[MAXNUMOFTAG];
	char* g_firstSecValue[MAXNUMOFTAG];
	char* g_maxSecValueTime[MAXNUMOFTAG];
	char* g_minSecValueTime[MAXNUMOFTAG];
	char* g_firstSecValueTime[MAXNUMOFTAG];
//}SECONDTIME;


//分钟表
//typedef struct _MINUTETIME
//{
	char* g_maxMinValue[MAXNUMOFTAG];
	char* g_minMinValue[MAXNUMOFTAG];
	char* g_firstMinValue[MAXNUMOFTAG];
	char* g_maxMinValueTime[MAXNUMOFTAG];
	char* g_minMinValueTime[MAXNUMOFTAG];
	char* g_firstMinValueTime[MAXNUMOFTAG];
//}MINUTETIME;


//小时表
//typedef struct _HOURTIME
//{
	char* g_maxHourValue[MAXNUMOFTAG];
	char* g_minHourValue[MAXNUMOFTAG];
	char* g_firstHourValue[MAXNUMOFTAG];
	char* g_maxHourValueTime[MAXNUMOFTAG];
	char* g_minHourValueTime[MAXNUMOFTAG];
	char* g_firstHourValueTime[MAXNUMOFTAG];
//}HOURTIME;


//天表
//typedef struct _HOURTIME
//{
	char* g_maxDayValue[MAXNUMOFTAG];
	char* g_minDayValue[MAXNUMOFTAG];
	char* g_firstDayValue[MAXNUMOFTAG];
	char* g_maxDayValueTime[MAXNUMOFTAG];
	char* g_minDayValueTime[MAXNUMOFTAG];
	char* g_firstDayValueTime[MAXNUMOFTAG];
//};		  HOURTIME


//月表
	char* g_maxMonValue[MAXNUMOFTAG];
	char* g_minMonValue[MAXNUMOFTAG];
	char* g_firstMonValue[MAXNUMOFTAG];
	char* g_maxMonValueTime[MAXNUMOFTAG];
	char* g_minMonValueTime[MAXNUMOFTAG];
	char* g_firstMonValueTime[MAXNUMOFTAG];

//年表
	char* g_maxYearValue[MAXNUMOFTAG];
	char* g_minYearValue[MAXNUMOFTAG];
	char* g_firstYearValue[MAXNUMOFTAG];
	char* g_maxYearValueTime[MAXNUMOFTAG];
	char* g_minYearValueTime[MAXNUMOFTAG];
	char* g_firstYearValueTime[MAXNUMOFTAG];

	char* g_nFirstTime[MAXNUMOFTAG];
	char* g_nFirstValue[MAXNUMOFTAG];

//strTime;
char strTime[MAXNUMOFTAG] = { 0 };

//LASTTIME
typedef struct _LASTTIME
{
	int nYear[MAXNUMOFTAG];
	int nMonth[MAXNUMOFTAG];
	int nDay[MAXNUMOFTAG];
	int nHour[MAXNUMOFTAG];
	int nMinute[MAXNUMOFTAG];
	int nSecond[MAXNUMOFTAG];
}LASTTIME;

LASTTIME PreTime;
LASTTIME NowTime;