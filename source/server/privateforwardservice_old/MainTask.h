/**************************************************************
*  Filename:    RecvThread.h
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description:     接收消息
*
*  @author:     liuqifeng
*  @version     02/22/2009  liuqifeng  Initial Version
**************************************************************/
// RecvThread.h: interface for the RecvThread class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _RECVTHREAD_H_
#define _RECVTHREAD_H_

#include <ace/Task.h>
#include "ClientSockHandler.h"
#include <string>
#include "CycleReader.h"
#include "../forwardaccessor/ForwardAccessor.h"

typedef long (*PFN_Initialize)(vector<TAGGROUP *> *vecTagGroup);
typedef long (*PFN_ReadAllTagsData)(vector<TAGGROUP *> *vecTagGroup);
typedef long (*PFN_UnInitialize)();

class ACE_DLL;
class CMainTask  : public ACE_Task<ACE_MT_SYNCH>
{
	friend class ACE_Singleton<CMainTask, ACE_Thread_Mutex>;
public:
	CMainTask();
	virtual ~CMainTask();
	int Start();
	void Stop();
protected:
	virtual int svc(void); 
	long AddTags2Redis(map<string, DRVTAG*> mapNewToAdd);
	long AddTag2Redis(DRVTAG* pTag);
public: 
	ACE_Reactor* m_pReactor;
	CCycleReader m_cycleReader;
};

#define MAIN_TASK ACE_Singleton<CMainTask, ACE_Thread_Mutex>::instance()

#endif // !defined(AFX_TCPRECVTHREAD_H__520D8B7B_FC1A_46C3_9C65_A771090CEF98__INCLUDED_)
