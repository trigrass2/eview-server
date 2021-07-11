/*!***********************************************************
 *  @file        OpStackGen.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  @brief       (Desp).
 *
 *  @author:     chenshengyu
 *  @version     04/09/2008  chenshengyu  Initial Version
**************************************************************/

#include "exprcalc/ExprCalc.h"
#include "parser.h"
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <stdio.h>

using std::string;
extern int yyparse(void);
extern void yyrestart( FILE *input_file );
typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern void yy_delete_buffer ( YY_BUFFER_STATE b );
extern YY_BUFFER_STATE yy_current_buffer;
#define YY_CURRENT_BUFFER yy_current_buffer

CExprCalc *g_pCurExprCalc = NULL;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
void Safe_CopyString(char *szDest, const char *szSrc, int nDestLen)
{
	PKStringHelper::Safe_StrNCpy(szDest, szSrc, nDestLen);
}

bool double_equal(double d1, double d2)
{
	return abs(d1 - d2) < 1e-24;
}

CExprCalc::CExprCalc()
{
	//m_pVarNameValidator = NULL;
	m_strErrMsg.str("");
	m_pRootNode = NULL;
	m_nExprEval = 0;
}

CExprCalc::~CExprCalc()
{
}

void CExprCalc::SetErrMsg( const char* szErrMsg)
{
	m_strErrMsg.clear();
	m_strErrMsg.str("");
	if(szErrMsg)
		m_strErrMsg << szErrMsg;
}

/**
 *  (calculate the node_stack).
 *  (!!! translate the node_stack to operator_stack + operand_stack, maybe more efficient).
 *
 *  @param  -[in]  nodeType*  p: [root for node_stack]
 *  @return (result).
 *
 */
bool CExprCalc::ValidateTree( ExprNode *p )
{
	if (!p)
		return false;
	
	switch(p->type)
	{
	case typeConst: 
		return true;
	case typeVar: 
		{
			bool bRet = this->ValidateVarName(p->var.szVarName);
			if (!bRet)
			{
				//std::string strErr("变量名称不合法: ");
				//strErr = strErr + p->var.szVarName;
				//this->SetErrMsg(strErr.c_str());
				return bRet;
			}
		}
		return true;
	case typeStr: 
		return true;

	case typeOpr: 
		switch(p->opr.oper)
		{ 
		case ' ': return ValidateTree(p->opr.op[0])&& ValidateTree(p->opr.op[1]);
		case '=': return ValidateTree(p->opr.op[1]);
		case UMINUS: 
		case NOT: 
		case '~':
			return ValidateTree(p->opr.op[0]);
		case '+': 
		case '-': 
		case '*': 
		case '/': 
		case '%': 
		case '<': 
		case '>':
		case '&':
		case '|':
		case '^':
		case GR_EQ: 
		case LE_EQ: 
		case NEQ: 
		case EQ: 
		case POW_FUNC: 
		case SHIFT_R:
		case SHIFT_L:
		case FMOD:
		case AND:
		case OR:
		case XOR:
			return ValidateTree(p->opr.op[0]) && ValidateTree(p->opr.op[1]);
			
		case SIN: 
		case COS: 
		case TAN: 
		case ASIN:
		case ACOS:
		case ATAN:
		case EXP: 
		case LOG: 
		case LOG10: 
		case ABS:
		case GETINT:
		case GETBOOL:
		case SQRT:
			return ValidateTree(p->opr.op[0]);
		case QC: 
			{
				return ValidateTree(p->opr.op[0]) && ValidateTree(p->opr.op[1]) && ValidateTree(p->opr.op[2]);
			}
		default:
			m_strErrMsg << ("invalid operator id = ") << p->opr.oper << "\t";
			return false;
		}

	} 
	m_strErrMsg << ("invalid type type = ") << p->type << "\t";
	return false;
}

/**
 *  (calculate the node_stack).
 *  (!!! translate the node_stack to operator_stack + operand_stack, maybe more efficient).
 *
 *  @param  -[in]  nodeType*  p: [root for node_stack]
 *  @return (result).
 *
 */
