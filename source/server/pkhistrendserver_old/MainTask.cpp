/**************************************************************
 *  Filename:    PKHisTrendServer.cpp
 *  Copyright:   Shanghai Peakinfo Software Co., Ltd.
 *
 *  Description: 历史数据处理
 *
 *  @author:     xingxing
 *  @version     04/10/2017  xingxing  Initial Version
**************************************************************/
#include "mongo/client/dbclient.h"
#include <ace/Default_Constants.h>
#include <fstream>  
#include <streambuf> 
#include <map>
#include <vector>
#include <time.h>  
#include <stdlib.h>  
#include <string.h> 
#include <stdio.h>
#include "common/OS.h"
#include "MainTask.h"
#include "common/pkcomm.h"
#include "sqlite/CppSQLite3.h"
#include "json/json.h"
#include "ace/Thread_Mutex.h"
#include "ace/Guard_T.h"
#include "math.h"
#include "common/pkdefine.h"
#include "pklog/pklog.h"
#include "LoadDateBase.h"
#include "ace/High_Res_Timer.h"
#include "global.h"
#include "pkdata/pkdata.h"

using namespace mongo;
using namespace std;

vector<CTagInfo > m_vecTagInfo;
map<int,string> g_mapAlarmID2Cname;

#define  CHECK_NULL_RETURN_ERR(X)  { if(!X) return EC_ICV_DA_CONFIG_FILE_STRUCTURE_ERROR; }
#define	DEFAULT_SLEEP_TIME		1000	
#define NULLASSTRING(x) x==NULL?"":x
#define PK_DIRECTORY_SEPARATOR_STR "\\"
#define MAXCOMMANDSIZE 256
#define LENOFVALUE 32

#ifndef PK_FAILURE
#define PK_FAILURE	-1

mongo::DBClientConnection con;
CPKLog PKLog;
string strConMongoServer = "";

#endif 
/**
 *  构造函数.
 *
 *	@return   ICV_SUCCESS/ICV_FAILURE.
 *
 *  @version     05/30/2008  xingxing  Initial Version.
 */
CMainTask::CMainTask()
{
	m_bStop = false;
	m_bStopped = false;
}

/**
 *  析构函数;
 *
 *	@return   无
 *
 *  @version     04/05/2017  xingxing  Initial Version.
 */
CMainTask::~CMainTask()
{
	m_bStop = true;
}


/**
 *  虚函数，继承自ACE_Task.
 *
 *	@return   无
 *
 *  @version     04/05/2017  xingxing  Initial Version.
 */
int CMainTask::svc()
{	
	ACE_High_Res_Timer::global_scale_factor();
	int nNum = 1;
	readConfig();

	//连接Mongodb数据库;
	int lRet = InitMongdb();
	if (lRet < 0)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR,"Ienit MongoDB Failed");
		return PK_FAILURE;
	}

	//start方法获取tag点信息;
	int nRet = OnStart();
	//启动后分配内存
	for (int i = 0; i < MAXNUMOFTAG; i++)
	{
		InitMemoryAllocation(i);
	}

	while(!m_bStop)
	{
		//Deal with database
		DealEvent();
		cout << "The " << nNum<<" times to run PKHisTrendServer." << endl;
		nNum++;
		ACE_Time_Value tvSleep;
		tvSleep.msec(DEFAULT_SLEEP_TIME); // 1000ms
		ACE_OS::sleep(tvSleep);
	}
	//释放内存
	for (int i = 0; i < MAXNUMOFTAG; i++)
	{
		UnInitMemoryAllocation(i);
	}
	PKLog.LogMessage(PK_LOGLEVEL_NOTICE,"PbServer 正常退出！");
	OnStop();
	m_bStopped = true;
	return nRet;	
}

/**
*  开始任务.
*
*  @return   PK_FAILURE/PK_FAILURE.
*
*  @version     04/10/2017  xingxing  Initial Version.
*/
int CMainTask::OnStart()
{
	int nStatus = PK_SUCCESS;
	int nRetn = InitRedis();
	nStatus = PKEviewDbApi.Init();

	if (nStatus != PK_SUCCESS)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR,"Init DataBase failed");
		return nStatus;
	}
	//连接sqlite数据库并且将数据进行保存
	m_vecTagInfo.clear();
	vector<vector<string>> rows;
	vector<string> fields;
	vector<int > lis;
	long lQueryId=0;
	//此处如果执行失败可能需要再次连接数据库;
	int nRet = PKEviewDbApi.SQLExecute("select * from t_device_tag where enable_history=1;", rows);
	if(nRet != 0)
	{
		PKLog.LogMessage(PK_LOGLEVEL_INFO, "执行SQL查询语句失败！");
		PKEviewDbApi.Exit();
		return PK_FAILURE;
	}

	//将符合条件的tag点保存起来;
	for(int iRow = 0; iRow < rows.size(); iRow ++)
	{
		vector<string> &row = rows[iRow];
		CTagInfo laneInfo;
		int iCol = 0;
		laneInfo.nTagID = atoi(row[iCol].c_str());
		iCol++;
		strcpy(laneInfo.szTagName,row[iCol].c_str()); 
		iCol++;
		m_vecTagInfo.push_back(laneInfo);
	}
	return PK_SUCCESS;
}

/**
*  停止任务.
* 
*  @version     04/10/2017  xingxing  Initial Version.
*/
void CMainTask::OnStop()
{
	// 停止其他线程;
	m_redisRW.finalize();
}

int CMainTask::Start()
{
	return activate(THR_NEW_LWP | THR_JOINABLE |THR_INHERIT_SCHED);
}

void CMainTask::Stop()
{
	m_bStop = true;
	int nWaitResult = wait();

	ACE_Time_Value tvSleep;
	tvSleep.msec(1000);
	while(!m_bStopped)
		ACE_OS::sleep(tvSleep);
}


/**
*  功能：初始化Redis.
*  
*  @version     04/10/2017  xingxing  Initial Version.
*/

int CMainTask::InitRedis()
{
	int nRet = 0;

	nRet = m_redisRW.initialize(PK_LOCALHOST_IP, PDB_LISTEN_PORT, PDB_ACCESS_PASSWORD, 0);
	if(nRet != PK_SUCCESS)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR,"MainTask m_redisRW.Initialize failed,ret:%d",nRet);
		return nRet;
	}

	PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "Init MemDB success");
	return nRet;
}

