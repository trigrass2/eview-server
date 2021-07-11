/*
RDA : Remote Data Access, 远程数据访问。 wuhaochi notes
*/

#ifndef _CVLDA_H
#define _CVLDA_H

#ifdef _WIN32
#	ifdef cvlda_EXPORTS
#		define LDA_API extern "C" __declspec(dllexport)
#	else
#		define LDA_API extern "C" __declspec(dllimport)
#	endif
#else//_WIN32
#	define LDA_API extern "C"
#endif//_WIN32

//RETURN CODE OF REDUNDANCY STATUS
#define LDA_SUCCESS					0		// 0 表示调用成功
#define LDA_RMSTATUS_INACTIVE		0		// 非活动主机
#define LDA_RMSTATUS_ACTIVE			1		// 活动主机
#define LDA_RMSTATUS_UNAVALIBLE		2		// 冗余状态未知，可能是冗余服务未启动或 正在协商中。按照活动状态激活

/************************************************************************/
/*  Global interface APIs: external local db accessor                   */
/************************************************************************/

// Initialize
LDA_API int LDA_Init(long *pnHandle);

// Release
LDA_API int LDA_Release(long lHandle);

// 获取当前的冗余状态。值的含义见本文件头定义
LDA_API int LDA_GetRMStatus(int *pnStatus);

// 读取tag点的值
LDA_API int LDA_ReadValue(char *szTagNameWithoutScada, char *szFieldName, char *szValueBuf, int *pnValueBufLen);

// 写tag点的值。超时单位为毫秒，预留
LDA_API int LDA_WriteValue(char *szTagNameWithoutScada, char *szFieldName, char *szValueBuf, int nMSTimeOut);

#endif // LDA_API


