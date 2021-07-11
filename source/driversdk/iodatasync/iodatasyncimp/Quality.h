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

// Substatus for BAD, 与OPC中的Substatus一致
#define SS_NON_SPECIFIC			0		// No specific reason
#define SS_CONFIG_ERROR			1		// 配置错误,                       Configuration Error
#define SS_NOT_CONNECTED		2		// 未连接数据源,                   Input not connected
#define SS_DEVICE_FAILURE		3		// 设备故障,                       Protocol Error cause of Failure
#define SS_SENSOR_FAILURE		4		// 传感器故障,                     Sensor failure
#define SS_LAST_KNOWN			5		// 通信故障，提供最后一次有效值,   Latched data
#define SS_COMM_FAILURE			6		// 通信故障，不提供最后一次有效值, Time-out Error cause of Failure
#define SS_OUT_OF_SERVICE		7		// 不在服务范围内,                 Out of Service 

// SubstatusEx for BAD, nLeftOver字段，nQuality=QUALITY_BAD时有效
// 该字段为0时，与OPC中的Substatus一致，非0则包含iCV扩展的错误信息
// 通用信息
#define CV_LV_NO_SPECIFIC						0	//无特殊状态
// 服务中断的分支, 000111XX
#define CV_LV_IO_NUMBER_LIMIT					1	//IO数量超限,   IO Number Exceed Limit
#define CV_LV_SCHEDULE_FAILURE					2	//启动服务失败, Schedule Block Failed
#define CV_LV_SCAN_DISABLED						3	//禁止扫描, scan disabled
// 配置错误的分支, 000001XX
#define CV_LV_DRIVERNAME_EMPTY					1   //驱动名为空
#define CV_LV_DRIVER_NOTFOUND					2	//找不到对应驱动
#define CV_LV_ADDRESSLEN_TOOLONG				3	//IO地址长度过长
#define CV_LV_ADDRESS_BEGINFROMDIGITAL   		4	//非法名称:以数字开头
#define CV_LV_ADDRESS_SEGNUM_TOOSMALL   		5	//地址解析错误，拆分出的地址段不足
#define CV_LV_ADDRESS_SEGNUM_TOOMUCH			6	//地址解析错误，拆分出的地址段过多
#define CV_LV_DEVICENAME_EMPTY   				7	//设备名为空
#define CV_LV_IOADDRESS_EMPTY   				8	//设备地址为空
#define CV_LV_IOADDRESS_EXCEEDLIMIT				9	//设备地址超出范围
#define CV_LV_IOADDRESS_BITOFFSET_NOTEMPTY		10	//位偏移量无效
#define CV_LV_IOADDRESS_DIGITALTAG_NOBITOFFSET	11	//缺少位偏移量
#define CV_LA_IOADDRESS_BITOFFSET_EXCEEDLIMIT	12 // 数字量缺少位偏移量
#define CV_LV_IOADDRESS_BYTEOFFSET_EXCEEDLIMIT	13	//字节偏移量无效
#define CV_LV_IOADDRESS_DATALENGTH_TOOSMALL		14	//地址解析错误，数据块长度过小
#define CV_LV_IOADDRESS_DATALENGTH_TOOMUCH		15	//地址解析错误，数据块长度过大
#define CV_LV_DEVICE_NOTFOUND					16	//找不到对应设备
#define CV_LV_DATABLOCK_NOTFOUND				17	//找不到对应数据块
#define CV_LV_RELATIONALBLK_NOTFOUND			18	//找不到依赖的变量	
#define CV_LV_CA_EXPRESSION_INVALID				19	//非法的CA表达式
// 设备故障的分支, 000011XX
#define CV_LV_RELATIONALBLK_QUALITYBAD			1	//依赖变量的质量为BAD
#define CV_LV_CA_CALC_FAILURE					2	//表达式运算失败


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


//Quality对应的字符串说明
#define QUALITY_BAD_DESC			"BAD"
#define QUALITY_UNCERTAIN_DESC		"UNCERTAIN"
#define QUALITY_GOOD_DESC			"GOOD"