/**
*  功能：处理接受Tag点的解析工作及MongoDb数据写入工作.
*
*  @version   04/10/2017  xingxing Initial Version.
*/
int CMainTask::DealEvent()
{
	//m_redisRW.get()
	//获取内存数据库中的数据点;
	vector<CTagInfo >::iterator curTag;
	Json::Reader reader;
	Json::Value root;
	int nQuality;
	string strTime;
	string strValue;
	int nTagNum=0;
	for(curTag=m_vecTagInfo.begin();curTag != m_vecTagInfo.end();curTag++)
	{
		//getTagValue from memdb;
		string curStr = m_redisRW.get(curTag->szTagName);
		reader.parse(curStr,root,false);

		//判断数据保存类型:int \ double \ string
		nQuality = atoi(root["quality"].asString().c_str());
		strTime = root["time"].asString();;
		strValue = root["value"].asString();
		if(nQuality==0)
		{
			//向数据库中更新tag点的值;
			int lRet = UpdataTagValue(curTag, strTime, strValue, nTagNum);
			nTagNum++;
			if(lRet<0)
			{
				PKLog.LogMessage(PK_LOGLEVEL_INFO,"Update tag value failed.");
				return PK_FAILURE;
			}
		}
		else
		{
			PKLog.LogMessage(PK_LOGLEVEL_INFO,"Can not find tag value...,tagName=%s",curStr.c_str());
			nTagNum++; //如果不需要取值则跳过该点
			return PK_FAILURE;
		} 
	}
	//将所有点都赋值一遍之后再置为false
	bIsFirstTime = false;
	return 0;
}


/**
*  功能：更新点值的操作
*  @param    -[in]	vector<CTagInfo >::iterator m_vec: [Tag点名称]
*  @param    -[in]	string strTime: [时间]
*  @param    -[in,out]	string strValue: [值]
*  @param    -[in,out]	int nTagNum: [序号]
*
*  @return   ICV_SUCCESS/ICV_FAILURE.
*
*  @version   04/10/2017  xingxing Initial Version.
*/
int CMainTask::UpdataTagValue(vector<CTagInfo >::iterator m_vec, string strTime, string strValue, int nTagNum)
{
	if (bIsFirstTime)
	{
		InitAllTableShows(m_vec, strTime, strValue, nTagNum);
		ParsePreTime((char *)strTime.c_str(), nTagNum);
	}
	else
	{
		ParseNowTime((char *)strTime.c_str(), nTagNum);

		int nNowTime = (int)StringToDatetime(strTime.c_str());
		UpdateRealTable(m_vec, strTime, strValue,nTagNum);

		//设置更新类型，这里逻辑需要重新设计，目前存在问题
		if (nNowTime - (int)StringToDatetime(g_firstSecValueTime[nTagNum]) >= SECONDTIME)		//second
		{
			UpdateSecondTable(m_vec, strTime, strValue,nTagNum);
			strcpy(g_firstSecValueTime[nTagNum], strTime.c_str());
		}
		if (nNowTime - (int)StringToDatetime(g_firstMinValueTime[nTagNum]) >= MINUTETIME)		//minute
		{
			UpdateMinuteTable(m_vec, strTime, strValue, nTagNum);
			strcpy(g_firstMinValueTime[nTagNum],strTime.c_str());
		}

		if (nNowTime -(int)StringToDatetime(g_firstHourValueTime[nTagNum]) >= HOURTIME)	//hour
		{
			UpdateHourTable(m_vec, strTime, strValue, nTagNum);
			strcpy(g_firstHourValueTime[nTagNum], strTime.c_str());
		}

		if (nNowTime -(int)StringToDatetime(g_firstDayValueTime[nTagNum]) >= DAYTIME)	//day
		{
			UpdateDayTable(m_vec, strTime, strValue, nTagNum);
			strcpy(g_firstDayValueTime[nTagNum], strTime.c_str());
		}

		if (nNowTime -(int)StringToDatetime(g_firstDayValueTime[nTagNum]) >= MONTHTIME)	//month
		{
			UpdateDayTable(m_vec, strTime, strValue, nTagNum);
			strcpy(g_firstMonValueTime[nTagNum], strTime.c_str());
		}

		if (nNowTime -(int)StringToDatetime(g_firstDayValueTime[nTagNum]) >= YEARTIME)	//year
		{
			UpdateDayTable(m_vec, strTime, strValue, nTagNum);
			strcpy(g_firstYearValueTime[nTagNum], strTime.c_str());
		}
	}	
	return PK_SUCCESS;
}


