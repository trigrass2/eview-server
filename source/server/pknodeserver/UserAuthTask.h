/**************************************************************
 *  Filename:    CtrlProcessTask.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: ��̬�����û�Ȩ����Ϣ���࣬����ÿ�β�ѯ���ݿ�ĵ�Ч��.
 *
 *  @author:     lijingjing
 *  @version     05/28/2008  lijingjing  Initial Version
 *  @version     01/28/2012  wanyingjie  Initial Version
**************************************************************/

#ifndef _USER_AUTH_TASK_H_
#define _USER_AUTH_TASK_H_

#include <ace/Task.h>
#include <string>
#include <map>

using namespace  std;

class CDataTag;
class CUserAuthTask : public ACE_Task<ACE_MT_SYNCH>
{
	friend class ACE_Singleton<CUserAuthTask, ACE_Thread_Mutex>;
public:
	CUserAuthTask();
	virtual ~CUserAuthTask();

	int Start();
	void Stop();

protected:
	bool			m_bStop;
	map<string, string> m_mapLoginName2RoleName;			// �û���¼��--��Ⱥ����
    map<string, map<string,bool> >	m_mapRoleName2SysNameMap;	// Ⱥ����+��ϵͳ��,true.û��Ȩ����鲻���������false
	ACE_Thread_Mutex	m_mutex;// ������������map��mutex

public:
	virtual int svc();

	// �̳߳�ʼ��
	int OnStart();
	// �߳���ֹͣ
	void OnStop();

	int LoadAuthFromDB();
	bool HasAuthByTagAndLoginName(string &strLoginName, CDataTag *pTag);
};

#define USERAUTH_TASK ACE_Singleton<CUserAuthTask, ACE_Thread_Mutex>::instance()

#endif  // _CTRL_PROC_TASK_H_
