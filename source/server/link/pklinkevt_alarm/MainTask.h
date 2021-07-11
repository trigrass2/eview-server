/**************************************************************
 *  Filename:    ReqFetchTask.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: interface for the CReqFetchTask class. 
 *   This class get a CVNDK-request from CVNDK queue and put to queue of Request Handler Task
 *
 *  @author:     shijunpu
 *  @version     05/29/2008  shijunpu    Initial Version
 *  @version     02/18/2009  wangjian    icv server first release
**************************************************************/

#ifndef _REQUESTFETCH_TASK_EVENT_REQEST_HANDLER_
#define _REQUESTFETCH_TASK_EVENT_REQEST_HANDLER_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <ace/Task.h>
#include "pkmemdbapi/pkmemdbapi.h"

using namespace std;
typedef long (*PFN_LinkEventCallback)(char* szEventId, char *szEventName, char *szPriority, char *szProduceTime,char *szEventParam);

typedef struct _LINK_EVENT
{
	string strId;
	string strName;
	string strSource;
	string strParam1;
	string strParam2;
	string strParam3;
	string strParam4;
	string strEventTypeName;
	string strDescription;
	string strSubsys;
	map<string, bool> mapAlarmName2Status;
	int    nAlarmingCountAsEvent;	// 几个报警产生才认为是事件发生
	int GetAlarmingCount()
	{
		int nAlarmingCount = 0;
		for (map<string, bool>::iterator it = mapAlarmName2Status.begin(); it != mapAlarmName2Status.end(); it++)
		{
			if (it->second)
				nAlarmingCount++;
		}
		return nAlarmingCount;
	}
	int IsEventTrigged()
	{
		int nAlarmingCount = GetAlarmingCount();
		if (nAlarmingCount < nAlarmingCountAsEvent)
			return false;
		else
			return true;
	}

	_LINK_EVENT()
	{
		nAlarmingCountAsEvent = 1;
	}
}LINK_EVENT;

class CMainTask : public ACE_Task<ACE_NULL_SYNCH>  
{
friend class ACE_Singleton<CMainTask, ACE_Null_Mutex>;

public:
	CMainTask();
	virtual ~CMainTask();

protected:
	bool				m_bShouldExit;
	CMemDBSubscriberSync 	m_memDBSubSync;
	int		ReadConfigFromDB();
	void ClearMap();

public:
	int		StartTask();
	int		StopTask();
	virtual int		svc();
	int OnRecvMessage(string &strMessage);
protected:

	map<string, LINK_EVENT *>	m_mapSingleAlmAction2Event; // 单个报警产生的事件 tag2.HH-alarm-produce---->pEvent
    vector<LINK_EVENT *>	m_vecMultiAlmEvent; // 一组报警中同时产生若干个才产生的事件 tag2.HH,tag2.LL-alarm-produce---->pEvent
    vector<LINK_EVENT *>	m_vecMultiEvent; // 短信事件
};
typedef ACE_Singleton<CMainTask, ACE_Null_Mutex> CReqFetchTaskSingleton;
#define MAIN_TASK CReqFetchTaskSingleton::instance()

#endif // _REQUESTFETCH_TASK_EVENT_REQEST_HANDLER_