/**
*  功能：初始化所有表格内容显示信息
*  @param    -[in]	vector<CTagInfo >::iterator m_vec: [Tag点名称]
*  @param    -[in]	string strTime: [时间]
*  @param    -[in,out]	string strValue: [值]
*  @param    -[in,out]	int nTagNum: [序号]
*
*  @return   ICV_SUCCESS/ICV_FAILURE.
*
*  @version   04/10/2017  xingxing Initial Version.
*/
void CMainTask::InitAllTableShows(vector<CTagInfo >::iterator m_vec, string strTime, string strValue, int nTagNum)
{
	//不同类型的点初始的时候都需要进行判断处理
	//char *pTemp = NULL;
	//strcpy(pTemp, strValue.c_str());
	//ParsePreTime(pTemp, nTagNum);
	//当前时间
	//g_nFirstTime[nTagNum] = (int)StringToDatetime(strTime.c_str());
	strcpy(g_nFirstTime[nTagNum], strTime.c_str());
	strcpy(g_nFirstValue[nTagNum],strValue.c_str());

	//初始化最大最小值
	strcpy(g_realTime[nTagNum], g_nFirstTime[nTagNum]);
	strcpy(g_realMaxValueTime[nTagNum], g_nFirstTime[nTagNum]);
	strcpy(g_realMinValueTime[nTagNum], g_nFirstTime[nTagNum]);
	strcpy(g_realTimeValue[nTagNum], g_nFirstValue[nTagNum]);
	strcpy(g_realMaxValue[nTagNum], g_nFirstValue[nTagNum]);
	strcpy(g_realMinValue[nTagNum], g_nFirstValue[nTagNum]);

	//更新实时表,不需要判断时间间隔,直接插入数据
	mongo::BSONObjBuilder MongReal;
	MongReal.append(TAGNAME, m_vec->szTagName);
	MongReal.append(TAGID, m_vec->nTagID);
	MongReal.append(REALVALUE , strValue.c_str());
	MongReal.append(REALVALUETIME, g_nFirstTime[nTagNum]);
	con.insert("PEAK.real", MongReal.obj());

	//更新秒表
	strcpy(g_maxSecValue[nTagNum], g_nFirstValue[nTagNum]);
	strcpy(g_minSecValue[nTagNum], g_nFirstValue[nTagNum]);
	strcpy(g_firstSecValue[nTagNum], g_nFirstValue[nTagNum]);
	strcpy(g_firstSecValueTime[nTagNum], g_nFirstTime[nTagNum]);
	strcpy(g_maxSecValueTime[nTagNum], g_nFirstTime[nTagNum]);
	strcpy(g_minSecValueTime[nTagNum], g_nFirstTime[nTagNum]);
	//写入数据库
	mongo::BSONObjBuilder MongSec;
	MongSec.append(TAGNAME, m_vec->szTagName);
	MongSec.append(TAGID, m_vec->nTagID);
	MongSec.append(MAXVALUE, g_maxSecValue[nTagNum]);
	MongSec.append(MAXVALUOFTIME, g_maxSecValueTime[nTagNum]);
	MongSec.append(MINVALUE, g_minSecValue[nTagNum]);
	MongSec.append(MINVALUEOFTIME, g_minSecValueTime[nTagNum]);
	MongSec.append(FIRSTVALUE, g_firstSecValue[nTagNum]);
	MongSec.append(FIRSTVALUETIME, g_firstSecValueTime[nTagNum]);
	con.insert("PEAK.second", MongSec.obj());

	//更新分钟表
	strcpy(g_maxMinValue[nTagNum],g_nFirstValue[nTagNum]);
	strcpy(g_minMinValue[nTagNum],g_nFirstValue[nTagNum]);
	strcpy(g_firstMinValue[nTagNum],g_nFirstValue[nTagNum]);
	strcpy(g_firstMinValueTime[nTagNum],g_nFirstTime[nTagNum]);
	strcpy(g_maxMinValueTime[nTagNum],g_nFirstTime[nTagNum]);
	strcpy(g_minMinValueTime[nTagNum],g_nFirstTime[nTagNum]);
	//写入数据库
	mongo::BSONObjBuilder MongMin;
	MongMin.append(TAGNAME, m_vec->szTagName);
	MongMin.append(TAGID, m_vec->nTagID);
	MongMin.append(MAXVALUE, g_maxMinValue[nTagNum]);
	MongMin.append(MAXVALUOFTIME, g_maxMinValueTime[nTagNum]);
	MongMin.append(MAXVALUE, g_minMinValue[nTagNum]);
	MongMin.append(MAXVALUOFTIME, g_minMinValueTime[nTagNum]);
	MongMin.append(FIRSTVALUE, g_firstMinValue[nTagNum]);
	MongMin.append(FIRSTVALUETIME, g_firstMinValueTime[nTagNum]);
	con.insert("PEAK.minute", MongMin.obj());

	//更新小时表
	strcpy(g_maxHourValue[nTagNum],g_nFirstValue[nTagNum]);
	strcpy(g_minHourValue[nTagNum],g_nFirstValue[nTagNum]);
	strcpy(g_firstHourValue[nTagNum], g_nFirstValue[nTagNum]);
	strcpy(g_firstHourValueTime[nTagNum],g_nFirstTime[nTagNum]);
	strcpy(g_maxHourValueTime[nTagNum],g_nFirstTime[nTagNum]);
	strcpy(g_minHourValueTime[nTagNum],g_nFirstTime[nTagNum]);
	//写入数据库
	mongo::BSONObjBuilder MongHour;
	MongHour.append(TAGNAME, m_vec->szTagName);
	MongHour.append(TAGID, m_vec->nTagID);
	MongHour.append(MAXVALUE, g_maxHourValue[nTagNum]);
	MongHour.append(MAXVALUOFTIME, g_maxHourValueTime[nTagNum]);
	MongHour.append(MINVALUE, g_minHourValue[nTagNum]);
	MongHour.append(MINVALUEOFTIME, g_minHourValueTime[nTagNum]);
	MongHour.append(FIRSTVALUE, g_firstHourValue[nTagNum]);
	MongHour.append(FIRSTVALUETIME, g_firstHourValueTime[nTagNum]);
	con.insert("PEAK.hour", MongHour.obj());

	//更新日表
	strcpy(g_maxDayValue[nTagNum],g_nFirstValue[nTagNum]);
	strcpy(g_minDayValue[nTagNum],g_nFirstValue[nTagNum]);
	strcpy(g_firstDayValue[nTagNum],g_nFirstValue[nTagNum]);
	strcpy(g_firstDayValueTime[nTagNum] ,g_nFirstTime[nTagNum]);
	strcpy(g_maxDayValueTime[nTagNum] ,g_nFirstTime[nTagNum]);
	strcpy(g_minDayValueTime[nTagNum] ,g_nFirstTime[nTagNum]);
	//写入数据库
	mongo::BSONObjBuilder MongDay;
	MongDay.append(TAGNAME, m_vec->szTagName);
	MongDay.append(TAGID, m_vec->nTagID);
	MongDay.append(MAXVALUE, g_maxDayValue[nTagNum]);
	MongDay.append(MAXVALUOFTIME, g_maxDayValueTime[nTagNum]);
	MongDay.append(MINVALUE, g_minDayValue[nTagNum]);
	MongDay.append(MINVALUEOFTIME, g_minDayValueTime[nTagNum]);
	MongDay.append(FIRSTVALUE, g_firstDayValue[nTagNum]);
	MongDay.append(FIRSTVALUETIME, g_firstDayValueTime[nTagNum]);
	con.insert("PEAK.day", MongDay.obj());

	//更新月表
	strcpy(g_maxMonValue[nTagNum],g_nFirstValue[nTagNum]);
	strcpy(g_minMonValue[nTagNum],g_nFirstValue[nTagNum]);
	strcpy(g_firstMonValue[nTagNum],g_nFirstValue[nTagNum]);
	strcpy(g_firstMonValueTime[nTagNum],g_nFirstTime[nTagNum]);
	strcpy(g_maxMonValueTime[nTagNum],g_nFirstTime[nTagNum]);
	strcpy(g_minMonValueTime[nTagNum],g_nFirstTime[nTagNum]);
	//写入数据库
	mongo::BSONObjBuilder MongMonth;
	MongMonth.append(TAGNAME, m_vec->szTagName);
	MongMonth.append(TAGID, m_vec->nTagID);
	MongMonth.append(MAXVALUE, g_maxMonValue[nTagNum]);
	MongMonth.append(MAXVALUOFTIME, g_maxMonValueTime[nTagNum]);
	MongMonth.append(MINVALUE, g_minMonValue[nTagNum]);
	MongMonth.append(MINVALUEOFTIME, g_minMonValueTime[nTagNum]);
	MongMonth.append(FIRSTVALUE, g_firstMonValue[nTagNum]);
	MongMonth.append(FIRSTVALUETIME, g_firstMonValueTime[nTagNum]);
	con.insert("PEAK.month", MongMonth.obj());

	//更新年表
	//更新日表
	strcpy(g_maxYearValue[nTagNum],g_nFirstValue[nTagNum]);
	strcpy(g_minYearValue[nTagNum],g_nFirstValue[nTagNum]);
	strcpy(g_firstYearValue[nTagNum],g_nFirstValue[nTagNum]);
	strcpy(g_firstYearValueTime[nTagNum],g_nFirstTime[nTagNum]);
	strcpy(g_maxYearValueTime[nTagNum],g_nFirstTime[nTagNum]);
	strcpy(g_minYearValueTime[nTagNum],g_nFirstTime[nTagNum]);
	//写入数据库
	mongo::BSONObjBuilder MongYear;
	MongYear.append(TAGNAME, m_vec->szTagName);
	MongYear.append(TAGID, m_vec->nTagID);
	MongYear.append(MAXVALUE, g_maxYearValue[nTagNum]);
	MongYear.append(MAXVALUOFTIME, g_maxYearValueTime[nTagNum]);
	MongYear.append(MINVALUE, g_minYearValue[nTagNum]);
	MongYear.append(MINVALUEOFTIME, g_minYearValueTime[nTagNum]);
	MongYear.append(FIRSTVALUE, g_firstYearValue[nTagNum]);
	MongYear.append(FIRSTVALUETIME, g_firstYearValueTime[nTagNum]);
	con.insert("PEAK.year", MongYear.obj());
}

