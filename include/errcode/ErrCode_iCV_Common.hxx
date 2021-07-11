/**************************************************************
 *  Filename:    ErrCode_iCV_Common.hxx
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: iCV5�������������������(13000-13199).
 *
 *  @author:     caichunlei
 *  @version     05/23/2008  caichunlei  Initial Version
**************************************************************/
#ifndef _ERRCODE_ICV_COMMON_HXX_
#define _ERRCODE_ICV_COMMON_HXX_


#include "errcode/error_code.h"


#define CV_ERR2BDB_ERR(_cv_err_)	( ((_cv_err_) == 0)?0: (-1 *((_cv_err_) & ~(0xc0cf0000))) )  // ����û���ã�����������⻹��Ҫ

#ifndef EC_ICV_COMM_BASENO
#define EC_ICV_COMM_BASENO 1300 // û�����ServiceBase.cpp�಻��
#endif

#endif // _ERRCODE_ICV_COMMON_HXX_
