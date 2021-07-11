/**************************************************************
 *  Filename:    RMConfigLoader.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: Impl. of CRMConfigLoader.
 *
 *  @author:     chenzhiquan
 *  @version     05/20/2008  chenzhiquan  Initial Version
**************************************************************/
#include "RMConfigLoader.h"
#include "errcode/ErrCode_iCV_RM.hxx"
#include <ace/OS_NS_stdlib.h>
#include "common/cvGlobalHelper.h"

/**
 *  Load Configuration.
 *
 *  @param  -[in,out]  const string&  strFile: [comment]
 *
 *  @version     05/20/2008  chenzhiquan  Initial Version.
 */
long CRMConfigLoader::LoadConfig( const string &strFile, CRMStatusCtrl *pRMStatusCtrl )
{
	long nErr = ICV_SUCCESS;

	// 1.Loading XML Config File.

	TiXmlElement* pElmChild = NULL;
	TiXmlDocument doc(strFile.c_str());

	if( !doc.LoadFile() ) 
        return EC_ICV_RMSERVICE_CONFIG_FILE_INVALID;
	
	// 2.Get Root Element.
	TiXmlElement* pElmRoot;
	pElmRoot = doc.FirstChildElement(RM_SVC_CFG_XML_ROOT_NODE); 
	CHECK_NULL_RETURN_ERR(pElmRoot);
	
	// 3.Load Sequnce.
	CHECK_NULL_RETURN_ERR(pElmRoot->Attribute(RM_SVC_CFG_XML_SEQUENCE_ATTR));
	string nSequence = pElmRoot->Attribute(RM_SVC_CFG_XML_SEQUENCE_ATTR);
	pRMStatusCtrl->SetSequence(ACE_OS::atoi(nSequence.c_str()));
	
	nErr = LoadStrategy(pElmRoot->FirstChildElement(RM_SVC_CFG_XML_STRATEGY_NODE), pRMStatusCtrl);
	if (nErr != ICV_SUCCESS)
	{
		return nErr;
	}

	nErr = LoadPeerCommunicator(pElmRoot->FirstChildElement(RM_SVC_CFG_XML_PEER_COMM_NODE), pRMStatusCtrl);
	if(nErr != ICV_SUCCESS)
	{
		return nErr;
	}

	nErr = LoadRMObjects(pElmRoot, pRMStatusCtrl);
	
	return nErr;	
}

/**
 *  Load Strategy Info.
 *
 *  @param  -[in,out]  TiXmlElement*  node: [comment]
 *
 *  @version     05/29/2008  chenzhiquan  Initial Version.
 */
long CRMConfigLoader::LoadStrategy( TiXmlElement* node, CRMStatusCtrl *pRMStatusCtrl)
{
	long nInterval = 0;
	long nThreshold = 0;
	CHECK_NULL_RETURN_ERR(node);

	TiXmlElement* nodeInterval = node->FirstChildElement(RM_SVC_CFG_XML_INTERVAL_NODE);
	TiXmlElement* nodeThreshold = node->FirstChildElement(RM_SVC_CFG_XML_THRESHOLD_NODE);
	
	CHECK_NULL_RETURN_ERR(nodeInterval);
	CHECK_NULL_RETURN_ERR(nodeThreshold);
	
	try
	{
		CHECK_NULL_RETURN_ERR(nodeInterval->GetText());
		CHECK_NULL_RETURN_ERR(nodeThreshold->GetText());

		nInterval = ACE_OS::atoi(nodeInterval->GetText());
		nThreshold = ACE_OS::atoi(nodeThreshold->GetText());
		
		// Normalize m_nInterval.
		if ( nInterval < RM_SVC_MIN_INTERVAL)
		{
			LH_ERROR(ACE_TEXT("Interval is Less then 50ms, Convert to 50ms instead!"));
			nInterval = RM_SVC_MIN_INTERVAL;
		}
		else if ( nInterval > RM_SVC_MAX_INTERVAL)
		{
			LH_ERROR(ACE_TEXT("Interval is Less then 3s, Convert to 3s instead!"));
			nInterval = RM_SVC_MAX_INTERVAL;
		}
		// Normalize 
		if ( nThreshold < RM_SVC_MIN_THRESHOLD)
		{
			LH_ERROR(ACE_TEXT("Threshold is Less then 4, Convert to 4 instead!"));
			nInterval = RM_SVC_MIN_INTERVAL;
		}
	}
	catch (...)
	{
		return EC_ICV_RMSERVICE_CONFIG_FILE_STRUCTURE_ERROR;
	}

	pRMStatusCtrl->SetStrategy(nInterval, nThreshold);

	return ICV_SUCCESS;
}

/**
 *  Load Peer Communicator.
 *
 *  @param  -[in,out]  TiXmlElement*  node: [comment]
 *
 *  @version     05/29/2008  chenzhiquan  Initial Version.
 */
long CRMConfigLoader::LoadPeerCommunicator( TiXmlElement* node, CRMStatusCtrl *pRMStatusCtrl)
{
	CRMPeerCommunicator* pPeerComm = NULL;

	CHECK_NULL_RETURN_ERR(node);
	CHECK_NULL_RETURN_ERR(node->Attribute(RM_SVC_CFG_XML_TYPE_ATTR));

	if (string(ACE_TEXT("CRMUDPComm")) == node->Attribute(RM_SVC_CFG_XML_TYPE_ATTR))
	{
		pPeerComm = new CRMUDPComm();
		long nErr = LoadUDPCommunicator(node, pRMStatusCtrl->GetSequence(), pPeerComm);
		if ( nErr != ICV_SUCCESS)
		{
			SAFE_DELETE(pPeerComm);
			return nErr;
		}

		pRMStatusCtrl->SetPeerCommunicator(pPeerComm);
	}
	else
	{

		return EC_ICV_RMSERVICE_CONFIG_FILE_UNKOWN_PEER_COMM;
	}
	return ICV_SUCCESS;
}

