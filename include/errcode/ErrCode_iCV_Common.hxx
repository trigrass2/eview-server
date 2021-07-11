/**************************************************************
 *  Filename:    ErrCode_iCV_Common.hxx
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: iCV5公共错误代码管理，代码段(13000-13199).
 *
 *  @author:     caichunlei
 *  @version     05/23/2008  caichunlei  Initial Version
**************************************************************/
#ifndef _ERRCODE_ICV_COMMON_HXX_
#define _ERRCODE_ICV_COMMON_HXX_


#include "errcode/error_code.h"


#define CV_ERR2BDB_ERR(_cv_err_)	( ((_cv_err_) == 0)?0: (-1 *((_cv_err_) & ~(0xc0cf0000))) )  // 好像没有用，放这儿，以免还需要

#ifndef EC_ICV_COMM_BASENO
#define EC_ICV_COMM_BASENO 1300 // 没有这个ServiceBase.cpp编不过
#endif

#endif // _ERRCODE_ICV_COMMON_HXX_
