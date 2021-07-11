/**************************************************************
 *  Filename:    TaskGroup.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 设备组信息实体类.
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

	// 启动线程
	int Start();
	// 停止线程
	void Stop();

	// Group线程启动时
	void OnStart();

	// Group线程停止时
	void OnStop();
	void DestroyDevices(); 

public:
	void PostGroupOnlineModifyCmd(CTaskGroup *pNewTaskGroup);
	void OnOnlineConfigModified(CTaskGroup *pNewGrp);
	void PostControlCmd(CDevice *pDevice, PKTAG *pTag, string strTagValue);
	virtual int handle_timeout(const ACE_Time_Value &current_time, const void *act);	// collect data
public:
	std::map<std::string, CDevice*> m_mapDevices;	// 设备列表
	CMainTask *m_pMainTask;

	char *m_pQueRecBuffer;	// 一次发送多个TAG点的内存指针
	unsigned int		m_nQueRecBufferLen;	// 一次发送多个TAG点的记录内存大小

	char *m_pTagVTQBuffer;	// 1个TAG点的内存指针
	unsigned int		m_nTagVTQBufferLen;	// 1个TAG点的内存大小

	char *m_pTagValueBuffer;	// 1个TAG点的内存指针
	unsigned int		m_nTagValueBufferLen;	// 1个TAG点的内存大小
public:
	virtual int handle_output (ACE_HANDLE fd = ACE_INVALID_HANDLE);
	int HandleWriteCmd();
	int UpdateTaskStatus(int lStatus, char *szErrorTip, char *szTime);
};
#endif  // _DEVICE_GROUP_H_
