// OpStackGen.h: interface for the COpStackGen class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_OPCALCBASE_H__INCLUDED_)
#define AFX_OPCALCBASE_H__INCLUDED_

#include <string>
#include <sstream>
#include <vector>
using namespace std;

#define DOUBLE_EPSILON  1e-24			// 比较两个double见差值，小于该值则认为相等
#include "common/eviewdefine.h"
#include "pkdriver/pkdrvcmn.h"
// /变量/属性/变量域/变量组定义
#define PK_TAGFIELDNAME_MAXLEN			32	// 变量域的名称的最大长度
#define PK_LONGTAGNAME_MAXLEN			(PK_NAME_MAXLEN + PK_NAME_MAXLEN + PK_TAGFIELDNAME_MAXLEN)	// 节点名.变量名.域名
#define PK_LONGOBJTAGNAME_MAXLEN		(PK_LONGTAGNAME_MAXLEN + PK_NAME_MAXLEN)	// 节点名.对象名.变量名.域名

#define EXPRCALC_EXCEPT_QUALITY_BAD						10000
#define EXPRCALC_EXCEPT_DIVIDE_BY_ZERO					10001
#define EXPRCALC_EXCEPT_OPERATOR_NOT_IMPLEMENTED		10002

enum nodeEnum{ typeConst, typeVar, typeStr, typeOpr };

//常数类型节点
/* constants */ 
struct conNodeType
{
	double fValue;/* value of constant */ 
} ;

/* identifiers */ 
//struct idNodeType
//{ 
//	int i;/* subscript to sym array */ 
//} ;

//变量名称类型节点 
struct VariNodeType
{ 
	//变量名称
	char szVarName[PK_NAME_MAXLEN];
} ;

//字符串类型节点 
struct StrNodeType
{ 
	//字符串名称
	char szStrNodeName[PK_NAME_MAXLEN];
} ;

/* operators */ 
struct oprNodeType
{ 
	int oper;		/* operator */ 
	int nops;		/* number of operands */ 
	struct ExprNode *op[1];	/* operands (expandable) */ 
};

struct ExprNode
{
	nodeEnum type; /* type of node */ 

	/* union must be last entry in nodeType */ 
	/* because operNodeType may dynamically increase */ 
	union {
		conNodeType con;/* constants */ 
		//idNodeType	id;/* identifiers */ 
		VariNodeType var;
		StrNodeType str;
		oprNodeType opr;/* operators */ 
	};

	void setDouble(double dbValue)
	{
		type = typeConst;
		con.fValue = dbValue;
	}
} ;

class TagException : public std::exception
  {
  public:
	  int	 m_nErrCode;
	  string m_strErrTip;

  public:
    /** Takes a character string describing the error.  */
	 TagException(int nErrCode, const char *szExcept){m_nErrCode, m_strErrTip = szExcept;}

	virtual ~TagException()throw (){}

    /** Returns a C-style character string describing the general cause of
     *  the current error (the same string passed to the ctor).  */
	virtual const char*  what(){return m_strErrTip.c_str();}
	virtual const int errcode(){ return m_nErrCode; }
  };
  

class CExprCalc
{
public:
	CExprCalc();
	virtual ~CExprCalc();

// 调用者使用的函数
public:	
	virtual double		DoCalc(ExprNode *p);
	virtual double		GetVarValue(const char*) = 0;
	virtual ExprNode *	ParseExpr(const char * pszExpression);
	virtual bool		GetVars(ExprNode *p, vector<string> &vars);
	virtual bool		IsSuccess() { return m_strErrMsg.str().empty(); }
	virtual ExprNode *	GetRootNode() { return m_pRootNode; }
	virtual bool		ValidateVarName(const char*);
public:
	std::stringstream		m_strErrMsg;		// 错误信息
	std::string				m_strExpression;	// 待计算的表达式

	// yacc中使用的函数
public:
	virtual void	SaveRootNode(ExprNode *pRootNode);
	virtual long	eval_getinput(char *buf, int maxlen);
	virtual void	SetErrMsg(const char* szErrMsg);

protected:

	virtual int		DeleteNode(ExprNode *pRootNode);

	virtual bool	ValidateTree(ExprNode *p);

	// 表达式计算是否成功

	static void FreeNode(ExprNode *pNode);
	//////////////////////////////////////////////////
	// specified the input buffer

	// reset internal buffer
	virtual void		Reset();

	// set error msg
	virtual const char*	GetErrMsg(){ return m_strErrMsg.str().c_str(); }


protected:

	long		m_nExprEval;		// the buffer len to eval
	ExprNode *	m_pRootNode;
};

#endif // !defined(AFX_OPCALCBASE_H__INCLUDED_)
