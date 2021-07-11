%{
#include <stdio.h> 
#include <stdlib.h> 
#include <stdarg.h> 
#include "exprcalc/ExprCalc.h"

/* prototypes */ 
	ExprNode *CreateOperNode(int oper, int nops, ...);
	ExprNode *CreateVariNode(const char* pVariName);
	ExprNode *CreateStrNode(const char* pString);
	ExprNode *CreateConstNode(double fValue);

	double ex(ExprNode *p);

	int yylex(void);
	void yyerror(char *s);

	extern CExprCalc *g_pCurExprCalc;
    extern void Safe_CopyString(char *szDest, const char *szSrc, int nDestLen);
%}

%union {
	double	fValue;		/* float value */ 
	char		szVarName[ICV_LONGOBJTAGNAME_MAXLEN];		/* virable name */ 
	char		szStrNodeName[ICV_TXTVALUE_MAXLEN + 2];		/* string, +'2' for ' begin and after the string */ 
	ExprNode *pExprNode;		/* node pointer */ 
};

%token <fValue> NUMBER
%token <szVarName> VARIABLE
%token <szStrNodeName> STRING
%token QC

%left '?' ':'
%left OR 
%left AND
%left XOR
%left '|'
%left '^'
%left '&'
%left  NEQ EQ
%left '<' '>' LE_EQ GR_EQ
%left SHIFT_L SHIFT_R
%left '+' '-'
%left '*' '/' '%'
%nonassoc UMINUS
%nonassoc NOT
%nonassoc '~'
%nonassoc SIN COS TAN LOG ASIN ACOS ATAN LOG10 GETINT GETBOOL ABS SQRT EXP POW_FUNC FMOD 

%type <pExprNode> stat expr

%%

stat : expr	{ g_pCurExprCalc->SaveRootNode($1); }