/**
*  功能：更新实时表格的数据信息
*
*  @param    -[in]	vector<CTagInfo >::iterator m_vec: [Tag点名称]
*  @param    -[in]	string strTime: [时间]
*  @param    -[in,out]	string strValue: [值]
*  @param    -[in,out]	int nTagNum: [序号]
*
*  @return   ICV_SUCCESS/ICV_FAILURE.
*
*  @version   04/10/2017  xingxing Initial Version.
*/
void CMainTask::UpdateRealTable(vector<CTagInfo >::iterator m_vec,string strTime,string strValue,int nTagNum)
{
	//字符串转时间戳;
	//realTime[nTagNum] = (int)StringToDatetime(strTime.c_str())
	strcpy(g_realTime[nTagNum], strTime.c_str());
	strcpy(g_realTimeValue[nTagNum], strValue.c_str());

	//判断最大sec值和最小sec值;
	if (atoi(g_realMaxValue[nTagNum]) < atoi(g_realTimeValue[nTagNum]))
	{
		strcpy(g_realMaxValue[nTagNum], g_realTimeValue[nTagNum]);
		strcpy(g_realMaxValueTime[nTagNum], g_realTime[nTagNum]);
	}
	else if (atoi(g_realMinValue[nTagNum]) >= atoi(g_realTimeValue[nTagNum]))
	{
		strcpy(g_realMinValue[nTagNum], g_realTimeValue[nTagNum]);
		strcpy(g_realMinValueTime[nTagNum], g_realTime[nTagNum]);
	}

	//判断最大min值和最小min值
	if (atoi(g_maxMinValue[nTagNum]) < atoi(g_realTimeValue[nTagNum]))
	{
		strcpy(g_maxMinValue[nTagNum], g_realTimeValue[nTagNum]);
		strcpy(g_maxMinValueTime[nTagNum], g_realTime[nTagNum]);
	}
	else if (atoi(g_minMinValue[nTagNum]) >= atoi(g_realTimeValue[nTagNum]))
	{
		strcpy(g_minMinValue[nTagNum], g_realTimeValue[nTagNum]);
		strcpy(g_minMinValueTime[nTagNum], g_realTime[nTagNum]);
	}

	//判断最大Hour值和最小Hour值
	if (atoi(g_maxHourValue[nTagNum]) < atoi(g_realTimeValue[nTagNum]))
	{
		strcpy(g_maxHourValue[nTagNum], g_realTimeValue[nTagNum]);
		strcpy(g_maxHourValueTime[nTagNum], g_realTime[nTagNum]);
	}
	else if (atoi(g_minHourValue[nTagNum]) >= atoi(g_realTimeValue[nTagNum]))
	{
		strcpy(g_minHourValue[nTagNum], g_realTimeValue[nTagNum]);
		strcpy(g_minHourValueTime[nTagNum], g_realTime[nTagNum]);
	}

	//判断最大Day值和最小Day值
	if (atoi(g_maxDayValue[nTagNum]) < atoi(g_realTimeValue[nTagNum]))
	{
		strcpy(g_maxDayValue[nTagNum], g_realTimeValue[nTagNum]);
		strcpy(g_maxDayValueTime[nTagNum], g_realTime[nTagNum]);
	}
	else if (atoi(g_minDayValue[nTagNum]) >= atoi(g_realTimeValue[nTagNum]))
	{
		strcpy(g_minDayValue[nTagNum], g_realTimeValue[nTagNum]);
		strcpy(g_minDayValueTime[nTagNum], g_realTime[nTagNum]);
	}

	//判断最大month值和最小month值
	if (atoi(g_maxMonValue[nTagNum]) < atoi(g_realTimeValue[nTagNum]))
	{
		strcpy(g_maxMonValue[nTagNum], g_realTimeValue[nTagNum]);
		strcpy(g_maxMonValueTime[nTagNum], g_realTime[nTagNum]);
	}
	else if (atoi(g_minMonValue[nTagNum]) >= atoi(g_realTimeValue[nTagNum]))
	{
		strcpy(g_minMonValue[nTagNum], g_realTimeValue[nTagNum]);
		strcpy(g_minMonValueTime[nTagNum], g_realTime[nTagNum]);
	}

	//判断最大year值和最小year值
	if (atoi(g_maxYearValue[nTagNum]) < atoi(g_realTimeValue[nTagNum]))
	{
		strcpy(g_maxYearValue[nTagNum], g_realTimeValue[nTagNum]);
		strcpy(g_maxYearValueTime[nTagNum], g_realTime[nTagNum]);
	}
	else if (atoi(g_minYearValue[nTagNum]) >= atoi(g_realTimeValue[nTagNum]))
	{
		strcpy(g_minYearValue[nTagNum], g_realTimeValue[nTagNum]);
		strcpy(g_minYearValueTime[nTagNum], g_realTime[nTagNum]);
	}
	//更新实时值
	mongo::BSONObjBuilder oBOJCharacter;
	oBOJCharacter.append(TAGNAME, m_vec->szTagName);
	oBOJCharacter.append(TAGID, m_vec->nTagID);
	oBOJCharacter.append(REALVALUE, g_realTimeValue[nTagNum]);
	//oBOJCharacter.append(REALVALUETIME, strTime);
	oBOJCharacter.append(REALVALUETIME, g_realTime[nTagNum]);
	con.insert("PEAK.real", oBOJCharacter.obj());
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "实时数据更新完成.tagName=%s",m_vec->szTagName);
}

