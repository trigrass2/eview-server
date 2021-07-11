/**************************************************************
 *  Filename:    CtrlProcessTask.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: �����������.
 *
 *  @author:     lijingjing
 *  @version     05/28/2008  lijingjing  Initial Version
 *  @version     01/28/2012  wanyingjie  Initial Version
**************************************************************/
#ifndef _CTRL_PROCESS_TASK_H_
#define _CTRL_PROCESS_TASK_H_
#include <ace/Task.h>
#include "tinyxml/tinyxml.h"
#include "json/json.h"
#include "pkmemdbapi/pkmemdbapi.h"
#include "driversdk/pkdrvcmn.h"
#include "pkeviewdbapi/pkeviewdbapi.h"

#include <map>
using namespace  std;
class CTaskGroup;
class CDevice;

#define TAGID "id"
#define TAGNAME "n"	
#define FIRSTVALUE "v"
#define FIRSTVALUETIME "t"
#define REALVALUE "v"
#define REALVALUETIME "t"
#define MAXVALUE "maxv"
#define MAXVALUOFTIME "maxt"
#define MINVALUE "minv"
#define MINVALUEOFTIME "mint"

#define SECONDTIME 1
#define MINUTETIME 60
#define HOURTIME 3600
#define DAYTIME  86400
#define MONTHTIME 2592000
#define YEARTIME  946080000


//CPKEviewDbApi PKDBConnect;

// ����tag������������Ϣ,�����ݸ���ֵʹ��;
typedef struct _CTagInfo
{
	int nTagID;  // tag��id;
	char szTagName[PK_NAME_MAXLEN];     // tag������;
	char szAddress[PK_NAME_MAXLEN];     // tag������;
	char szValue[PK_NAME_MAXLEN];	    // tag��ֵ;
	char szTagCname[PK_DESC_MAXLEN];	// tag��������;
	int nSubsysID;						// ��ϵͳid;
	char szSubsysname[PK_DESC_MAXLEN];  // ��ϵͳ��;
	map<string,string> mapVal2Lable;	// ��tag���ֵӳ�䵽����������֣���0--�رգ�1---��;
	_CTagInfo()
	{
		nTagID = -1;
		memset(szTagCname,0x00,sizeof(szTagName));
		memset(szAddress,0x00,sizeof(szAddress));
		memset(szValue,0x00,sizeof(szValue));
		memset(szTagCname,0x00,sizeof(szTagCname));
		memset(szSubsysname,0x00,sizeof(szSubsysname));
		mapVal2Lable.clear();
	}
}CTagInfo;


class CMainTask : public ACE_Task<ACE_MT_SYNCH>
{
	friend class ACE_Singleton<CMainTask, ACE_Thread_Mutex>;
public:
	CMainTask();
	virtual ~CMainTask();
	int Start();
	void Stop();

	int DealEvent();
	void readConfig();

	void InitMemoryAllocation(int nTagNum);
	void UnInitMemoryAllocation(int nTagNum);
	int UpdataTagValue(vector<CTagInfo >::iterator m_vec, string strTime, string strValue, int nTagNum);   //����tag���ֵ;

	//��ʼ��Mongodb;
	int InitMongdb();
	void InitAllTableShows(vector<CTagInfo >::iterator m_vec, string strTime, string strValue, int nTagNum);
	void UpdateRealTable(vector<CTagInfo >::iterator m_vec, string strTime, string strValue, int nTagNum);
	void UpdateSecondTable(vector<CTagInfo >::iterator m_vec, string strTime, string strValue, int nTagNum);
	void UpdateMinuteTable(vector<CTagInfo >::iterator m_vec, string strTime, string strValue, int nTagNum);
	void UpdateHourTable(vector<CTagInfo >::iterator m_vec, string strTime, string strValue, int nTagNum);
	void UpdateDayTable(vector<CTagInfo >::iterator m_vec, string strTime, string strValue, int nTagNum);
	void UpdateMonthTable(vector<CTagInfo >::iterator m_vec, string strTime, string strValue, int nTagNum);
	void UpdateYearTable(vector<CTagInfo >::iterator m_vec, string strTime, string strValue, int nTagNum);

public:
	bool			m_bStop;
	bool			m_bStopped;
	CMemDBClient	m_redisRW;

protected:
	virtual int svc();
	// �̳߳�ʼ��;
	int OnStart();
	// �߳���ֹͣ;
	void OnStop();
	// �������ļ����غ����¼������е�����theTaskGroupList;
	int LoadConfig();
	int InitRedis();
};

//string trim(const string& str);
void ParseNowTime(char *str, int n);
void ParsePreTime(char *str, int n);
time_t StringToDatetime(const char *str);

#define MAIN_TASK ACE_Singleton<CMainTask, ACE_Thread_Mutex>::instance()

#endif  // _CTRL_PROCESS_TASK_H_
