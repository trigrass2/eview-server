/**************************************************************
*  Filename:    MqttForwardServer.cpp
*  Copyright:   XinHong Software Co., Ltd.
*
*  Description: Mqtt Upload Server
*
*  @author:     xingxing
*  @version     2021/07/09 Initial Version
**************************************************************/
#ifndef _MQTT_FORWARD_SERVICE_H_
#define _MQTT_FORWARD_SERVICE_H_

#include <ace/Singleton.h>
#include "pkcomm/pkcomm.h"
#include "pkserver/PKServerBase.h"
#include "pklog/pklog.h"
#include <vector>
#include <string>
#include <map>
#include "MainTask.h"

class CMqttForwardServer : public CPKServerBase
{
public:
    CMqttForwardServer();
    virtual ~CMqttForwardServer();

    virtual int OnStart(int argc, char* args[]) ;
    virtual int OnStop() ;
private:
	int LoadConfig();
public:
    bool		m_bStop;
};

#endif//_MQTT_FORWARD_SERVICE_H_
