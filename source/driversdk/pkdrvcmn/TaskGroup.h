/**************************************************************
 *  Filename:    TaskGroup.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: �豸����Ϣʵ����.
 *
 *  @author:     lijingjing
 *  @version     05/28/2008  lijingjing  Initial Version
**************************************************************/
#ifndef _DEVICE_GROUP_H_
#define _DEVICE_GROUP_H_

#include "ace/Reactor.h"
#include "ace/Select_Reactor.h"
#include "ace/Acceptor.h"
#include <ace/SOCK_Acceptor.h>
#include <map>
#include <utility>

#include "drvframework.h"
#include "Device.h"
#include "common/CommHelper.h"

class CMainTask;
class CTaskGroup : public ACE_Task<ACE_MT_SYNCH>
{
public:
	ACE_Reactor*	m_pReactor; 
	virtual int svc ();
	bool m_bExit;
	CDrvObjectBase	m_drvObjBase;
public:
	CTaskGroup(const char *szGroupName, const char *szDescription);
	virtual ~CTaskGroup();

	// �����߳�
	int Start();
	// ֹͣ�߳�
	void Stop();

	// Group�߳�����ʱ
	void OnStart();

	// Group�߳�ֹͣʱ
	void OnStop();
	void DestroyDevices(); 

public:
	void PostGroupOnlineModifyCmd(CTaskGroup *pNewTaskGroup);
	void OnOnlineConfigModified(CTaskGroup *pNewGrp);
	void PostControlCmd(CDevice *pDevice, PKTAG *pTag, string strTagValue);
	virtual int handle_timeout(const ACE_Time_Value &current_time, const void *act);	// collect data
public:
	std::map<std::string, CDevice*> m_mapDevices;	// �豸�б�
	CMainTask *m_pMainTask;

	char *m_pQueRecBuffer;	// һ�η��Ͷ��TAG����ڴ�ָ��
	unsigned int		m_nQueRecBufferLen;	// һ�η��Ͷ��TAG��ļ�¼�ڴ��С

	char *m_pTagVTQBuffer;	// 1��TAG����ڴ�ָ��
	unsigned int		m_nTagVTQBufferLen;	// 1��TAG����ڴ��С

	char *m_pTagValueBuffer;	// 1��TAG����ڴ�ָ��
	unsigned int		m_nTagValueBufferLen;	// 1��TAG����ڴ��С
public:
	virtual int handle_output (ACE_HANDLE fd = ACE_INVALID_HANDLE);
	int HandleWriteCmd();
	int UpdateTaskStatus(int lStatus, char *szErrorTip, char *szTime);
};
#endif  // _DEVICE_GROUP_H_
