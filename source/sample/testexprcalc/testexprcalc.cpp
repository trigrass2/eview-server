// TestEAAction.cxx: implementation of the CTestLogicExpr class.
//
//////////////////////////////////////////////////////////////////////
#include "exprcalc/ExprCalc.h"
#include <map>
#include <string>
#include <vector>

using namespace std;
#define _(X)	(X)

map<string,double>	g_mapTagName2Value;


class CCAExprCalc : public CExprCalc
{
public:
	CCAExprCalc();
	virtual ~CCAExprCalc();
	virtual bool ValidateVarName( const char* szVarName);
	virtual double GetVarValue(const char* szVar);
};

CCAExprCalc::CCAExprCalc()
{
}

CCAExprCalc::~CCAExprCalc()
{

}

bool CCAExprCalc::ValidateVarName( const char* szVarName)
{
	return true;
	// 如果返回false，则将不进行计算并无法获取到root
	map<string,double>::iterator itMap = g_mapTagName2Value.find(szVarName);
	if(itMap == g_mapTagName2Value.end())
		return false;
	return true;
}

double CCAExprCalc::GetVarValue(const char* szVar)
{
	map<string,double>::iterator itMap = g_mapTagName2Value.find(szVar);
	if(itMap == g_mapTagName2Value.end())
	{
		m_strErrMsg <<_( "get variable '") << szVar << _("' failed: no speified value") << "\t";
		return 0.f;
	}
	return itMap->second;
}

int main()
{
	g_mapTagName2Value["中文.test1"] = 100;
	g_mapTagName2Value["test2"] = 200;

	CCAExprCalc calc;
	ExprNode *node = calc.ParseExpr("bool(中文.test1+test2+test3)");
	vector<string> vars;
	bool bRet = calc.GetVars(node, vars);
	double result = calc.DoCalc(node);
	printf("result is:%f", result);
	return 0;
};


/*
void CTestLogicExpr::Test_ParseExpr()
{
	bool bRet;

	//case 1
	printf( "\nscada.tag.field" );
	bRet = g_pEvaluator->ParseExpr("scada.tag.field");
	CPPUNIT_ASSERT(bRet);
	printf("\t\tPASS");

	//case 2
	printf( "\nscada.obj.selfdeffield.field" );
	bRet = g_pEvaluator->ParseExpr("scada.obj.selfdeffield.field");
	CPPUNIT_ASSERT(bRet);
	printf("\tPASS");

	//case 3
	printf( "\n中文.在.特殊.字符" );
	bRet = g_pEvaluator->ParseExpr("中文.在.特殊.字符");
	CPPUNIT_ASSERT(bRet);
	printf("\tPASS");

	//case 4
	printf( "\n&!@.abc.ef.g" );
	bRet = g_pEvaluator->ParseExpr("&!@.abc.ef.g");
	CPPUNIT_ASSERT(!bRet);
	printf("\tPASS");

	//case 5
	printf( "\n$年" );
	bRet = g_pEvaluator->ParseExpr("$年");
	CPPUNIT_ASSERT(bRet);
	printf("\tPASS");

	//case 6
	printf( "\n$年月日" );
	bRet = g_pEvaluator->ParseExpr("$年月日");
	CPPUNIT_ASSERT(bRet);
	printf("\tPASS");

	//case 7
	printf( "\n$农历年" );
	bRet = g_pEvaluator->ParseExpr("$农历年");
	CPPUNIT_ASSERT(bRet);
	printf("\tPASS");

	//case 8
	printf( "\n$农_历年" );
	bRet = g_pEvaluator->ParseExpr("$农_历年");
	CPPUNIT_ASSERT(bRet);
	printf("\tPASS");

	//case 9
	printf( "\n$农*历年" );
	bRet = g_pEvaluator->ParseExpr("$农*历年");
	CPPUNIT_ASSERT(!bRet);
	printf("\tPASS");

	//case 10
	printf( "\n$农 AND a.b.c" );
	bRet = g_pEvaluator->ParseExpr("$农 AND a.b.c");
	CPPUNIT_ASSERT(bRet);
	printf("\tPASS");

	//case 11
	printf( "\n$农 && a.b.c" );
	bRet = g_pEvaluator->ParseExpr("$农 && a.b.c");
	CPPUNIT_ASSERT(bRet);
	printf("\tPASS");

	bRet = g_pEvaluator->ParseExpr("$农 && a.b.c");
}
*/

