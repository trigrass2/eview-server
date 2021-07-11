/**************************************************************
 *  Filename:    RMAPI.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: RM API .
 *
 *  @author:     chenzhiquan
 *  @version     05/29/2008  chenzhiquan  Initial Version
**************************************************************/
#ifndef _RM_API_H_
#define _RM_API_H_

//RETURN CODE OF REDUNDANCY MANAGEMENT
#define RM_STATUS_INACTIVE		0
#define RM_STATUS_ACTIVE		1
#define RM_STATUS_UNAVALIBLE	2

#ifdef _WIN32

#	ifdef rmapi_EXPORTS
#		define RMAPI_DLLEXPORT extern "C" //__declspec(dllexport)
#	else
#		define RMAPI_DLLEXPORT extern "C" //__declspec(dllimport)
#	endif

#else //_WIN32

#	define RMAPI_DLLEXPORT extern "C" __attribute__ ((visibility ("default")))

#endif //_WIN32

// Initialize
RMAPI_DLLEXPORT int RMAPI_Init();

/**
 *  Get RM Status.
 *  get ACTIVE/INACTIVE.
 *
 *  @param  -[in,out]  int*  pStatus: [RMStatus]
 *  @return ICV_SUCCESS if success.
 *
 *  @version     05/29/2008  chenzhiquan  Initial Version.
 */
RMAPI_DLLEXPORT int GetRMStatus(int* pnStatus);

/**
 *  Set Redundancy Status Manualy.
 *
 *  @param  -[in,out]  int  nStatus: [Status To Ctrl]
 *  @return ICV_SUCCESS if success.
 *
 *  @version     05/29/2008  chenzhiquan  Initial Version.
 */
RMAPI_DLLEXPORT int SetRMStatus(int nStatus);

#endif// _RM_API_H_