/**
*  功能：更新秒表的数据显示
*
*  @param    -[in]	vector<CTagInfo >::iterator m_vec: [Tag点名称]
*  @param    -[in]	string strTime: [时间]
*  @param    -[in,out]	string strValue: [值]
*  @param    -[in,out]	int nTagNum: [序号]
*
*  @return   ICV_SUCCESS/ICV_FAILURE.
*
*  @version   04/10/2017  xingxing Initial Version.
*/
void CMainTask::UpdateSecondTable(vector<CTagInfo >::iterator m_vec,string strTime,string strValue,int nTagNum)
{
	mongo::BSONObjBuilder oBChar;
	oBChar.append(TAGNAME, m_vec->szTagName);
	oBChar.append(TAGID, m_vec->nTagID);
	oBChar.append(MAXVALUE, g_maxSecValue[nTagNum]);
	oBChar.append(MAXVALUOFTIME, g_maxSecValueTime[nTagNum]);
	oBChar.append(MINVALUE, g_minSecValue[nTagNum]);
	oBChar.append(MAXVALUOFTIME, g_minSecValueTime[nTagNum]);
	oBChar.append(FIRSTVALUE, g_firstSecValue[nTagNum]);
	oBChar.append(FIRSTVALUETIME, g_firstSecValueTime[nTagNum]);
	con.insert("PEAK.second", oBChar.obj());	

	strcpy(g_firstSecValueTime[nTagNum], strTime.c_str());
	strcpy(g_firstSecValue[nTagNum], strValue.c_str());
	//更新下个周期的最大最小值起始值
	strcpy(g_maxSecValue[nTagNum], g_firstSecValue[nTagNum]);
	strcpy(g_maxSecValueTime[nTagNum], g_firstSecValueTime[nTagNum]);
	strcpy(g_minSecValue[nTagNum], g_firstSecValue[nTagNum]);
	strcpy(g_minSecValueTime[nTagNum], g_firstSecValueTime[nTagNum]);
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "秒级数据更新完成.tagName=%s", m_vec->szTagName);
}

/**
*  功能：更新分钟表的数据显示
*
*  @param    -[in]	vector<CTagInfo >::iterator m_vec: [Tag点名称]
*  @param    -[in]	string strTime: [时间]
*  @param    -[in,out]	string strValue: [值]
*  @param    -[in,out]	int nTagNum: [序号]
*
*  @return   ICV_SUCCESS/ICV_FAILURE.
*
*  @version   04/10/2017  xingxing Initial Version.
*/
void CMainTask::UpdateMinuteTable(vector<CTagInfo >::iterator m_vec, string strTime, string strValue, int nTagNum)
{
	//每个字段进行赋值操作
	mongo::BSONObjBuilder oBChar;
	oBChar.append(TAGNAME, m_vec->szTagName);
	oBChar.append(TAGID, m_vec->nTagID);
	oBChar.append(MAXVALUE, g_maxMinValue[nTagNum]);
	oBChar.append(MAXVALUOFTIME, g_maxMinValueTime[nTagNum]);
	oBChar.append(MINVALUE, g_minMinValue[nTagNum]);
	oBChar.append(MINVALUEOFTIME, g_minMinValueTime[nTagNum]);
	oBChar.append(FIRSTVALUE, g_firstMinValue[nTagNum]);
	oBChar.append(FIRSTVALUETIME, g_firstMinValueTime[nTagNum]);
	con.insert("PEAK.minute", oBChar.obj());

	strcpy(g_firstMinValue[nTagNum], strValue.c_str());
	strcpy(g_firstMinValueTime[nTagNum], strTime.c_str());
	//g_firstMinValueTime[nTagNum] = (int)StringToDatetime(strTime.c_str());
	//下个周期的起始值
	strcpy(g_maxMinValue[nTagNum], g_firstMinValue[nTagNum]);
	strcpy(g_minMinValue[nTagNum], g_firstMinValue[nTagNum]);
	strcpy(g_maxMinValueTime[nTagNum],g_firstMinValueTime[nTagNum]);
	strcpy(g_minMinValueTime[nTagNum],g_firstMinValueTime[nTagNum]);
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "分钟级数据更新完成.tagName=%s", m_vec->szTagName);
}

/**
*  功能：更新小时表的数据显示
*
*  @param    -[in]	vector<CTagInfo >::iterator m_vec: [Tag点名称]
*  @param    -[in]	string strTime: [时间]
*  @param    -[in,out]	string strValue: [值]
*  @param    -[in,out]	int nTagNum: [序号]
*
*  @return   ICV_SUCCESS/ICV_FAILURE.
*
*  @version   04/10/2017  xingxing Initial Version.
*/
void CMainTask::UpdateHourTable(vector<CTagInfo >::iterator m_vec, string strTime, string strValue, int nTagNum)
{
	mongo::BSONObjBuilder oBChar;
	oBChar.append(TAGNAME, m_vec->szTagName);
	oBChar.append(TAGID, m_vec->nTagID);
	oBChar.append(MAXVALUE, g_maxHourValue[nTagNum]);
	oBChar.append(MAXVALUOFTIME, g_maxHourValueTime[nTagNum]);
	oBChar.append(MINVALUE, g_minHourValue[nTagNum]);
	oBChar.append(MINVALUEOFTIME, g_minHourValueTime[nTagNum]);
	oBChar.append(FIRSTVALUE, g_firstHourValue[nTagNum]);
	oBChar.append(FIRSTVALUETIME, g_firstHourValueTime[nTagNum]);
	con.insert("PEAK.hour", oBChar.obj());

	strcpy(g_firstHourValue[nTagNum], strValue.c_str());
	strcpy(g_firstHourValueTime[nTagNum], strTime.c_str());
	//g_firstHourValueTime[nTagNum] = StringToDatetime(strTime.c_str());
	//更新下个周期的起始值
	strcpy(g_maxHourValue[nTagNum] ,g_firstHourValue[nTagNum]);
	strcpy(g_minHourValue[nTagNum] , g_firstHourValue[nTagNum]);
	strcpy(g_maxHourValueTime[nTagNum] ,g_firstHourValueTime[nTagNum]);
	strcpy(g_minHourValueTime[nTagNum] ,g_firstHourValueTime[nTagNum]);
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "小时级数据更新完成.tagName=%s", m_vec->szTagName);
}

