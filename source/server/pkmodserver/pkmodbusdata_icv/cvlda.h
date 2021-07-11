/*
RDA : Remote Data Access, Զ�����ݷ��ʡ� wuhaochi notes
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
#define LDA_SUCCESS					0		// 0 ��ʾ���óɹ�
#define LDA_RMSTATUS_INACTIVE		0		// �ǻ����
#define LDA_RMSTATUS_ACTIVE			1		// �����
#define LDA_RMSTATUS_UNAVALIBLE		2		// ����״̬δ֪���������������δ������ ����Э���С����ջ״̬����

/************************************************************************/
/*  Global interface APIs: external local db accessor                   */
/************************************************************************/

// Initialize
LDA_API int LDA_Init(long *pnHandle);

// Release
LDA_API int LDA_Release(long lHandle);

// ��ȡ��ǰ������״̬��ֵ�ĺ�������ļ�ͷ����
LDA_API int LDA_GetRMStatus(int *pnStatus);

// ��ȡtag���ֵ
LDA_API int LDA_ReadValue(char *szTagNameWithoutScada, char *szFieldName, char *szValueBuf, int *pnValueBufLen);

// дtag���ֵ����ʱ��λΪ���룬Ԥ��
LDA_API int LDA_WriteValue(char *szTagNameWithoutScada, char *szFieldName, char *szValueBuf, int nMSTimeOut);

#endif // LDA_API


