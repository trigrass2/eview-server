// @doc Quality
//		
// @module Quality.h - OPC Data Quality Bit definitions |
//
// This file contains the definition of the OPC Data Quality Bits.
// It is for use in the 7.0 drivers.
//

//	(C) COPYRIGHT 1996 INTELLUTION INC. ALL RIGHTS RESERVED
//
//	This software contains information which represents trade secrets of 
//	Intellution and may not be copied or disclosed to others except as 
//	provided in the	license with Intellution.
//
// Maintenance
//	Version	  Date		Who		What
//	-------	--------	---		------
//	7.0		07/25/97	jra		Created
//

#ifndef QUALITY_H
#define QUALITY_H	1

// WORD quality - Used to set the entire quality to bad, for example.
#define	WORD_QUALITY_BAD		0x00	// 00000000
#define QUALITY_IS_GOOD(qua_stat)	(((qua_stat)>>6) == 3)

#define	WORD_QUALITY_GOOD		0xC0	// 11000000
#define	WORD_QUALITY_UNCERTAIN	0x40	// 01000000

#if defined(__hpux) || defined(_hpux) || defined(hpux) || defined(_AIX)
//#define	WORD_QUALITY_GOOD		0xC000	// 11000000000000000
//#define	WORD_QUALITY_UNCERTAIN	0x4000	// 01000000000000000
#pragma pack(1)
typedef struct
{
	unsigned short nLeftOver  : 8;
	unsigned short nQuality   : 2;		// 0..3   (2 bits)
	unsigned short nSubStatus : 4;		// 0..15  (4 bits)
	unsigned short nLimit     : 2;		// 0..3   (2 bits)
} QUALITY_STATE;
#pragma pack()

#else

// definition of the OPC Quality State
typedef struct
{
    unsigned short nLimit     : 2;		// 0..3   (2 bits)
    unsigned short nSubStatus : 4;		// 0..15  (4 bits)
    unsigned short nQuality   : 2;		// 0..3   (2 bits)
	unsigned short nLeftOver  : 8;
} QUALITY_STATE;
#endif

// Definition of Quality Values
#define QUALITY_BAD				0
#define QUALITY_UNCERTAIN		1
#define QUALITY_GOOD			3

// Substatus for BAD, ��OPC�е�Substatusһ��
#define SS_NON_SPECIFIC			0		// No specific reason
#define SS_CONFIG_ERROR			1		// ���ô���,                       Configuration Error
#define SS_NOT_CONNECTED		2		// δ��������Դ,                   Input not connected
#define SS_DEVICE_FAILURE		3		// �豸����,                       Protocol Error cause of Failure
#define SS_SENSOR_FAILURE		4		// ����������,                     Sensor failure
#define SS_LAST_KNOWN			5		// ͨ�Ź��ϣ��ṩ���һ����Чֵ,   Latched data
#define SS_COMM_FAILURE			6		// ͨ�Ź��ϣ����ṩ���һ����Чֵ, Time-out Error cause of Failure
#define SS_OUT_OF_SERVICE		7		// ���ڷ���Χ��,                 Out of Service 

