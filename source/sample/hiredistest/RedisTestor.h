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
#include "common/ServerBase.h"

class CRedisTestor : public CServerBase
{
public:
	CRedisTestor();
	virtual ~CRedisTestor();

	virtual int OnStart(int argc, ACE_TCHAR* args[]);
	virtual int OnStop();
	
};

//typedef ACE_Singleton<CRedisTestor, ACE_Null_Mutex> RM_SERVICE;

#endif//_RM_SERVICE_H_
