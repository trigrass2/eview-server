// RegularDllExceptionAttacher.h: interface for the RegularDllExceptionAttacher class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_REGULARDLLEXCEPTIONATTACHER_H__A12A93C6_801D_4102_8671_2A8AE1B5FCB1__INCLUDED_)
#define AFX_REGULARDLLEXCEPTIONATTACHER_H__A12A93C6_801D_4102_8671_2A8AE1B5FCB1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CRegularDllExceptionAttacher  
{
public:
	CRegularDllExceptionAttacher(long IsPopExceptDlg = 0);	// 是否弹出异常对话框。0表示不弹出，1表示弹出
	virtual ~CRegularDllExceptionAttacher();
};

#endif // !defined(AFX_REGULARDLLEXCEPTIONATTACHER_H__A12A93C6_801D_4102_8671_2A8AE1B5FCB1__INCLUDED_)
