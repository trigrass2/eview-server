#ifndef _PK_DRVCOMMON_H_
#define _PK_DRVCOMMON_H_

#include <string.h>
#include <iostream>
#include <iomanip>
#include "eviewcomm/eviewcomm.h"
using namespace std;

#ifdef _WIN32
#define  PKDRIVER_EXPORTS extern "C" __declspec(dllexport)
#else //LINUX,UNIX
#define  PKDRIVER_EXPORTS extern "C" __attribute__ ((visibility ("default")))
#endif //LINUX,UNIX

typedef struct _PKTAG
{
	char	*szName;		//点的名称
	char	*szAddress;		//点的地址串，eg.400001#10 or 400001:1.如果有设备数据地址也放在这里，如snmp的int32、uint32、counter32、gauge32、timeticks 、octet、oid
	
	int		nDataType;		//点的数据类型
	int		nDataLen;		// 保存tag点的值的长度，和nLenBits不同，nLenBits是实际长度，这里是tag存储的数据类型的长度，单位是字节。这个是根据数据类型而定，在驱动中不能被修改！
	int		nByteOrder;		// 用于记录数值类型的解析方式（44332211,11223344）
	int		nPollRate;		//点的扫描周期
	char	*szParam;		// 可能存储站号等, 从配置中读取

	// 上述变量在驱动中不能修改, 下面的变量是在驱动中可以修改
	int		nStartBit;		// 供调用者使用，用于存储该变量所在数据块的块内偏移起始位
	int		nEndBit;		// 供调用者使用，用于存储该变量所在数据块的块内结束位
	int		nLenBits;		// 字符串/blob的长度,bit为单位, 由配置的tag点的数据类型确定初始值。对于按照几位取的地址如:40001:4-7，该长度会在解析时被改变

	// 保存更新时的值、质量、时间戳、值的类型（字符串还是具体类型）
	char	*szData;		// 保存tag点的值, 可以是二进制数据, 也可以是转换为ascii后的值, 长度为nDataLen+1
	int		nQuality;
	unsigned int  nTimeSec;	// 1970年开始的时间戳,到秒,time_t, 实际上unsigned int, 4个字节
	unsigned short nTimeMilSec; // 秒内的毫秒数,0...100 2015-09-24 09:20:38.851

	int		nData1;			// 预留, 供调用者使用
	int		nData2;			// 预留, 供调用者使用
	int		nData3;			// 预留, 供调用者使用
	int		nData4;			// 预留, 供调用者使用
	void *	pData1;			// 预留，供调用者使用，可以指向DataBlock, 在python驱动中指向pyTag
	void *	pData2;			// 预留，供调用者使用，可以指向DataBlock
}PKTAG;

struct _PKDRIVER;
typedef struct _PKDEVICE
{
	char *		szName;
	char *		szDesc;
	char *		szConnType;		// tcpclient/tcpserver/serial/udpserver/other
	char *		szConnParam;
	char *		szParam1;		// 配置的设备参数1
	char *		szParam2;		// 配置的设备参数2
	char *		szParam3;		// 配置的设备参数3
	char *		szParam4;		// 配置的设备参数4
	char *		szParam5;		// 配置的设备参数5
	char *		szParam6;		// 配置的设备参数6
	char *		szParam7;		// 配置的设备参数7
	char *		szParam8;		// 配置的设备参数8

	_PKDRIVER *	pDriver;		// 指向驱动的指针
	PKTAG **	ppTags;			// 该设备所具有的所有tag点的数组
	int			nTagNum;		// 该设备的tag点个数
	void *		_pInternalRef;	// 指向内部数据引用，不能使用

	int			nConnTimeout;	// 连接超时, 毫秒为单位
	int			nRecvTimeout;	// 接收超时, 毫秒为单位
	bool		bEnableConnect; // 设备是否允许连接

	// 上面参数驱动不能修改, 下面参数是预留给驱动的, 可以被驱动使用和改变
	void *		pUserData[PK_USERDATA_MAXNUM];	// 保留字段，供驱动开发人员保留使用
	int			nUserData[PK_USERDATA_MAXNUM];	// 保留字段，供驱动开发人员保留使用
	char		szUserData[PK_USERDATA_MAXNUM][PK_DESC_MAXLEN];// 保留字段，供驱动开发人员保留使用
}PKDEVICE;

typedef struct _PKDRIVER
{
	char	*	szName;
	char	*	szDesc;
	char	*	szParam1;
	char	*	szParam2;
	char	*	szParam3;
	char	*	szParam4;

	PKDEVICE **	ppDevices;		// 该驱动下所属的所有设备数目
	int			nDeviceNum;		// 设备个数
	void *		_pInternalRef;	// 指向内部数据引用，不能使用

	// 上面属性不能修改, 下面属性预留给驱动开发使用
	int			nUserData[PK_USERDATA_MAXNUM];		
	char		szUserData[PK_USERDATA_MAXNUM][PK_PARAM_MAXLEN];
	void *		pUserData[PK_USERDATA_MAXNUM];
}PKDRIVER;

