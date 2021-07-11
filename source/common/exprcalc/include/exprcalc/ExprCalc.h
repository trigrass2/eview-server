// OpStackGen.h: interface for the COpStackGen class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_OPCALCBASE_H__INCLUDED_)
#define AFX_OPCALCBASE_H__INCLUDED_

#include <string>
#include <sstream>
#include <vector>
using namespace std;

#define DOUBLE_EPSILON  1e-24			// �Ƚ�����double����ֵ��С�ڸ�ֵ����Ϊ���
#include "common/eviewdefine.h"
#include "pkdriver/pkdrvcmn.h"
// /����/����/������/�����鶨��
#define PK_TAGFIELDNAME_MAXLEN			32	// ����������Ƶ���󳤶�
#define PK_LONGTAGNAME_MAXLEN			(PK_NAME_MAXLEN + PK_NAME_MAXLEN + PK_TAGFIELDNAME_MAXLEN)	// �ڵ���.������.����
#define PK_LONGOBJTAGNAME_MAXLEN		(PK_LONGTAGNAME_MAXLEN + PK_NAME_MAXLEN)	// �ڵ���.������.������.����

#define EXPRCALC_EXCEPT_QUALITY_BAD						10000
#define EXPRCALC_EXCEPT_DIVIDE_BY_ZERO					10001
#define EXPRCALC_EXCEPT_OPERATOR_NOT_IMPLEMENTED		10002

enum nodeEnum{ typeConst, typeVar, typeStr, typeOpr };

//�������ͽڵ�
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

//�����������ͽڵ� 
struct VariNodeType
{ 
	//��������
	char szVarName[PK_NAME_MAXLEN];
} ;

//�ַ������ͽڵ� 
struct StrNodeType
{ 
	//�ַ�������
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

// ������ʹ�õĺ���
public:	
	virtual double		DoCalc(ExprNode *p);
	virtual double		GetVarValue(const char*) = 0;
	virtual ExprNode *	ParseExpr(const char * pszExpression);
	virtual bool		GetVars(ExprNode *p, vector<string> &vars);
	virtual bool		IsSuccess() { return m_strErrMsg.str().empty(); }
	virtual ExprNode *	GetRootNode() { return m_pRootNode; }
	virtual bool		ValidateVarName(const char*);
public:
	std::stringstream		m_strErrMsg;		// ������Ϣ
	std::string				m_strExpression;	// ������ı��ʽ

	// yacc��ʹ�õĺ���
public:
	virtual void	SaveRootNode(ExprNode *pRootNode);
	virtual long	eval_getinput(char *buf, int maxlen);
	virtual void	SetErrMsg(const char* szErrMsg);

protected:

	virtual int		DeleteNode(ExprNode *pRootNode);

	virtual bool	ValidateTree(ExprNode *p);

	// ���ʽ�����Ƿ�ɹ�

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