bool CExprCalc::GetVars(ExprNode *p, vector<string> &vars)
{
	if (!p)
		return false;
	
	switch(p->type)
	{
	case typeConst: 
		return true;
	case typeVar: 
		{
			bool bRet = this->ValidateVarName(p->var.szVarName);
			if (!bRet)
			{
				//std::string strErr("变量名称不合法: ");
				//strErr = strErr + p->var.szVarName;
				//this->SetErrMsg(strErr.c_str());
				return bRet;
			}
			vars.push_back(p->var.szVarName);
		}
		return true;
	case typeStr: 
		return true;

	case typeOpr: 
		switch(p->opr.oper)
		{ 
		case ' ': return GetVars(p->opr.op[0], vars)&& GetVars(p->opr.op[1], vars);
		case '=': return GetVars(p->opr.op[1], vars);
		case UMINUS: 
		case NOT: 
		case '~':
			return GetVars(p->opr.op[0], vars);
		case '+': 
		case '-': 
		case '*': 
		case '/': 
		case '%': 
		case '<': 
		case '>':
		case '&':
		case '|':
		case '^':
		case GR_EQ: 
		case LE_EQ: 
		case NEQ: 
		case EQ: 
		case POW_FUNC: 
		case SHIFT_R:
		case SHIFT_L:
		case FMOD:
		case AND:
		case OR:
		case XOR:
			return GetVars(p->opr.op[0], vars) && GetVars(p->opr.op[1], vars);
			
		case SIN: 
		case COS: 
		case TAN: 
		case ASIN:
		case ACOS:
		case ATAN:
		case EXP: 
		case LOG: 
		case LOG10: 
		case ABS:
		case GETINT:
		case GETBOOL:
		case SQRT:
			return GetVars(p->opr.op[0], vars);
		case QC: 
			{
				return GetVars(p->opr.op[0], vars) && GetVars(p->opr.op[1], vars) && GetVars(p->opr.op[2], vars);
			}
		default:
			m_strErrMsg << ("invalid operator id = ") << p->opr.oper << "\t";
			return false;
		}

	} 
	m_strErrMsg << ("invalid type type = ") << p->type << "\t";
	return false;
}

ExprNode * CExprCalc::ParseExpr( const char * pszExpression )
{
	Reset();

	if (!pszExpression || strlen(pszExpression) == 0)
		return NULL;

	m_strExpression = pszExpression;

	g_pCurExprCalc = this; // 必须赋值，因为在yacc中用到这个全局变量
	// *  @version  08/03/2005  chenshengyu  $(!!!Important, need yyrestart to empty the buffer).
	yyrestart(NULL);
	if (yyparse() != 0)
		return NULL;
	if(m_strErrMsg.str().length() != 0)
		return NULL;

	bool bRet = ValidateTree(m_pRootNode);
	if (bRet)
	{
		return m_pRootNode;
	}
	else
	{
		FreeNode(m_pRootNode);
		m_pRootNode = NULL;
		return NULL;
	}
}

bool CExprCalc::ValidateVarName( const char* szVarName)
{
	if(!szVarName || strlen(szVarName) == 0)
		return false;
	return true;
} 

int CExprCalc::DeleteNode( ExprNode *pRootNode )
{
	if(pRootNode == NULL)
		return 0;

	//根节点类型
	switch(pRootNode->type)
	{
	case typeConst://常量
	case typeVar://变量
	case typeStr:
		break;
	case typeOpr://二维表达式计算
		DeleteNode(pRootNode->opr.op[0]);
		if (pRootNode->opr.nops >= 2)
			DeleteNode(pRootNode->opr.op[1]);
		break;
	default:
		break;
	}

	delete pRootNode;
	pRootNode = NULL;
	return 0;
}