expr : 	NUMBER	{ 
					//printf("Syntex:NUMBER %f\n", $1); 
					$$ = CreateConstNode($1);
				}
		| VARIABLE			{ 
								//printf("Syntex:VARIABLE %s\n", $1); 
								$$ = CreateVariNode($1);
							}
		|STRING				{
								//printf("Syntex: STRING %s\n", $1);
								$$ = CreateStrNode($1);
							}
		| expr '?' expr ':' expr { 
								//printf("Syntex:NOT \n");
								$$ = CreateOperNode(QC, 3, $1, $3, $5);
							}
							
		|'(' expr ')'		{ 
								//printf("Syntex:() \n");  
								$$ = $2;
							}
		|'-' expr		{ 
								//printf("Syntex: - \n");  
								$$ = CreateOperNode(UMINUS, 1, $2);  
							}
		|'~' expr		{ 
								//printf("Syntex: ~ (bit not) \n");  
								$$ = CreateOperNode('~', 1, $2);  
							}
		| expr '&' expr		{ 
								//printf("Syntex: & \n");
								$$ = CreateOperNode('&', 2, $1, $3);  
							}
		| expr '|' expr		{ 
								//printf("Syntex: | \n");
								$$ = CreateOperNode('|', 2, $1, $3);  
							}
		| expr '^' expr		{ 
								//printf("Syntex: ^ \n");
								$$ = CreateOperNode('^', 2, $1, $3);  
							}
		| expr '<' expr		{ 
								//printf("Syntex:<  \n");
								$$ = CreateOperNode('<', 2, $1, $3);  
							}

		| expr '>' expr		{ 
								//printf("Syntex:>  \n");  
								$$ = CreateOperNode('>', 2, $1, $3);
							}
		| expr LE_EQ expr	{ 
								//printf("Syntex:<= \n"); 
								$$ = CreateOperNode(LE_EQ, 2, $1, $3); 
							}
		| expr GR_EQ expr	{ 
								//printf("Syntex:>= \n"); 
								$$ = CreateOperNode(GR_EQ, 2, $1, $3);
							}
		| expr NEQ expr		{ 
								//printf("Syntex:!= \n"); 
								$$ = CreateOperNode(NEQ, 2, $1, $3);
							}
		| expr EQ expr		{ 
								//printf("Syntex:== \n"); 
								$$ = CreateOperNode(EQ, 2, $1, $3);
							}
		| expr '+' expr		{ 
								//printf("Syntex:+  \n"); 
								$$ = CreateOperNode('+', 2, $1, $3); 
							}
		| expr '-' expr		{ 
								//printf("Syntex:-  \n"); 
								$$ = CreateOperNode('-', 2, $1, $3);
							}
		| expr '*' expr		{ 
								//printf("Syntex:*  \n"); 
								$$ = CreateOperNode('*', 2, $1, $3); 
							}
		| expr '/' expr		{ 
								//printf("Syntex:/  \n"); 
								$$ = CreateOperNode('/', 2, $1, $3);
							}
		| expr '%' expr		{ 
								//printf("Syntex:%  \n"); 
								$$ = CreateOperNode('%', 2, $1, $3);
							}
		| expr SHIFT_R expr	{ 
								//printf("Syntex:<< \n"); 
								$$ = CreateOperNode(SHIFT_R, 2, $1, $3);
							} 
		| expr SHIFT_L expr		{ 
								//printf("Syntex:>> \n"); 
								$$ = CreateOperNode(SHIFT_L, 2, $1, $3);
							} 

		| SIN '(' expr ')'	{
								//printf("Syntex:SIN  \n");
								$$ = CreateOperNode(SIN, 1, $3); 
							}
		| COS '(' expr ')'	{
								//printf("Syntex:COS  \n");
								$$ = CreateOperNode(COS, 1, $3); 
							}
		| TAN '(' expr ')'	{
								//printf("Syntex:TAN  \n");
								$$ = CreateOperNode(TAN, 1, $3);
							}
		| LOG '(' expr ')'	{
								//printf("Syntex:LOG  \n");
								$$ = CreateOperNode(LOG, 1, $3);
							}
		| ASIN '(' expr ')'	{
								//printf("Syntex:ASIN  \n");
								$$ = CreateOperNode(ASIN, 1, $3);
							}
		| ACOS '(' expr ')'	{
								//printf("Syntex:ACOS  \n");
								$$ = CreateOperNode(ACOS, 1, $3);
							}
		| ATAN '(' expr ')'	{
								//printf("Syntex:ATAN  \n");
								$$ = CreateOperNode(ATAN, 1, $3);
							}
		| LOG10 '(' expr ')'	{
									//printf("Syntex:LOG10  \n");
									$$ = CreateOperNode(LOG10, 1, $3);
								}
		| GETINT '(' expr ')'	{
									//printf("Syntex:INT  \n");
									$$ = CreateOperNode(GETINT, 1, $3);
								}
		| GETBOOL '(' expr ')'	{
									//printf("Syntex:BOOL  \n");
									$$ = CreateOperNode(GETBOOL, 1, $3);
								}
		| ABS '(' expr ')'	{
								//printf("Syntex:ABS  \n");
								$$ = CreateOperNode(ABS, 1, $3);
							}
		| SQRT '(' expr ')'	{
								//printf("Syntex:SQRT  \n");
								$$ = CreateOperNode(SQRT, 1, $3);
							}
		| EXP '(' expr ')'	{
								//printf("Syntex:EXP  \n");
								$$ = CreateOperNode(EXP, 1, $3);
							}
		| FMOD '(' expr ',' expr ')'	{ 
								//printf("Syntex:&& \n"); 
								$$ = CreateOperNode(FMOD, 2, $3, $5);
							}
		| POW_FUNC '(' expr ',' expr ')'	{ 
								//printf("Syntex:&& \n"); 
								$$ = CreateOperNode(POW_FUNC, 2, $3, $5);
							}
		| expr AND expr		{ 
								//printf("Syntex:&& \n"); 
								$$ = CreateOperNode(AND, 2, $1, $3);
							}
		| expr OR expr		{ 
								//printf("Syntex:|| \n"); 
								$$ = CreateOperNode(OR, 2, $1, $3);
							} 
		| expr XOR expr		{	
								//printf("Syntex:XOR \n"); 
								$$ = CreateOperNode(XOR, 2, $1, $3);
							}
		| NOT expr			{ 
								//printf("Syntex:NOT \n");
								$$ = CreateOperNode(NOT, 1, $2);
							}
		;