/**
*  功能：更新日表的数据显示
*
*  @param    -[in]	vector<CTagInfo >::iterator m_vec: [Tag点名称]
*  @param    -[in]	string strTime: [时间]
*  @param    -[in,out]	string strValue: [值]
*  @param    -[in,out]	int nTagNum: [序号]
*
*  @return   ICV_SUCCESS/ICV_FAILURE.
*
*  @version   04/10/2017  xingxing Initial Version.
*/
void CMainTask::UpdateDayTable(vector<CTagInfo >::iterator m_vec, string strTime, string strValue, int nTagNum)
{
	//InitMemoryAllocation(nTagNum);
	//每个字段进行赋值操作
	mongo::BSONObjBuilder oBChar;
	oBChar.append(TAGNAME, m_vec->szTagName);
	oBChar.append(TAGID, m_vec->nTagID);
	oBChar.append(MAXVALUE, g_maxDayValue[nTagNum]);
	oBChar.append(MAXVALUOFTIME, g_maxDayValueTime[nTagNum]);
	oBChar.append(MINVALUE, g_minDayValue[nTagNum]);
	oBChar.append(MINVALUEOFTIME, g_minDayValueTime[nTagNum]);
	oBChar.append(FIRSTVALUE, g_firstDayValue[nTagNum]);
	oBChar.append(FIRSTVALUETIME, g_firstDayValueTime[nTagNum]);
	con.insert("PEAK.day", oBChar.obj());

	strcpy(g_firstDayValue[nTagNum], strValue.c_str());
	strcpy(g_firstDayValueTime[nTagNum], strTime.c_str());
	//g_firstDayValueTime[nTagNum],StringToDatetime(strTime.c_str());
	//更新下个周期的起始值
	strcpy(g_maxDayValue[nTagNum],g_firstDayValue[nTagNum]);
	strcpy(g_minDayValue[nTagNum],g_firstDayValue[nTagNum]);
	strcpy(g_maxDayValueTime[nTagNum],g_firstDayValueTime[nTagNum]);
	strcpy(g_minDayValueTime[nTagNum],g_firstDayValueTime[nTagNum]);
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "天级数据更新完成.tagName=%s", m_vec->szTagName);
}

/**
*  功能：更新月表的数据显示
*
*  @param    -[in]	vector<CTagInfo >::iterator m_vec: [Tag点名称]
*  @param    -[in]	string strTime: [时间]
*  @param    -[in,out]	string strValue: [值]
*  @param    -[in,out]	int nTagNum: [序号]
*
*  @return   ICV_SUCCESS/ICV_FAILURE.
*
*  @version   04/10/2017  xingxing Initial Version.
*/
void CMainTask::UpdateMonthTable(vector<CTagInfo >::iterator m_vec, string strTime, string strValue, int nTagNum)
{
	//InitMemoryAllocation(nTagNum);
	//每个字段进行赋值操作
	mongo::BSONObjBuilder oBChar;
	oBChar.append(TAGNAME, m_vec->szTagName);
	oBChar.append(TAGID, m_vec->nTagID);
	oBChar.append(MAXVALUE, g_maxMonValue[nTagNum]);
	oBChar.append(MAXVALUOFTIME, g_maxMonValueTime[nTagNum]);
	oBChar.append(MINVALUE, g_minMonValue[nTagNum]);
	oBChar.append(MINVALUEOFTIME, g_minMonValueTime[nTagNum]);
	oBChar.append(FIRSTVALUE, g_firstMonValue[nTagNum]);
	oBChar.append(FIRSTVALUETIME, g_firstMonValueTime[nTagNum]);
	con.insert("PEAK.month", oBChar.obj());

	strcpy(g_firstMonValue[nTagNum], strValue.c_str());
	strcpy(g_firstMonValueTime[nTagNum], strTime.c_str());
	//g_firstMonValueTime[nTagNum] = StringToDatetime(strTime.c_str());
	//更新下个周期的起始值
	strcpy(g_maxMonValue[nTagNum] ,g_firstMonValue[nTagNum]);
	strcpy(g_minMonValue[nTagNum] ,g_firstMonValue[nTagNum]);
	strcpy(g_maxMonValueTime[nTagNum], g_firstMonValueTime[nTagNum]);
	strcpy(g_minMonValueTime[nTagNum], g_firstMonValueTime[nTagNum]);
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "月级数据更新完成.tagName=%s", m_vec->szTagName);
	return;
}

