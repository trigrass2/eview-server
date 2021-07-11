/*!***********************************************************
 *  @file        AlmMiniDef.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  @brief       (������豨�������С�ṹ�嶨��).
 *
 *  @author:     yangbo
 *  @version     11/15/2012  yangbo  Initial Version
**************************************************************/
#ifndef _ALM_MINI_DEF_HEADER_
#define _ALM_MINI_DEF_HEADER_

//////////////////// �������ͺ궨�� //////////////////////////
#define ALM_TYPE_LOLO       1	// �͵ͱ���
#define ALM_TYPE_LO         2	// �ͱ���
#define ALM_TYPE_HI         3	// �߱���
#define ALM_TYPE_HIHI       4 	// �߸߱���
#define ALM_TYPE_ROC        5	// �仯�ʱ���
#define ALM_TYPE_COS		6	// ��λ����
#define ALM_TYPE_OPEN       26	// 0->1����
#define ALM_TYPE_CLOSE      27	// 1->0����
// �Զ��屨����LA����ʹ��0-15��MDI����ʹ��0-7�����������Զ���
#define ALM_TYPE_USER_DEF0  28	// user defined alarm type			
#define ALM_TYPE_USER_DEF1  29	// user defined alarm type			
#define ALM_TYPE_USER_DEF2  30	// user defined alarm type			
#define ALM_TYPE_USER_DEF3  31	// user defined alarm type			
#define ALM_TYPE_USER_DEF4  32	// user defined alarm type			
#define ALM_TYPE_USER_DEF5  33	// user defined alarm type			
#define ALM_TYPE_USER_DEF6  34	// user defined alarm type			
#define ALM_TYPE_USER_DEF7  35	// user defined alarm type			
#define ALM_TYPE_USER_DEF8  36	// user defined alarm type			
#define ALM_TYPE_USER_DEF9  37	// user defined alarm type			
#define ALM_TYPE_USER_DEF10 38	// user defined alarm type			
#define ALM_TYPE_USER_DEF11 39	// user defined alarm type			
#define ALM_TYPE_USER_DEF12 40	// user defined alarm type			
#define ALM_TYPE_USER_DEF13 41	// user defined alarm type			
#define ALM_TYPE_USER_DEF14 42	// user defined alarm type			
#define ALM_TYPE_USER_DEF15 43	// user defined alarm type	
#define ALM_TYPE_KEEPON     44	// =1״̬����			
#define ALM_TYPE_KEEPOFF    45	// =0״̬����			
#define ALM_TYPE_WILDCARD   255	// ����ͨ���

//////////////////// ����״̬�궨�� //////////////////////////
#define ALM_STATUS_OK	0	// no alarm(acked & recovered)			0000 0000
#define ALM_STATUS_ACK	1	// acked alarm(ȷ��)					0000 0001
#define ALM_STATUS_RTN	2	// return to normal(�ָ�)				0000 0010
#define ALM_STATUS_ALM	3	// in alarm��������						0000 0011

#define ALM_STATUS_ALM_MASK		1	// ����δ�ָ���MASK				0000 0001
#define ALM_STATUS_UNACK_MASK	2	// ����δȷ�ϵ�MASK				0000 0010

//////////////////// �����������ֵ���� //////////////////////////
#define ALM_RET_SUCCESS				0	// �����ɹ�
#define ALM_RET_INVALID_PARAMETER	1	// �Ƿ�����
#define ALM_RET_NEED_NOT_SEND		2	// ����Ҫ���͸��ͻ���

//////////////////// ��������汾�Ŷ��� //////////////////////////
#define ICV_DLL_ALM_INVALID_VERSION 0	// ��Ч�汾��
#define ICV_DLL_ALM_VERSION_ONE		1	// ��ʼ�汾��

//////// �����ṹ�壬ֻ���������Ҫ����ü������� //////////
struct CV_ALARM_MINI
{
	unsigned int	ulAlmSeq;		// alarm sequence
	const char*		pszNodeName;	// node name string ptr
	const char*		pszTagName;		// tag name string ptr
	unsigned char	ucType;			// alarm type
	unsigned char	ucStatus;		// alarm status

	CV_ALARM_MINI()
	{
		ulAlmSeq = 0;
		pszNodeName = NULL;
		pszTagName = NULL;
		ucType = 0;
		ucStatus = 0;
	}
};

#endif //_ALM_MINI_DEF_HEADER_
