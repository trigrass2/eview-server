/**************************************************************
 *  Filename:    RMConfigLoader.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: Configuation Loader.
 *
 *  @author:     chenzhiquan
 *  @version     05/20/2008  chenzhiquan  Initial Version
**************************************************************/
#ifndef _RM_CONFIG_LOADER_H_
#define _RM_CONFIG_LOADER_H_

#include <ace/INET_Addr.h>
#include "tinyxml/tinyxml.h"
#include "RMPeerCommunicator.h"
#include "RMStatusCtrl.h"
#include "common/CommHelper.h"
#include "common/RMConfigXMLDef.h"

#include <string>
using namespace std;

#define RM_SVC_MIN_INTERVAL		50
#define RM_SVC_MAX_INTERVAL		3000
#define RM_SVC_MIN_THRESHOLD	4


#define CHECK_NULL_RETURN_ERR(X) \
	{if(!X) return EC_ICV_RMSERVICE_CONFIG_FILE_STRUCTURE_ERROR;}

class CRMConfigLoader
{
	UNITTEST(CRMConfigLoader);
private:
	// Load Strategy
	long LoadStrategy(TiXmlElement* node, CRMStatusCtrl *pRMStatusCtrl);
	// Load Peer Communicator
	long LoadPeerCommunicator(TiXmlElement* node, CRMStatusCtrl *pRMStatusCtrl);
	// Load UDP Communicator
	long LoadUDPCommunicator(TiXmlElement *node, long nCurSequence, CRMPeerCommunicator* pComm);
	// Load RMObject
	long LoadRMObjects(TiXmlElement *node, CRMStatusCtrl *pRMStatusCtrl);

public:
	// Load Config File
	long LoadConfig(const string &strFile, CRMStatusCtrl *pRMStatusCtrl);
	// Get Sequence
	long GetSequence();
	// Get Strategy Information.
	void GetStrategyInfo(long &nInterval, long &nThreshold);
	// Get Peer Communicator
	CRMPeerCommunicator* GetPeerComm();

};

#endif//_RM_CONFIG_LOADER_H_