//未定义取值的字符串说明
#define QUALITY_UNDEFINED_DESC		"N/A"
#define SS_UNDEFINED_DESC			"未定义状态"

//SubQuality对应的字符串说明
#define SS_NON_SPECIFIC_GOOD_DESC "好的品质"
#define SS_LOCAL_OVERRIDE_DESC  "本地值被覆盖"

#define SS_NON_SPECIFIC_UNCERTAIN_DESC	"未知原因" 
#define SS_LAST_USABLE_DESC			"最后的可用值"
#define	SS_SENSOR_NOT_ACCURATE_DESC	"传感器取值超限"
#define SS_EGU_UNITS_EXCEEDED_DESC	"工程单元超限"
#define SS_SUBNORMAL_DESC			"多源应用中可用数据源不足"

#define SS_NON_SPECIFIC_BAD_DESC		"未知错误"
#define SS_CONFIG_ERROR_DESC			"配置错误"
#define SS_NOT_CONNECTED_DESC		"未连接设备或数据源"
#define SS_DEVICE_FAILURE_DESC		"设备故障"
#define SS_SENSOR_FAILURE_DESC		"传感器故障"
#define SS_LAST_KNOWN_DESC			"通信故障，最后的值可用"
#define SS_COMM_FAILURE_DESC			"通信故障，最后的值不可用"
#define SS_OUT_OF_SERVICE_DESC		"无法提供服务"

//LeftOver的字符串说明
#define CV_LV_IO_NUMBER_LIMIT_DESC					"IO数量超限"
#define CV_LV_SCHEDULE_FAILURE_DESC					"定时扫描失败"
#define CV_LV_SCAN_DISABLED_DESC					"禁止扫描"
#define CV_LV_DRIVER_NOTFOUND_DESC					"找不到对应驱动"
#define CV_LV_ADDRESSLEN_TOOLONG_DESC				"地址长度过长"
#define CV_LV_ADDRESS_BEGINFROMDIGITAL_DESC   		"地址段以数字开头"
#define CV_LV_ADDRESS_SEGNUM_TOOSMALL_DESC   		"拆分出的地址段不足"
#define CV_LV_ADDRESS_SEGNUM_TOOMUCH_DESC			"拆分出的地址段过多"
#define CV_LV_DRIVERNAME_EMPTY_DESC					"驱动名为空"
#define CV_LV_DEVICENAME_EMPTY_DESC   				"设备名为空"
#define CV_LV_IOADDRESS_EMPTY_DESC   				"设备地址为空"
#define CV_LV_IOADDRESS_EXCEEDLIMIT_DESC			"设备地址段超出允许范围"
#define CV_LV_IOADDRESS_BITOFFSET_NOTEMPTY_DESC		"地址解析出位偏移量"
#define CV_LV_IOADDRESS_DIGITALTAG_NOBITOFFSET_DESC	"缺少位偏移量"
#define CV_LA_IOADDRESS_BITOFFSET_EXCEEDLIMIT_DESC	"位偏移量超出允许范围"
#define CV_LV_IOADDRESS_BYTEOFFSET_EXCEEDLIMIT_DESC	"字节偏移量超出允许范围"
#define CV_LV_IOADDRESS_DATALENGTH_TOOSMALL_DESC	"数据块长度过小"
#define CV_LV_IOADDRESS_DATALENGTH_TOOMUCH_DESC		"数据块长度过大"
#define CV_LV_DEVICE_NOTFOUND_DESC					"找不到对应设备"
#define CV_LV_DATABLOCK_NOTFOUND_DESC				"找不到对应数据块"
#define CV_LV_RELATIONALBLK_NOTFOUND_DESC			"找不到依赖的变量"	
#define CV_LV_CA_EXPRESSION_INVALID_DESC			"非法的CA表达式"
#define CV_LV_RELATIONALBLK_QUALITYBAD_DESC			"依赖变量的质量为BAD"
#define CV_LV_CA_CALC_FAILURE_DESC					"表达式运算失败"


#endif
