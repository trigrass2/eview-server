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
#include "common/pkcomm.h"
#include "pkserver/ServerBase.h"
#include "pklog/pklog.h"
#include <vector>
#include <string>
#include <map>
using namespace std;

class CDataServer : public CServerBase
{
public:
	CDataServer();
	virtual ~CDataServer();

	virtual int OnStart(int argc, ACE_TCHAR* args[]);
	virtual int OnStop();
	// 可以被子类调用的方法
	int	UpdateServerStatus(int nStatus, char *szErrTip);
	int UpdateServerStartTime();
private:
	int LoadConfig();
	
private:

};

typedef ACE_Singleton<CDataServer, ACE_Null_Mutex> DATA_SERVER;

#endif//_RM_SERVICE_H_
