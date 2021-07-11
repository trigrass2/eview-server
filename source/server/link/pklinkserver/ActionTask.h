/**************************************************************
 *  Filename:    ActionTask.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: ���ط���˶�����̬�����е��߳���.
 *
 *  @author:     zhaobin
 *  @version     01/06/2009  zhaobin  Initial Version
**************************************************************/
#ifndef _ACTIONTASK_INCLUDE_HEADER_
#define _ACTIONTASK_INCLUDE_HEADER_

#include "ace/Task.h"
#include "pkcomm/pkcomm.h"
#include "common/eviewdefine.h"

typedef long (*PFN_InitAction)(); //  ��ʼ��Action�ĺ���
typedef long (*PFN_ExitAction)(); // �˳�ʱ���õĺ���
// ����ִ�е��ú�����
// ע��, ����ֵ���Ƕ�������������ֵ�������ķ���ֵ�����ûص���������, ������Ϊ��Щ�����綨ʱ���첽���ؽ����, ͳһ�ڼ��ûص��������ؽ��
typedef long (*PFN_ExecuteAction)(char *szActionId, char *szTagNameList, char *szTagValue, char *szHoldSeconds, char *szActionParam4, char *szPriority, char* szJsonEventParams, char *szResult, int nResultBufLen);


class CActionTask : public ACE_Task<ACE_MT_SYNCH>   
{
public:
	CActionTask();
	virtual ~CActionTask();

	//�趨�̶߳�Ӧ�Ĵ���Դ����ID, ����Դ�жϵĶ�̬������
	void	SetActionInfo(const char *szActionName, const char* szDllName);
	int		Start();
	void	Stop();
    int		PostAction(char *szActionId, char *szActionParam, char *szActionParam2, char *szActionParam3, char *szActionParam4, char *szPriority, char* szJsonEventParams);

protected:
	bool m_bStop;
	bool m_bStopped;
	// �̳߳�ʼ��
	int OnStart();
	// �߳���ֹͣ
	void OnStop();
	//�߳���ں���
	int		svc();

public:
	//��̬������
	char	m_szActionTypeName[PK_LONGFILENAME_MAXLEN];
	char	m_szActionDllName[PK_LONGFILENAME_MAXLEN];
	//ACE�Ķ�̬�⣬�������ش���Դ�ж���̬��
	ACE_DLL				 m_dllAction;
	PFN_InitAction		m_pfnInitAction;
	PFN_ExitAction		m_pfnExitAction;
	PFN_ExecuteAction	m_pfnExecuteAction;
};


#endif // ifndef _ACTIONTASK_INCLUDE_HEADER_
