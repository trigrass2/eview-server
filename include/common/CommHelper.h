/*!************************************************************
 *  @file		 CommHelper.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  @brief		 (通用操作宏定义).
 *
 *  @author:     chenshengyu
 *  @version     03/12/2006  chenshengyu  Initial Version
 *  @version	 09/03/2006  chenshengyu  (删除TRIM_CSTRING宏：不满足跨平台要求).
 *  @version     12/17/2007  chenshengyu  (增加：UNREFERENCED_PARAMETER，用于非WIN32平台).
 *  @version     05/27/2008  chenzhiquan  增加单元测试声明友元的宏.
**************************************************************/
#ifndef _COMM_HELPER_H_
#define _COMM_HELPER_H_

/************************************************************************/
/*						 通用操作宏定义                                 */
/************************************************************************/

//////////////////////////////////////////////////////////////////////////
//	安全地删除对象
#define SAFE_DELETE(x)	\
{				\
if (x)			\
	delete x;	\
x = NULL;		\
}

//////////////////////////////////////////////////////////////////////////
// 安全地删除数组对象
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