// SubstatusEx for BAD, nLeftOver�ֶΣ�nQuality=QUALITY_BADʱ��Ч
// ���ֶ�Ϊ0ʱ����OPC�е�Substatusһ�£���0�����iCV��չ�Ĵ�����Ϣ
// ͨ����Ϣ
#define CV_LV_NO_SPECIFIC						0	//������״̬
// �����жϵķ�֧, 000111XX
#define CV_LV_IO_NUMBER_LIMIT					1	//IO��������,   IO Number Exceed Limit
#define CV_LV_SCHEDULE_FAILURE					2	//��������ʧ��, Schedule Block Failed
#define CV_LV_SCAN_DISABLED						3	//��ֹɨ��, scan disabled
// ���ô���ķ�֧, 000001XX
#define CV_LV_DRIVERNAME_EMPTY					1   //������Ϊ��
#define CV_LV_DRIVER_NOTFOUND					2	//�Ҳ�����Ӧ����
#define CV_LV_ADDRESSLEN_TOOLONG				3	//IO��ַ���ȹ���
#define CV_LV_ADDRESS_BEGINFROMDIGITAL   		4	//�Ƿ�����:�����ֿ�ͷ
#define CV_LV_ADDRESS_SEGNUM_TOOSMALL   		5	//��ַ�������󣬲�ֳ��ĵ�ַ�β���
#define CV_LV_ADDRESS_SEGNUM_TOOMUCH			6	//��ַ�������󣬲�ֳ��ĵ�ַ�ι���
#define CV_LV_DEVICENAME_EMPTY   				7	//�豸��Ϊ��
#define CV_LV_IOADDRESS_EMPTY   				8	//�豸��ַΪ��
#define CV_LV_IOADDRESS_EXCEEDLIMIT				9	//�豸��ַ������Χ
#define CV_LV_IOADDRESS_BITOFFSET_NOTEMPTY		10	//λƫ������Ч
#define CV_LV_IOADDRESS_DIGITALTAG_NOBITOFFSET	11	//ȱ��λƫ����
#define CV_LA_IOADDRESS_BITOFFSET_EXCEEDLIMIT	12 // ������ȱ��λƫ����
#define CV_LV_IOADDRESS_BYTEOFFSET_EXCEEDLIMIT	13	//�ֽ�ƫ������Ч
#define CV_LV_IOADDRESS_DATALENGTH_TOOSMALL		14	//��ַ�����������ݿ鳤�ȹ�С
#define CV_LV_IOADDRESS_DATALENGTH_TOOMUCH		15	//��ַ�����������ݿ鳤�ȹ���
#define CV_LV_DEVICE_NOTFOUND					16	//�Ҳ�����Ӧ�豸
#define CV_LV_DATABLOCK_NOTFOUND				17	//�Ҳ�����Ӧ���ݿ�
#define CV_LV_RELATIONALBLK_NOTFOUND			18	//�Ҳ��������ı���	
#define CV_LV_CA_EXPRESSION_INVALID				19	//�Ƿ���CA���ʽ
// �豸���ϵķ�֧, 000011XX
#define CV_LV_RELATIONALBLK_QUALITYBAD			1	//��������������ΪBAD
#define CV_LV_CA_CALC_FAILURE					2	//���ʽ����ʧ��


// Substatus for UNCERTAIN
#define SS_NON_SPECIFIC			0		// No specific reason 
#define SS_LAST_USABLE			1		// Last usable value
#define	SS_SENSOR_NOT_ACCURATE	4		// Sensor not accurate
#define SS_EGU_UNITS_EXCEEDED	5		// Engineering Units exceeded
#define SS_SUBNORMAL			6		// Sub-normal

// Substatus for GOOD
#define SS_LOCAL_OVERRIDE		6		// Local override

// Limit Field
#define LIMIT_NOT_LIMITED		0		// Value is free to move
#define LIMIT_LOW_LIMIT			1		// Value is pegged to low limit
#define LIMIT_HIGH_LIMIT		2		// Value is pegged to high limit
#define LIMIT_CONSTANT_LIMIT	3		// Value cannot move


//Quality��Ӧ���ַ���˵��
#define QUALITY_BAD_DESC			"BAD"
#define QUALITY_UNCERTAIN_DESC		"UNCERTAIN"
#define QUALITY_GOOD_DESC			"GOOD"

//δ����ȡֵ���ַ���˵��
#define QUALITY_UNDEFINED_DESC		"N/A"
#define SS_UNDEFINED_DESC			"δ����״̬"

//SubQuality��Ӧ���ַ���˵��
#define SS_NON_SPECIFIC_GOOD_DESC "�õ�Ʒ��"
#define SS_LOCAL_OVERRIDE_DESC  "����ֵ������"

