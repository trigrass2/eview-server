/**************************************************************
 *  Filename:    RMService.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: RM Service Object.
 *
 *  @author:     chenzhiquan
 *  @version     05/20/2008  chenzhiquan  Initial Version
**************************************************************/
#ifndef _DBToEviewServer_H_
#define _DBToEviewServer_H_

#include <ace/Singleton.h>
#include "pkcomm/pkcomm.h"
#include "pkserver/PKServerBase.h"
#include "pklog/pklog.h"
#include <vector>
#include <string>
#include <map>
using namespace std;

class CDBToEviewServer : public CPKServerBase
{
public:
	CDBToEviewServer();
	virtual ~CDBToEviewServer();

	virtual int OnStart(int argc, ACE_TCHAR* args[]);
	virtual int OnStop();
	
private:

};

typedef ACE_Singleton<CDBToEviewServer, ACE_Null_Mutex> EVENT_SERVER;

#endif//DBToEviewServer