/**
*  功能：更新年表的数据显示
*
*  @param    -[in]	vector<CTagInfo >::iterator m_vec: [Tag点名称]
*  @param    -[in]	string strTime: [时间]
*  @param    -[in,out]	string strValue: [值]
*  @param    -[in,out]	int nTagNum: [序号]
*
*  @return   ICV_SUCCESS/ICV_FAILURE.
*
*  @version   04/10/2017  xingxing Initial Version.
*/
void CMainTask::UpdateYearTable(vector<CTagInfo >::iterator m_vec, string strTime, string strValue, int nTagNum)
{
	//InitMemoryAllocation(nTagNum);
	//每个字段进行赋值操作
	mongo::BSONObjBuilder oBChar;
	oBChar.append(TAGNAME, m_vec->szTagName);
	oBChar.append(TAGID, m_vec->nTagID);
	oBChar.append(MAXVALUE, g_maxYearValue[nTagNum]);
	oBChar.append(MAXVALUOFTIME, g_maxYearValueTime[nTagNum]);
	oBChar.append(MINVALUE, g_minYearValue[nTagNum]);
	oBChar.append(MINVALUEOFTIME, g_minYearValueTime[nTagNum]);
	oBChar.append(FIRSTVALUE, g_firstYearValue[nTagNum]);
	oBChar.append(FIRSTVALUETIME, g_firstYearValueTime[nTagNum]);
	con.insert("PEAK.month", oBChar.obj());

	strcpy(g_firstYearValue[nTagNum], strValue.c_str());
	strcpy(g_firstYearValueTime[nTagNum], strTime.c_str());
	//g_firstYearValueTime[nTagNum] = StringToDatetime(strTime.c_str());
	//更新下个周期的起始值
	strcpy(g_maxYearValue[nTagNum] ,g_firstYearValue[nTagNum]);
	strcpy(g_minYearValue[nTagNum] ,g_firstYearValue[nTagNum]);
	strcpy(g_maxYearValueTime[nTagNum],g_firstYearValueTime[nTagNum]);
	strcpy(g_minYearValueTime[nTagNum],g_firstYearValueTime[nTagNum]);
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "年级数据更新完成.tagName=%s", m_vec->szTagName);
	return;
}

/**
*  功能：初始化MongoDb数据库
*
*  @version   04/10/2017  xingxing Initial Version.
*/
int CMainTask::InitMongdb()
{
	WSAData wsaData;
	if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)
	{
		return -1;
	}

	try
	{
		mongo::client::initialize();
		con.connect(strConMongoServer.c_str());
		cout << "connect to mongodb successfully" << endl;
	}
	catch (const mongo::DBException &e)
	{
		cout << "caught " << e.what() << endl;
	}
//	getchar();
	return PK_SUCCESS;
}

