/**************************************************************
 *  Filename:    RMService.h
 *  Copyright:   Shanghai Baosight Software Co., Ltd.
 *
 *  Description: RM Service Object.
 *
 *  @author:     chenzhiquan
 *  @version     05/20/2008  chenzhiquan  Initial Version
**************************************************************/
#pragma once

#include <ace/Singleton.h>
#include "pkcomm/pkcomm.h"
#include "pkserver/PKServerBase.h"
#include "pklog/pklog.h"
#include <vector>
#include <string>
#include <map>
#include "MainTask.h"

class CSyncToUpperNodeServer : public CPKServerBase
{
public:
	CSyncToUpperNodeServer();
    virtual ~CSyncToUpperNodeServer();

    // 必须实现的函数
    virtual int OnStart(int argc, char* args[]) ;
    virtual int OnStop() ;

private:
	int LoadConfig();
public:
    bool		m_bStop;

};
 
