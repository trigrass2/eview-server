/**************************************************************
*  Filename:    MainTask.h
*  Copyright:   Shanghai peakinfo Software Co., Ltd.
*
*  Description: mqtt read redis and publish to topic.
*
*  @author:     yanchaodong
*  @version     04/16/2018  yanchaodong  Initial Version
**************************************************************/

#ifndef _EVIEWFORWARD_MAINTASK_H_
#define _EVIEWFORWARD_MAINTASK_H_

#include <ace/Task.h>
#include <string>
#include <map>
#include <vector>
#include "json/json.h"
#include "pkcomm/pkcomm.h"
#include "pkdbapi/pkdbapi.h"
#include "pkdata/pkdata.h"
#include "DbTransferDefine.h"

class CMainTask : public ACE_Task<ACE_MT_SYNCH>
{
	friend class ACE_Singleton<CMainTask, ACE_Thread_Mutex>;
public:
	CMainTask();
	virtual ~CMainTask();
	CPKDbApi	m_dbEview;

	int Start();
	void Stop();
protected:
    bool		m_bStop;

public:
	map<string, CDBConnection *>	m_mapId2DbConn;		// ���е����ݿ�����
	map<string, CDBTransferRule *>	m_mapId2TransferRule;	// ���е�ת������

public:
	virtual int svc();

	// �̳߳�ʼ��
	int OnStart();
	// �߳���ֹͣ
	int OnStop();


	int		LoadConfig();
	int		ReadAllTags(); 
	int		ReadDBConnList();
	int ReadTransferRuleList();
	int ReadTagList();
	int AssignRuleToTimerObject();

public:
	vector<string>		m_vecTagsName;
	string	m_strValueOnBadQuality_Int;	// ����ΪBADʱ��ֵ��ȱʡΪ???
	string	m_strValueOnBadQuality_Real;	// ����ΪBADʱ��ֵ��ȱʡΪ???
	string	m_strValueOnBadQuality_String;	// ����ΪBADʱ��ֵ��ȱʡΪ???
	string	m_strValueOnBadQuality_DateTime;	// ����ΪBADʱ��ֵ��ȱʡΪ???
};

// ����tag������������Ϣ,����¼��־��ʹ��

#define MAIN_TASK ACE_Singleton<CMainTask, ACE_Thread_Mutex>::instance()

#endif  // _ALARM_ROUTE_TASK_H_