long CExprCalc::eval_getinput(char *buf, int maxlen)
{
	memset(buf, 0, maxlen);

	long nLenCalc = 0;

	string strTemp;
	strTemp = m_strExpression.c_str() + m_nExprEval;

	PKStringHelper::Safe_StrNCpy(buf, strTemp.c_str(), maxlen);
	nLenCalc = strlen(buf);

	m_nExprEval += nLenCalc;

	return nLenCalc;
}


void CExprCalc::SaveRootNode(ExprNode *pRootNode)
{
	m_pRootNode = pRootNode;
}

void CExprCalc::Reset()
{
	// 初始化
	m_strErrMsg.clear(); //m_strErrMsg.begin(), m_strErrMsg.end());
	m_nExprEval = 0;
	m_pRootNode = NULL;
}

void CExprCalc::FreeNode( ExprNode *p)
{
	if (!p) 
		return;

	if (p->type == typeOpr)
	{ 
		for (int i = 0;i < p->opr.nops;i++) 
			FreeNode(p->opr.op[i]);
	} 
	free (p);
}

/*
"$", "error", "$undefined.", "NUMBER",
"VARIABLE", "STRING", "QC", "'?'", "':'", "OR", "AND", "XOR", "'|'", "'^'", "'&'", "NEQ",
"EQ", "'<'", "'>'", "LE_EQ", "GR_EQ", "SHIFT_L", "SHIFT_R", "'+'", "'-'", "'*'", "'/'",
"'%'", "UMINUS", "NOT", "'~'", "SIN", "COS", "TAN", "LOG", "ASIN", "ACOS", "ATAN", "LOG10",
"GETINT", "GETBOOL", "ABS", "SQRT", "EXP", "POW_FUNC", "FMOD", "'('", "')'", "','", "stat",
"expr", ""
*/
/**
 *  (calculate the node_stack).
 *  (!!! translate the node_stack to operator_stack + operand_stack, maybe more efficient).
 *
 *  @param  -[in]  nodeType*  p: [root for node_stack]
 *  @return (result).
 *
 *  @version  04/10/2008  chenshengyu  Initial Version.
 */
