/**************************************************************
 *  Filename:    RMService.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: RM Service Object.
 *
 *  @author:     chenzhiquan
 *  @version     05/20/2008  chenzhiquan  Initial Version
**************************************************************/
#ifndef _RM_SERVICE_H_
#define _RM_SERVICE_H_

#include <ace/Singleton.h>
#include "RMStatusCtrl.h"
#include "RMConfigLoader.h"
#include "errcode/ErrCode_iCV_RM.hxx"
#include "common/pkcomm.h"
#include "common/ServiceBase.h"

class CRMService : public CServiceBase
{
private:
	//friend class ACE_Singleton<CRMService, ACE_Null_Mutex>;

public:
	CRMService();
	virtual ~CRMService();

	virtual long Init(int argc, ACE_TCHAR* args[]);

	virtual long Start()
	{
		m_RMSCtrl.StartThread();
		return ICV_SUCCESS;
	}

	virtual long Fini()
	{
		m_RMSCtrl.StopThread();
		return ICV_SUCCESS;
	}

	virtual bool ProcessCmd(char c);

	virtual void PrintHelpScreen();

	virtual void Refresh();

	//void RunService();

	//void FiniService();
	//设置日志文件名称，保证打印ServiceBase里的日志到期望的文件中
	virtual void SetLogFileName();

private:
	void InteractiveLoop();
	long LoadConfig();
	long ParseOptions(int argc, ACE_TCHAR* args[]);
	
private:
	CRMStatusCtrl	m_RMSCtrl;
	string			m_strCfgFile;
	string			m_strHome;
	CRMConfigLoader m_cfgLoader;
};

//typedef ACE_Singleton<CRMService, ACE_Null_Mutex> RM_SERVICE;

#endif//_RM_SERVICE_H_
