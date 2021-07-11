/**************************************************************
 *  Filename:    ErrCode_iCV_DW.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: ��ý�������붨��.
 *
 *  @author:     xulizai
 *  @version     05/23/2008  xulizai  Initial Version
**************************************************************/
#ifndef _ERRCODE_ICV_DW_H_
#define _ERRCODE_ICV_DW_H_

#include "errcode/error_code.h"

#define EC_ICV_DWCFG_START_NO   						12501 // ����ϵͳ���ô����뿪ʼ���

#define DW_ERRORCODE_SUCCESS			ICV_SUCCESS							// ���سɹ�

#define DW_ERRORCODE_FAILED				( EC_ICV_DWCFG_START_NO + 30 )		// ����δ֪����

#define DW_ERRORCODE_HALFSUCCESS		( EC_ICV_DWCFG_START_NO + 31 )		// ���ֳɹ�

#define DW_ERRORCODE_MODE_LENGTHOUT		( EC_ICV_DWCFG_START_NO + 32 )		// buffer��ʽ��ģʽ����Խ��

#define DW_ERRORCODE_ERORR_MODETYPE		( EC_ICV_DWCFG_START_NO + 33 )		// ģʽ���Ͳ���ȷ

#define DW_ERRORCODE_ERRTEXT			( EC_ICV_DWCFG_START_NO + 34 )		// ��Ϣ���ݲ���ȷ

#define DW_ERRORCODE_XMLCFGPOINTERISNULL ( EC_ICV_DWCFG_START_NO + 35 )	    // �����ļ�ָ��Ϊ��

#define DW_ERRORCODE_SIGNALUSED     	( EC_ICV_DWCFG_START_NO + 36)		// �ź�Դ���˿�ʹ��

#define DW_ERRORCODE_PORTLIMIT			( EC_ICV_DWCFG_START_NO + 37)		// ����;���˿ڵ���ϱ���Ψһ���ź�Դֻ�ܶ�Ӧһ���˿�


#define EC_DW_DWDRIVER_SUCCESS				0		// ���سɹ�

#define EC_DW_DWDRIVER_FAILED				-1		// ����δ֪����

#define EC_DW_DWDRIVER_HALFSUCCESS			1		// ���ֳɹ�

#define EC_DW_DWDRIVER_NETFAILED			2		// �������

#endif//_ERRCODE_ICV_DW_H_



