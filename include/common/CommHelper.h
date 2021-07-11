/*!************************************************************
 *  @file		 CommHelper.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  @brief		 (ͨ�ò����궨��).
 *
 *  @author:     chenshengyu
 *  @version     03/12/2006  chenshengyu  Initial Version
 *  @version	 09/03/2006  chenshengyu  (ɾ��TRIM_CSTRING�꣺�������ƽ̨Ҫ��).
 *  @version     12/17/2007  chenshengyu  (���ӣ�UNREFERENCED_PARAMETER�����ڷ�WIN32ƽ̨).
 *  @version     05/27/2008  chenzhiquan  ���ӵ�Ԫ����������Ԫ�ĺ�.
**************************************************************/
#ifndef _COMM_HELPER_H_
#define _COMM_HELPER_H_

/************************************************************************/
/*						 ͨ�ò����궨��                                 */
/************************************************************************/

//////////////////////////////////////////////////////////////////////////
//	��ȫ��ɾ������
#define SAFE_DELETE(x)	\
{				\
if (x)			\
	delete x;	\
x = NULL;		\
}

//////////////////////////////////////////////////////////////////////////
// ��ȫ��ɾ���������
#define SAFE_DELETE_ARRAY(x)	\
{				\
if (x)			\
	delete[] x;	\
	x = NULL;	\
}

#define SAFE_FREE(x)	\
{					\
	if (x)			\
		free(x);	\
	x = NULL;		\
}

#ifdef CPPUNIT_FRIEND_CLASS
#define UNITTEST(cls) friend class cls##Tester
#else//CPPUNIT_FRIEND_CLASS
#define UNITTEST(cls)
#endif//CPPUNIT_FRIEND_CLASS

#ifndef _WIN32
	#ifndef UNREFERENCED_PARAMETER
		#define UNREFERENCED_PARAMETER(P)          (P)
	#endif
#endif

#define CHECK_ERROR_AND_RETURN_FAIL(X)	\
{										\
	if(X != 0)							\
		return X;						\
}

#ifdef _WIN32
#	define SEHTRY		__try
#	define SEHCATCH		__except(1)
#else
#	define SEHTRY		try
#	define SEHCATCH		catch(...)
#endif

#define BAD_FILE_ERRNO	-11111

#endif//#ifndef _COMM_HELPER_H_
