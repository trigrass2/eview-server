#pragma once		 
#include "ace/Time_Value.h"   
#include "ace/Log_Msg.h"   
#include "ace/Synch.h"   
#include "ace/Reactor.h"   
#include "ace/Event_Handler.h"  
#include "ace/Select_Reactor.h"
#include <ace/Task.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "json/json.h"
#include "pkmemdbapi/pkmemdbapi.h"
#include "pkeviewdbapi/pkeviewdbapi.h"
#include "mongo/client/dbclient.h"
#include "global.h"

using namespace std;
using namespace mongo;

class CRecordTimerTask;
class CRecordTimer : public  ACE_Event_Handler
{
public:
	CRecordTimer();
	~CRecordTimer();	 

public:
	int ProcessTags(ACE_Time_Value tvTimer);

public:
	ACE_Reactor* m_pReactor;
	map<int, CRecordTimer *> m_mapInterval2TagGroupTimer;
	CRecordTimerTask * m_pTimerTask;

public:
	int m_nIntervalMS;
	bool m_bFirstTimerCome; // �Ƿ��ǵ�һ�ζ�ʱ���������ǵ�һ�Σ���ô���ܱ������ݣ���Ϊ������������
	vector<CTagInfo*>	m_vecTags;
	vector<string>		m_vecTagName; // Ҫ�����ڴ����ݿ�������

public:
	virtual   int handle_timeout(const  ACE_Time_Value &current_time, const   void  *act  /* = 0 */);
	int  Start();
	int  Stop();

public:
	void SetTimerTask(CRecordTimerTask *pMainTask);
	int RealData_TO_Table_Record(vector<BSONObj> &vecBsonObj, string &strTagName, string &strNewRecTime, string &strNewRecValue, int nNewRecQuality);
};