#define SS_NON_SPECIFIC_UNCERTAIN_DESC	"δ֪ԭ��" 
#define SS_LAST_USABLE_DESC			"���Ŀ���ֵ"
#define	SS_SENSOR_NOT_ACCURATE_DESC	"������ȡֵ����"
#define SS_EGU_UNITS_EXCEEDED_DESC	"���̵�Ԫ����"
#define SS_SUBNORMAL_DESC			"��ԴӦ���п�������Դ����"

#define SS_NON_SPECIFIC_BAD_DESC		"δ֪����"
#define SS_CONFIG_ERROR_DESC			"���ô���"
#define SS_NOT_CONNECTED_DESC		"δ�����豸������Դ"
#define SS_DEVICE_FAILURE_DESC		"�豸����"
#define SS_SENSOR_FAILURE_DESC		"����������"
#define SS_LAST_KNOWN_DESC			"ͨ�Ź��ϣ�����ֵ����"
#define SS_COMM_FAILURE_DESC			"ͨ�Ź��ϣ�����ֵ������"
#define SS_OUT_OF_SERVICE_DESC		"�޷��ṩ����"

//LeftOver���ַ���˵��
#define CV_LV_IO_NUMBER_LIMIT_DESC					"IO��������"
#define CV_LV_SCHEDULE_FAILURE_DESC					"��ʱɨ��ʧ��"
#define CV_LV_SCAN_DISABLED_DESC					"��ֹɨ��"
#define CV_LV_DRIVER_NOTFOUND_DESC					"�Ҳ�����Ӧ����"
#define CV_LV_ADDRESSLEN_TOOLONG_DESC				"��ַ���ȹ���"
#define CV_LV_ADDRESS_BEGINFROMDIGITAL_DESC   		"��ַ�������ֿ�ͷ"
#define CV_LV_ADDRESS_SEGNUM_TOOSMALL_DESC   		"��ֳ��ĵ�ַ�β���"
#define CV_LV_ADDRESS_SEGNUM_TOOMUCH_DESC			"��ֳ��ĵ�ַ�ι���"
#define CV_LV_DRIVERNAME_EMPTY_DESC					"������Ϊ��"
#define CV_LV_DEVICENAME_EMPTY_DESC   				"�豸��Ϊ��"
#define CV_LV_IOADDRESS_EMPTY_DESC   				"�豸��ַΪ��"
#define CV_LV_IOADDRESS_EXCEEDLIMIT_DESC			"�豸��ַ�γ�������Χ"
#define CV_LV_IOADDRESS_BITOFFSET_NOTEMPTY_DESC		"��ַ������λƫ����"
#define CV_LV_IOADDRESS_DIGITALTAG_NOBITOFFSET_DESC	"ȱ��λƫ����"
#define CV_LA_IOADDRESS_BITOFFSET_EXCEEDLIMIT_DESC	"λƫ������������Χ"
#define CV_LV_IOADDRESS_BYTEOFFSET_EXCEEDLIMIT_DESC	"�ֽ�ƫ������������Χ"
#define CV_LV_IOADDRESS_DATALENGTH_TOOSMALL_DESC	"���ݿ鳤�ȹ�С"
#define CV_LV_IOADDRESS_DATALENGTH_TOOMUCH_DESC		"���ݿ鳤�ȹ���"
#define CV_LV_DEVICE_NOTFOUND_DESC					"�Ҳ�����Ӧ�豸"
#define CV_LV_DATABLOCK_NOTFOUND_DESC				"�Ҳ�����Ӧ���ݿ�"
#define CV_LV_RELATIONALBLK_NOTFOUND_DESC			"�Ҳ��������ı���"	
#define CV_LV_CA_EXPRESSION_INVALID_DESC			"�Ƿ���CA���ʽ"
#define CV_LV_RELATIONALBLK_QUALITYBAD_DESC			"��������������ΪBAD"
#define CV_LV_CA_CALC_FAILURE_DESC					"���ʽ����ʧ��"


#endif