typedef struct _PKTIMER
{
	int			nPeriodMS;	// 定时周期，单位毫秒
	int			nPhaseMS;	// 相位, 定时器的起始时间，可以合理设置做到整分整秒
	void *		_pInternalRef; // 内部引用，不能使用

	// 下面参数预留给驱动使用
	int			nUserData[PK_USERDATA_MAXNUM];		// 预留用户参数
	char		szUserData[PK_USERDATA_MAXNUM][PK_PARAM_MAXLEN];// 预留用户参数
	void *		pUserData[PK_USERDATA_MAXNUM];// 预留用户参数
}PKTIMER;

PKDRIVER_EXPORTS int	Drv_Connect(PKDEVICE *pDevice, int nTimeOutMS);	// 主动与设备进行连接。通常不需要显示调用，读写数据时驱动框架会自动和设备建立连接。发生错误时可以手工断开和重新连接
PKDRIVER_EXPORTS int	Drv_Disconnect(PKDEVICE *pDevice);	// 主动和设备断开连接。通常不需要显示调用，可以在发生错误时主动断开与设备的连接，以便重新握手时调用
PKDRIVER_EXPORTS int	Drv_Send(PKDEVICE *pDevice, const char *szBuffer, int nBufLen, int nTimeOutMS);	// 向设备发送nBufLen长度的内容，返回发送的字节个数
PKDRIVER_EXPORTS int	Drv_Recv(PKDEVICE *pDevice, char *szBuffer, int nBufLen, int nTimeOutMS);	// 从设备接收最多nBufLen长度的内容，返回接收的实际字节个数
PKDRIVER_EXPORTS void	Drv_ClearRecvBuffer(PKDEVICE *pDevice);

PKDRIVER_EXPORTS int	Drv_SetTagData_Text(PKTAG *pTag, const char *szValue, unsigned int nTagSec = 0, unsigned short nTagMSec = 0, int nTagQuality = 0); // 设置某个TAG的值(需为文本字符串格式)、时间戳、质量。仅修改pTag的VTQ, 还未写入内存数据库
PKDRIVER_EXPORTS int	Drv_SetTagData_Binary(PKTAG *pTag, void *szData, int nValueLen, unsigned int nTagSec = 0, unsigned short nTagMSec = 0, int nTagQuality = 0); // 设置某个TAG的值(二进制原始值)、时间戳、质量。仅修改pTag的VTQ, 还未写入内存数据库
PKDRIVER_EXPORTS int	Drv_UpdateTagsData(PKDEVICE *pDevice, PKTAG **pTags, int nTagNum); // 更新若干个TAG的数据、状态、时间戳到内存数据库
PKDRIVER_EXPORTS int	Drv_UpdateTagsDataByAddress(PKDEVICE *pDevice, const char *szTagAddress, const char *szValue, int nValueLen = 0, unsigned int nTimeSec = 0, unsigned short nTimeMilSec = 0, int nQuality = 0); // 更新数据块的数据、状态、时间戳到内存数据库，返回更新的tag点个数

PKDRIVER_EXPORTS void	Drv_LogMessage(int nLogLevel, const char *fmt, ...);	// 打印日志到文件和控制台

PKDRIVER_EXPORTS void *	Drv_CreateTimer(PKDEVICE *pDevice, PKTIMER *timerInfo);
PKDRIVER_EXPORTS int	Drv_DestroyTimer(PKDEVICE *pDevice, void *pTimerHandle);

PKDRIVER_EXPORTS int	Drv_GetTagsByAddr(PKDEVICE *pDevice, const char *szTagAddress, PKTAG **pTags, int nMaxTagNum);// 根据tag地址找到tag点数组结构体指针. 返回找到的tag个数
PKDRIVER_EXPORTS int	Drv_GetTagData(PKDEVICE *pDevice, PKTAG *pTag, char *szValue, int nValueBufLen, int *pnRetValueLen = NULL, unsigned int *pnTimeSec = NULL, unsigned short *pnTimeMilSec = NULL, int *pnQuality = NULL); // 获取数据块信息。用于块设备进行按位控制时，先获取tag点对应的块的值，然后和tag点其余位操作
PKDRIVER_EXPORTS int	Drv_TagValStr2Bin(PKTAG *pTag, const char *szTagStrVal, char *szTagValBuff, int nTagValBuffLen, int *pnRetValBuffLenBytes, int *pnRetValBuffLenBits = NULL); // tag点值从字符串转换为二进制
																																													 // 串口设备有效，用于设置通讯连接状态超时， 超过该时间还未收到消息则会认为设备断开
PKDRIVER_EXPORTS int	Drv_SetConnectOKTimeout(PKDEVICE *pDevice, int nConnBadTimeoutSec); // 用于设置通讯连接状态超时， 超过该时间还未收到消息则会认为设备断开
PKDRIVER_EXPORTS int	Drv_SetConnectOK(PKDEVICE *pDevice); // 设置当前连接状态为：OK
#endif // _PK_DRVCOMMON_H_