%%

#define SIZEOF_NODETYPE ((char *)&p->con - (char *)p) 

ExprNode *CreateConstNode(double fValue)
{ 
	ExprNode *p = NULL;
	long nodeSize;
	/* allocate node */ 
	nodeSize = SIZEOF_NODETYPE + sizeof(conNodeType);

	if ((p = (ExprNode *)malloc(nodeSize)) == NULL) 
		yyerror("out of memory");

	/* copy information */ 
	p->type = typeConst;
	p->con.fValue = fValue;

	return p;
} 

ExprNode* CreateVariNode(const char* pVariName)
{
	ExprNode *p = NULL;
	long nodeSize;
	/* allocate node */ 
	nodeSize = SIZEOF_NODETYPE + sizeof(VariNodeType);

	if ((p = (ExprNode *)malloc(nodeSize)) == NULL) 
		yyerror("out of memory");

	/* copy information */ 
	p->type = typeVar;
	Safe_CopyString(p->var.szVarName, pVariName, sizeof(p->var.szVarName));

	return p;
}

ExprNode *CreateStrNode(const char* pString)
{ 
	ExprNode *p = NULL;
	size_t nodeSize;

	/* allocate node */ 
	nodeSize = SIZEOF_NODETYPE + sizeof(StrNodeType);

	if ((p = (ExprNode *)malloc(nodeSize)) == NULL) 
		yyerror("out of memory");

	/* copy information */ 
	p->type = typeStr;
	
	int nIndex = 1; // omit first '
	int nTgtIndex = 0;
	while(pString[nIndex] != '\0' && nTgtIndex < sizeof(p->str.szStrNodeName))
	{
		if (pString[nIndex] == '\\' && pString[nIndex + 1] == '\'') //Escape String
		{
			p->str.szStrNodeName[nTgtIndex] = pString[nIndex + 1];
			nIndex += 2;
			nTgtIndex ++;
		}
		else
		{
			p->str.szStrNodeName[nTgtIndex] = pString[nIndex];
			nIndex ++;
			nTgtIndex ++;
		}
	}
	
	if(nTgtIndex != 0)
	{
		p->str.szStrNodeName[nTgtIndex - 1] = '\0'; //omit last '
	}
	else
	{
		p->str.szStrNodeName[0] = '\0';
	}
		
	Safe_CopyString(p->str.szStrNodeName, pString, sizeof(p->str.szStrNodeName));
	return p;
}

ExprNode *CreateOperNode(int oper, int nops, ...)
{ 
	va_list ap;
	ExprNode *p = NULL;
	size_t nodeSize;
	int i;

	/* allocate node */ 
	nodeSize = SIZEOF_NODETYPE + sizeof(oprNodeType) + 
	(nops - 1) * sizeof(ExprNode*);

	if ((p = (ExprNode *)malloc(nodeSize)) == NULL) 
		yyerror("out of memory");

	/* copy information */ 
	p->type = typeOpr;
	p->opr.oper = oper;
	p->opr.nops = nops;

	va_start(ap, nops);
	for (i = 0;i < nops;i++) 
	p->opr.op[i] = va_arg(ap, ExprNode*);
	va_end(ap);

	return p;
} 

void yyerror(char *s) 
{ 
	//fprintf(stdout, "%s\n", s);
	g_pCurExprCalc->SetErrMsg(s);
} 



