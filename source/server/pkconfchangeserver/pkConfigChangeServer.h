/**************************************************************
 *  Filename:    RMService.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: RM Service Object.
 *
 *  @author:     chenzhiquan
 *  @version     05/20/2008  chenzhiquan  Initial Version
**************************************************************/
#ifndef _CONFIG_CHANGE_H_
#define _CONFIG_CHANGE_H_

#include <ace/Singleton.h>
#include "common/pkcomm.h"
#include "pkserver/ServerBase.h"
#include "pklog/pklog.h"
#include <vector>
#include <string>
#include <map>
using namespace std;


class CConfigChangeServer : public CServerBase
{
public:
	CConfigChangeServer();
	virtual ~CConfigChangeServer();

	virtual int OnStart(int argc, ACE_TCHAR* args[]);
	virtual int OnStop();
	// 可以被子类调用的方法
	int	UpdateServerStatus(int nStatus, char *szErrTip);
	int UpdateServerStartTime();
private:
	int LoadConfig();
};

typedef ACE_Singleton<CConfigChangeServer, ACE_Null_Mutex> CONFIGCHANGE_SERVER;

#endif//_CONFIG_CHANGE_H_
