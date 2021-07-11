/**************************************************************
 *  Filename:    ErrCode_iCV_DW.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 多媒体错误代码定义.
 *
 *  @author:     xulizai
 *  @version     05/23/2008  xulizai  Initial Version
**************************************************************/
#ifndef _ERRCODE_ICV_DW_H_
#define _ERRCODE_ICV_DW_H_

#include "errcode/error_code.h"

#define EC_ICV_DWCFG_START_NO   						12501 // 大屏系统配置错误码开始编号

#define DW_ERRORCODE_SUCCESS			ICV_SUCCESS							// 返回成功

#define DW_ERRORCODE_FAILED				( EC_ICV_DWCFG_START_NO + 30 )		// 返回未知错误

#define DW_ERRORCODE_HALFSUCCESS		( EC_ICV_DWCFG_START_NO + 31 )		// 部分成功

#define DW_ERRORCODE_MODE_LENGTHOUT		( EC_ICV_DWCFG_START_NO + 32 )		// buffer形式的模式长度越界

#define DW_ERRORCODE_ERORR_MODETYPE		( EC_ICV_DWCFG_START_NO + 33 )		// 模式类型不正确

#define DW_ERRORCODE_ERRTEXT			( EC_ICV_DWCFG_START_NO + 34 )		// 信息内容不正确

#define DW_ERRORCODE_XMLCFGPOINTERISNULL ( EC_ICV_DWCFG_START_NO + 35 )	    // 配置文件指针为空

#define DW_ERRORCODE_SIGNALUSED     	( EC_ICV_DWCFG_START_NO + 36)		// 信号源被端口使用

#define DW_ERRORCODE_PORTLIMIT			( EC_ICV_DWCFG_START_NO + 37)		// 矩阵和矩阵端口的组合必须唯一；信号源只能对应一个端口


#define EC_DW_DWDRIVER_SUCCESS				0		// 返回成功

#define EC_DW_DWDRIVER_FAILED				-1		// 返回未知错误

#define EC_DW_DWDRIVER_HALFSUCCESS			1		// 部分成功

#define EC_DW_DWDRIVER_NETFAILED			2		// 网络错误

#endif//_ERRCODE_ICV_DW_H_



