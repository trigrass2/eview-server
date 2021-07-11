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
#include "pkcomm/pkcomm.h"
#include "pkserver/PKServerBase.h"
#include "pklog/pklog.h"
#include "NodeCommonDef.h"
#include "TagExprCalc.h"
#include <vector>
#include <string>
#include <map>
using namespace std;

#define ACTIONNAME_

class CPKNodeServer : public CPKServerBase
{
public:
	CPKNodeServer();
	virtual ~CPKNodeServer();

	virtual int OnStart(int argc, ACE_TCHAR* args[]);
	virtual int OnStop();
	// 可以被子类调用的方法
	//int	UpdateServerStatus(int nStatus, char *szErrTip);
	//int UpdateServerStartTime();
	
private:
	string			m_strCfgFile;
	string			m_strHome;
};

#endif//_RM_SERVICE_H_