double CExprCalc::DoCalc( ExprNode *p )
{
	if (!p)
		return 0;

	switch(p->type)
	{
	case typeConst: 
		return p->con.fValue;
	case typeVar:
		{
			double dbValue = this->GetVarValue(p->var.szVarName);	
			//printf("DoCalcing:%s,value:%f\n", p->var.szVarName, dbValue);
			return dbValue;
		}
	case typeOpr: 
		switch(p->opr.oper)
		{ 
		//TODO: 空格的Operator？
		//case ' ': DoCalcTree(p->opr.op[0]);
		//	return DoCalcTree(p->opr.op[1]);
		//TODO: 不支持賦值操作case '=': return SymTable[p->opr.op[0]->id.i] = CalcTree(p->opr.op[1]);
		case UMINUS: 
			{
				return - DoCalc(p->opr.op[0]);
			}
		case '+':
		{
			double dbVal1 = DoCalc(p->opr.op[0]);
			double dbVal2 = DoCalc(p->opr.op[1]);
			//printf("+,%f,%f\n", dbVal1, dbVal2);
			return dbVal1 + dbVal2;
		}
		case '-': 
			return DoCalc(p->opr.op[0]) - DoCalc(p->opr.op[1]);
		case '*': 
			return DoCalc(p->opr.op[0]) * DoCalc(p->opr.op[1]);
		case '/': 
			if(DoCalc(p->opr.op[1]) < 1e-10)
			{
				SetErrMsg(("divisor is 0\n"));
				throw TagException(EXPRCALC_EXCEPT_DIVIDE_BY_ZERO, "divisor is 0");
				return 0;
			}
			return DoCalc(p->opr.op[0]) / DoCalc(p->opr.op[1]);
		case '%':
			if(((long)DoCalc(p->opr.op[1])) == 0)
			{
				SetErrMsg(("divisor is 0\n"));
				throw TagException(EXPRCALC_EXCEPT_DIVIDE_BY_ZERO, "divisor is 0");
				return 0;
			}
			return ((long)DoCalc(p->opr.op[0])) % ((long)DoCalc(p->opr.op[1]));
		case '<': 
			return DoCalc(p->opr.op[0]) < DoCalc(p->opr.op[1]);
		case '>': 
			return DoCalc(p->opr.op[0]) > DoCalc(p->opr.op[1]);
		case '&': 
			return (double)((int)DoCalc(p->opr.op[0]) & (int)DoCalc(p->opr.op[1]));
		case '|': 
			return (double)((int)DoCalc(p->opr.op[0]) | (int)DoCalc(p->opr.op[1]));
		case '~': 
			return (double)(~(int)DoCalc(p->opr.op[0]));
		case '^': 
			return (double)((int)DoCalc(p->opr.op[0]) ^ (int)DoCalc(p->opr.op[1]));
		
		case SHIFT_L: 
			return (int)DoCalc(p->opr.op[0]) << (int)DoCalc(p->opr.op[1]);
		case SHIFT_R: 
			return (int)DoCalc(p->opr.op[0]) >> (int)DoCalc(p->opr.op[1]);
		case GR_EQ: 
			return DoCalc(p->opr.op[0]) >= DoCalc(p->opr.op[1]);
		case LE_EQ: 
			return DoCalc(p->opr.op[0]) <= DoCalc(p->opr.op[1]);
		case NEQ: 
			return !double_equal(DoCalc(p->opr.op[0]), DoCalc(p->opr.op[1]));
		case EQ: 
			return double_equal(DoCalc(p->opr.op[0]), DoCalc(p->opr.op[1]));

		case NOT: 
			return DoCalc(p->opr.op[0]) == 0;
		case AND: 
			return double((int) DoCalc(p->opr.op[0]) && (int)DoCalc(p->opr.op[1]));
		case OR: 
			return double((int) DoCalc(p->opr.op[0]) || (int)DoCalc(p->opr.op[1]));
		case XOR: 
			return double( ((int)DoCalc(p->opr.op[0]) != 0) ^ ((int)DoCalc(p->opr.op[1]) != 0));
			
		case SIN: 
			return sin(DoCalc(p->opr.op[0]));
		case ABS: 
			return abs(DoCalc(p->opr.op[0]));
		case COS: 
			return cos(DoCalc(p->opr.op[0]));
		case TAN: 
			return tan(DoCalc(p->opr.op[0]));
		case ASIN: 
			return asin(DoCalc(p->opr.op[0]));
		case ACOS: 
			return acos(DoCalc(p->opr.op[0]));
		case ATAN:
			return atan(DoCalc(p->opr.op[0]));
		case EXP: 
			return exp( DoCalc(p->opr.op[0]) );
		case GETINT: 
			return (long)(DoCalc(p->opr.op[0]));
		case GETBOOL:
			return fabs(DoCalc(p->opr.op[0])) < 1e-10 ? 0:1;
		case SQRT: 
			return sqrt(DoCalc(p->opr.op[0]));
		case FMOD: 
			return fmod(DoCalc(p->opr.op[0]), DoCalc(p->opr.op[1]));
		case POW_FUNC: 
			return pow( DoCalc(p->opr.op[0]), DoCalc(p->opr.op[1]) );
		case LOG: 
			return log(DoCalc(p->opr.op[0]));
		case LOG10: 
			return log10(DoCalc(p->opr.op[0]));
		case QC:// ?: 问号冒号－－－三元操作符
			{
				if ((long)DoCalc(p->opr.op[0]) != 0)
					return DoCalc(p->opr.op[1]);
				else
					return DoCalc(p->opr.op[2]);
			}/**/
		default:
			m_strErrMsg << ("operator not implemented id = ") << p->opr.oper << "\t";
			char szExcept[256] = {0};
			sprintf(szExcept, "operator not implemented id = %d", p->opr.oper);
			throw TagException(EXPRCALC_EXCEPT_OPERATOR_NOT_IMPLEMENTED, szExcept);
		} 
	} 
	
	return 0;
}