/**
*  功能：将时间格式从string转换成整形数
*
*  @version   04/10/2017  xingxing Initial Version.
*/
time_t StringToDatetime(const char *str)  
{  
	tm tm_;  
	int year, month, day, hour, minute,second;  
	sscanf(str,"%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);  
	tm_.tm_year  = year-1900;  
	tm_.tm_mon   = month-1;  
	tm_.tm_mday  = day;  
	tm_.tm_hour  = hour;  
	tm_.tm_min   = minute;  
	tm_.tm_sec   = second;  
	tm_.tm_isdst = 0;  

	time_t t_ = mktime(&tm_); //已经减了8个时区  
	return t_; //秒时间;  
}

/**
*  功能：解析显示时间各个字段
*
*  @version   04/10/2017  xingxing Initial Version.
*/
void ParsePreTime(char *str,int n)
{
	sscanf(str, "%d-%d-%d %d:%d:%d", &PreTime.nYear[n], &PreTime.nMonth[n], &PreTime.nDay[n], &PreTime.nHour[n], &PreTime.nMinute[n], &PreTime.nSecond[n]);
}

/**
*  功能：解析显示时间各个字段
*
*  @version   04/10/2017  xingxing Initial Version.
*/
void ParseNowTime(char *str, int n)
{
	sscanf(str, "%d-%d-%d %d:%d:%d", &NowTime.nYear[n], &NowTime.nMonth[n], &NowTime.nDay[n], &NowTime.nHour[n], &NowTime.nMinute[n], &NowTime.nSecond[n]);
}

/*
*  功能：内存分配
*
*  @version   04 / 10 / 2017  xingxing Initial Version.
*/
void  CMainTask::InitMemoryAllocation(int nTagNum)
{
	//分配内存
	//实时
	g_nFirstValue[nTagNum] = new char[LENOFVALUE];
	g_realTimeValue[nTagNum] = new char[LENOFVALUE];
	g_realMaxValue[nTagNum] = new char[LENOFVALUE];
	g_realMinValue[nTagNum] = new char[LENOFVALUE];
	//秒
	g_maxSecValue[nTagNum] = new char[LENOFVALUE];
	g_minSecValue[nTagNum] = new char[LENOFVALUE];
	g_firstSecValue[nTagNum] = new char[LENOFVALUE];
	//分
	g_maxMinValue[nTagNum] = new char[LENOFVALUE];
	g_minMinValue[nTagNum] = new char[LENOFVALUE];
	g_firstMinValue[nTagNum] = new char[LENOFVALUE];
	//时
	g_maxHourValue[nTagNum] = new char[LENOFVALUE];
	g_minHourValue[nTagNum] = new char[LENOFVALUE];
	g_firstHourValue[nTagNum] = new char[LENOFVALUE];
	//日
	g_maxDayValue[nTagNum] = new char[LENOFVALUE];
	g_minDayValue[nTagNum] = new char[LENOFVALUE];
	g_firstDayValue[nTagNum] = new char[LENOFVALUE];
	//月
	g_maxMonValue[nTagNum] = new char[LENOFVALUE];
	g_minMonValue[nTagNum] = new char[LENOFVALUE];
	g_firstMonValue[nTagNum] = new char[LENOFVALUE];
	//年
	g_maxYearValue[nTagNum] = new char[LENOFVALUE];
	g_minYearValue[nTagNum] = new char[LENOFVALUE];
	g_firstYearValue[nTagNum] = new char[LENOFVALUE];

	//分配内存
	//实时
	g_nFirstTime[nTagNum] = new char[LENOFVALUE];
	g_realTime[nTagNum] = new char[LENOFVALUE];
	g_realMaxValueTime[nTagNum] = new char[LENOFVALUE];
	g_realMinValueTime[nTagNum] = new char[LENOFVALUE];
	//秒
	g_maxSecValueTime[nTagNum] = new char[LENOFVALUE];
	g_minSecValueTime[nTagNum] = new char[LENOFVALUE];
	g_firstSecValueTime[nTagNum] = new char[LENOFVALUE];
	//分
	g_maxMinValueTime[nTagNum] = new char[LENOFVALUE];
	g_minMinValueTime[nTagNum] = new char[LENOFVALUE];
	g_firstMinValueTime[nTagNum] = new char[LENOFVALUE];
	//时
	g_maxHourValueTime[nTagNum] = new char[LENOFVALUE];
	g_minHourValueTime[nTagNum] = new char[LENOFVALUE];
	g_firstHourValueTime[nTagNum] = new char[LENOFVALUE];
	//日
	g_maxDayValueTime[nTagNum] = new char[LENOFVALUE];
	g_minDayValueTime[nTagNum] = new char[LENOFVALUE];
	g_firstDayValueTime[nTagNum] = new char[LENOFVALUE];
	//月
	g_maxMonValueTime[nTagNum] = new char[LENOFVALUE];
	g_minMonValueTime[nTagNum] = new char[LENOFVALUE];
	g_firstMonValueTime[nTagNum] = new char[LENOFVALUE];
	//年
	g_maxYearValueTime[nTagNum] = new char[LENOFVALUE];
	g_minYearValueTime[nTagNum] = new char[LENOFVALUE];
	g_firstYearValueTime[nTagNum] = new char[LENOFVALUE];
}

/**
*  功能：释放内存
*
*  @version   04/10/2017  xingxing Initial Version.
*/
void  CMainTask::UnInitMemoryAllocation(int nTagNum)
{
	//time
	delete[] g_nFirstTime[nTagNum];
	delete[] g_realTime[nTagNum];
	delete[] g_realMaxValueTime[nTagNum];
	delete[] g_realMinValueTime[nTagNum];

	delete[] g_maxSecValueTime[nTagNum];
	delete[] g_minSecValueTime[nTagNum];
	delete[] g_firstSecValueTime[nTagNum];
	
	delete[] g_maxMinValueTime[nTagNum];
	delete[] g_minMinValueTime[nTagNum];
	delete[] g_firstMinValueTime[nTagNum];
	
	delete[] g_maxHourValueTime[nTagNum];
	delete[] g_minHourValueTime[nTagNum];
	delete[] g_firstHourValueTime[nTagNum];
	
	delete[] g_maxDayValueTime[nTagNum];
	delete[] g_minDayValueTime[nTagNum];
	delete[] g_firstDayValueTime[nTagNum];

	delete[] g_maxMonValueTime[nTagNum];
	delete[] g_minMonValueTime[nTagNum];
	delete[] g_firstMonValueTime[nTagNum];

	delete[] g_maxYearValueTime[nTagNum];
	delete[] g_minYearValueTime[nTagNum];
	delete[] g_firstYearValueTime[nTagNum];

	//value
	delete[] g_nFirstValue[nTagNum];
	delete[] g_realTimeValue[nTagNum];
	delete[] g_realMaxValue[nTagNum];
	delete[] g_realMinValue[nTagNum];

	delete[] g_maxSecValue[nTagNum];
	delete[] g_minSecValue[nTagNum];
	delete[] g_firstSecValue[nTagNum];

	delete[] g_maxMinValue[nTagNum];
	delete[] g_minMinValue[nTagNum];
	delete[] g_firstMinValue[nTagNum];

	delete[] g_maxHourValue[nTagNum];
	delete[] g_minHourValue[nTagNum];
	delete[] g_firstHourValue[nTagNum];

	delete[] g_maxDayValue[nTagNum];
	delete[] g_minDayValue[nTagNum];
	delete[] g_firstDayValue[nTagNum];

	delete[] g_maxMonValue[nTagNum];
	delete[] g_minMonValue[nTagNum];
	delete[] g_firstMonValue[nTagNum];

	delete[] g_maxYearValue[nTagNum];
	delete[] g_minYearValue[nTagNum];
	delete[] g_firstYearValue[nTagNum];
}


string trime(const string& str)
{
	string::size_type pos = str.find_first_not_of(' ');
	if (pos == string::npos)
	{
		return str;
	}
	string::size_type pos2 = str.find_last_not_of(' ');
	if (pos2 != string::npos)
	{
		return str.substr(pos, pos2 - pos + 1);
	}
	return str.substr(pos);
}


/**
*  功能：读取配置文件
*
*  @version   04/10/2017  xingxing Initial Version.
*/
void CMainTask::readConfig()
{
	const char * szConfigPath = "..\\..\\..\\..\\config";
	string strConfigPath = szConfigPath;
	strConfigPath = strConfigPath + PK_DIRECTORY_SEPARATOR_STR + "db.conf";

	//std::ifstream textFile(strConfigPath.c_str());
	std::ifstream textFile(strConfigPath.c_str());
	if (!textFile.is_open())
	{
		int	nTimeOut = 3;
		string strConfPath = "..\\..\\..\\config";
		return;
	}
	// 计算文件大小;  
	textFile.seekg(0, std::ios::end);
	std::streampos len = textFile.tellg();
	textFile.seekg(0, std::ios::beg);

	// 方法1  
	std::string strConfigData((std::istreambuf_iterator<char>(textFile)), std::istreambuf_iterator<char>());
	textFile.close();
	string strConnStr = "";
	string strUserName = "";
	string strPassword = "";
	string strDbType = "";
	string strTimeout = "";
	string strCodingSet = "";
	while (strConfigData.length() > 0)
	{
		string strLine = "";
		int nPosLine = strConfigData.find('\n');
		if (nPosLine > 0)
		{
			strLine = strConfigData.substr(0, nPosLine);
			strConfigData = strConfigData.substr(nPosLine + 1);
		}
		else
		{
			strLine = strConfigData;
			strConfigData = "";
		}
		strLine = trime(strLine);
		if (strLine.empty())
			continue;
		// x=y
		int nPosSection = strLine.find('=');
		if (nPosSection <= 0)
		{
			continue;
		}
		string strSecName = strLine.substr(0, nPosSection);
		string strSecValue = strLine.substr(nPosSection + 1);
		strSecName = trime(strSecName);
		strSecValue = trime(strSecValue);
		std::transform(strSecName.begin(), strSecName.end(), strSecName.begin(), ::tolower);
		if (strSecName.compare("dbtype") == 0)
			strDbType = strSecValue;
		else if (strSecName.compare("connection") == 0)
			strConnStr = strSecValue;
		else if (strSecName.compare("username") == 0)
			strUserName = strSecValue;
		else if (strSecName.compare("password") == 0)
			strPassword = strSecValue;
		else if (strSecName.compare("timeout") == 0)
			strTimeout = strSecValue;
		else if (strSecName.compare("codingset") == 0)
			strCodingSet = strSecValue;
		else if (strSecName.compare("connserver") == 0)
			strConMongoServer = strSecValue;
		else
		{
			continue;
		}
	}
	//strncpy(szConnServer, strConMongoServer.c_str(),sizeof(strConMongoServer.c_str()));
}

//char * DatetimeToString(time_t tt)
//{
//	char *now;
//	struct tm *ttime;
//	ttime = localtime(&tt);
//	strftime(now, 64, "%Y-%m-%d %H:%M:%S", ttime);
//	cout << "the time is " << now << endl;
//	return NULL;
//}