/**
 *  Load UDP Communicator.
 *
 *  @param  -[in,out]  TiXmlElement*  node: [comment]
 *
 *  @version     05/29/2008  chenzhiquan  Initial Version.
 */
long CRMConfigLoader::LoadUDPCommunicator( TiXmlElement *node, long nCurSequence, CRMPeerCommunicator* pComm)
{
	CRMUDPComm* pUDPPC = dynamic_cast<CRMUDPComm*>(pComm);

	//Load Port Lists
	TiXmlElement* nodePortList = node->FirstChildElement(RM_SVC_CFG_XML_PORT_LIST_NODE);
	CHECK_NULL_RETURN_ERR(nodePortList);

	TiXmlElement* pElement = nodePortList->FirstChildElement(RM_SVC_CFG_XML_PORT_NODE);
	CHECK_NULL_RETURN_ERR(pElement);

	do 
	{
		try
		{
			CHECK_NULL_RETURN_ERR(pElement->GetText());
			u_short usPort = (u_short) ACE_OS::atoi(pElement->GetText());
			pUDPPC->AddPortAddr(usPort);
		}
		catch (...)
		{
			return EC_ICV_RMSERVICE_CONFIG_FILE_STRUCTURE_ERROR;
		}

	} while((pElement = pElement->NextSiblingElement(RM_SVC_CFG_XML_PORT_NODE)) != NULL); // Next Port

	
	//int nCountIP = 0;
	TiXmlElement* nodePeerList = node->FirstChildElement(RM_SVC_CFG_XML_PEER_LIST_NODE);
	CHECK_NULL_RETURN_ERR(nodePeerList);

	//Load IP Lists
	for ( TiXmlElement* nodePeer = nodePeerList->FirstChildElement(RM_SVC_CFG_XML_PEER_NODE); 
		  nodePeer != NULL; 
		  nodePeer = nodePeer->NextSiblingElement(RM_SVC_CFG_XML_PEER_NODE))
	{ // traversing peer list and add peer ip to CRMUDPComm object
		CHECK_NULL_RETURN_ERR(nodePeer->Attribute(RM_SVC_CFG_XML_SEQUENCE_ATTR));
		
		if( nCurSequence == atoi(nodePeer->Attribute(RM_SVC_CFG_XML_SEQUENCE_ATTR)))
		{
			continue;
		}

		for (pElement = nodePeer->FirstChildElement(RM_SVC_CFG_XML_IP_NODE);
			 pElement != NULL;
			 pElement = pElement->NextSiblingElement(RM_SVC_CFG_XML_IP_NODE))
		{
			if (pElement->GetText() == NULL)
			{
				break;
			}
			pUDPPC->AddIPAddr(pElement->GetText());
			//nCountIP++;
		}

	}
//	// Check IP Count : cannot be 0
//	if (nCountIP == 0)
//		return EC_ICV_RMSERVICE_CONFIG_FILE_NO_UDP_IP_SPEC;
	
	return ICV_SUCCESS;
}

long CRMConfigLoader::LoadRMObjects(TiXmlElement *node, CRMStatusCtrl *pRMStatusCtrl)
{
	TiXmlElement* nodeRMObject = node->FirstChildElement(RM_SVC_CFG_XML_RMOBJECT_NODE);

	while( nodeRMObject != NULL)
	{
		CHECK_NULL_RETURN_ERR(nodeRMObject->Attribute(RM_SVC_CFG_XML_NAME_ATTR));
		CHECK_NULL_RETURN_ERR(nodeRMObject->Attribute(RM_SVC_CFG_XML_PRIM_SEQ_ATTR));
		CHECK_NULL_RETURN_ERR(nodeRMObject->Attribute(RM_SVC_CFG_XML_RUNMODE_ATTR));
		CHECK_NULL_RETURN_ERR(nodeRMObject->Attribute(RM_SVC_CFG_XML_NEED_SYNC_ATTR));

		const char * pstrTemp = nodeRMObject->Attribute(RM_SVC_CFG_XML_NAME_ATTR);
		if(pstrTemp != NULL && pstrTemp[0] != 0)
		{
			RMObject rmObject;

			CVStringHelper::Safe_StrNCpy(rmObject.szObjName, pstrTemp, sizeof(rmObject.szObjName));

			rmObject.nPrimSeq = atoi(nodeRMObject->Attribute(RM_SVC_CFG_XML_PRIM_SEQ_ATTR));

			int nRunMode = atoi(nodeRMObject->Attribute(RM_SVC_CFG_XML_RUNMODE_ATTR));
			if(nRunMode == 1)
			{
				rmObject.bBeSingle = true;
			}
			else
			{
				rmObject.bBeSingle = false;
			}

			int nNeedSync = atoi(nodeRMObject->Attribute(RM_SVC_CFG_XML_NEED_SYNC_ATTR));
			if(nNeedSync == 1)
			{
				rmObject.bNeedSync = true;
			}
			else
			{
				rmObject.bNeedSync = false;
			}
			
			rmObject.nCtrlCount = 0;
			rmObject.nCtrlExec = RM_STATUS_CTRL_NIL;
			rmObject.nSend = RM_STATUS_CTRL_NIL;

			pRMStatusCtrl->AddRMObject(&rmObject);
		}

		nodeRMObject = nodeRMObject->NextSiblingElement(RM_SVC_CFG_XML_RMOBJECT_NODE);
	}
	
	return ICV_SUCCESS;
} 
