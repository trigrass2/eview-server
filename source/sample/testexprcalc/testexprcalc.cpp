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
	// �������false���򽫲����м��㲢�޷���ȡ��root
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
	g_mapTagName2Value["����.test1"] = 100;
	g_mapTagName2Value["test2"] = 200;

	CCAExprCalc calc;
	ExprNode *node = calc.ParseExpr("bool(����.test1+test2+test3)");
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
	printf( "\n����.��.����.�ַ�" );
	bRet = g_pEvaluator->ParseExpr("����.��.����.�ַ�");
	CPPUNIT_ASSERT(bRet);
	printf("\tPASS");

	//case 4
	printf( "\n&!@.abc.ef.g" );
	bRet = g_pEvaluator->ParseExpr("&!@.abc.ef.g");
	CPPUNIT_ASSERT(!bRet);
	printf("\tPASS");

	//case 5
	printf( "\n$��" );
	bRet = g_pEvaluator->ParseExpr("$��");
	CPPUNIT_ASSERT(bRet);
	printf("\tPASS");

	//case 6
	printf( "\n$������" );
	bRet = g_pEvaluator->ParseExpr("$������");
	CPPUNIT_ASSERT(bRet);
	printf("\tPASS");

	//case 7
	printf( "\n$ũ����" );
	bRet = g_pEvaluator->ParseExpr("$ũ����");
	CPPUNIT_ASSERT(bRet);
	printf("\tPASS");

	//case 8
	printf( "\n$ũ_����" );
	bRet = g_pEvaluator->ParseExpr("$ũ_����");
	CPPUNIT_ASSERT(bRet);
	printf("\tPASS");

	//case 9
	printf( "\n$ũ*����" );
	bRet = g_pEvaluator->ParseExpr("$ũ*����");
	CPPUNIT_ASSERT(!bRet);
	printf("\tPASS");

	//case 10
	printf( "\n$ũ AND a.b.c" );
	bRet = g_pEvaluator->ParseExpr("$ũ AND a.b.c");
	CPPUNIT_ASSERT(bRet);
	printf("\tPASS");

	//case 11
	printf( "\n$ũ && a.b.c" );
	bRet = g_pEvaluator->ParseExpr("$ũ && a.b.c");
	CPPUNIT_ASSERT(bRet);
	printf("\tPASS");

	bRet = g_pEvaluator->ParseExpr("$ũ && a.b.c");
}
*/

