/*第一段Lex程序：C和Lex的全局声明*/
%{

	#ifndef _WIN32
	#include <unistd.h>
	#else//_WIN32
	#define YY_NO_UNISTD_H
	#include "io.h"
	#define isatty _isatty
	#endif
	#include <errno.h>
	
	#include <stdlib.h> 
    #include "exprcalc/ExprCalc.h"

	#include "parser.h" 

	void yyerror(char *);

#define YY_NEVER_INTERACTIVE 1

extern CExprCalc *g_pCurExprCalc;
extern int eval_getinput(char *buf, int maxlen);
extern void Safe_CopyString(char *szDest, const char *szSrc, int nDestLen);
#undef YY_INPUT
#define YY_INPUT(buf, retval, maxlen) (retval = g_pCurExprCalc->eval_getinput(buf, maxlen))

%}

NUMBER	([0-9]+|[0-9]+\.[0-9]+)
VARIABLE ([a-zA-Z_\x80-\xff\$][0-9a-zA-Z_\x80-\xff\.]*)
/*VARIABLE (([a-zA-Z_\x80-\xff][a-zA-Z_|\x80-\xff|0-9]*)[\.]){2,3}([a-zA-Z_|\x80-\xff][a-zA-Z0-9_|\x80-\xff]*)|(\$[a-zA-Z_|\x80-\xff][a-zA-Z_|\x80-\xff|0-9]*)*/
STRING \'(((\\\')|[^\'.])*)\'
/*VARIABLE ([A-H])*/


%%	/*第一段和第二段Lex程序分界*/

[ \t\n]+		/*忽略空白*/

("&&")	{/*逻辑与*/
			/*printf("Lex:This is operator AND\n");*/
			return AND; 
		} 
AND		{/*逻辑与*/ 
			/*printf("Lex:This is operator AND\n");*/
			return AND; 
		}	
(\|\|) 	{/*逻辑或*/ 
			/*printf("Lex:This is operator OR\n");*/
			return OR;
		} 
(OR) 	{/*逻辑或*/ 
			/*printf("Lex:This is operator OR\n");*/
			return OR;  
		} 
(XOR)	{/*逻辑异或*/ 
			/*printf("Lex:This is operator XOR\n");*/
			return XOR; 
		} 
(NOT)	{/*逻辑非*/ 
			/*printf("Lex:This is operator NOT\n");*/
			return NOT; 
		} 
("!")		{/*逻辑非*/ 
			/*printf("Lex:This is operator NOT\n");*/
			return NOT; 
		} 
(<=)	{/*小于等于*/
			/*printf("Lex:This is operator <=\n");*/
			return LE_EQ;
		} 
(>=)	{/*大于等于*/
			/*printf("Lex:This is operator >=\n");*/
			return GR_EQ;
		} 
(<>)	{/*不等于*/
			/*printf("Lex:This is operator <>\n");*/
			return NEQ;  
		} 
(!=)	{/*不等于*/
			/*printf("Lex:This is operator !=\n");*/
			return NEQ;  
		} 
(==)	{/*等于*/
			/*printf("Lex:This is operator ==\n");*/
			return EQ;
		}
(=)		{/*等于*/
			/*printf("Lex:This is operator =\n");*/
			return EQ;
		}
(>>)	{/*右移位操作*/
			/*printf("Lex:This is operator >>\n");*/
			return SHIFT_R;
		}
(<<)	{/*左移位操作*/
			/*printf("Lex:This is operator >>\n");*/
			return SHIFT_L;
		}

([sS][iI][nN])	{
				/*printf("Lex:This is Function sin\n");*/
				return SIN;
			}
([cC][oO][sS])		{
				/*printf("Lex:This is Function cos\n");*/
				return COS;
			}
([tT][aA][nN])		{
				/*printf("Lex:This is Function tan\n");*/
				return TAN;
			}
([aA][sS][iI][nN])		{
				/*printf("Lex:This is Function asin\n");*/
				return ASIN;
			}
([aA][cC][oO][sS])		{
				/*printf("Lex:This is Function acos\n");*/
				return ACOS;
			}
([aA][tT][aA][nN])		{
				/*printf("Lex:This is Function atan\n");*/
				return ATAN;
			}
([pP][oO][wW])		{
				/*printf("Lex:This is Function pow\n");*/
				return POW_FUNC;
			}
([lL][nN])		{
				/*printf("Lex:This is Function ln\n");*/
				return LOG;
			}
([lL][oO][gG])	{
				/*printf("Lex:This is Function log\n");*/
				return LOG10;
			}
([iI][nN][tT])		{
				/*printf("Lex:This is Function int\n");*/
				return GETINT;
			}
([bB][oO][oO][lL])		{
				/*printf("Lex:This is Function bool\n");*/
				return GETBOOL;
			}
([aA][bB][sS])		{
				/*printf("Lex:This is Function abs\n");*/
				return ABS;
			}
([sS][qQ][rR][tT])		{
				/*printf("Lex:This is Function sqrt\n");*/
				return SQRT;
			}	
([eE][xX][pP])		{
				/*printf("Lex:This is Function sin\n");*/
				return EXP;
			}
([fF][mM][oO][dD])		{
				/*printf("Lex:This is Function fmod\n");*/
				return FMOD;
			}
[-()<>+*/%?:,~&|^]	{ 
				return *yytext;
			} 	
{NUMBER}	{
				yylval.fValue = atof(yytext); 
				/*printf("Lex:This is NUMBER %s\n", yytext);*/
				return NUMBER;
			}
{VARIABLE}	{ 
				if (strlen(yytext) > sizeof(yylval.szVarName) - 1 )
				{
					yyerror("变量名超长");
				}

				Safe_CopyString(yylval.szVarName, yytext, sizeof(yylval.szVarName));
				/*printf("Lex:This is VARIABLE %s\n", yytext);*/
				return VARIABLE;	
			}
{STRING}	{ 
				if (strlen(yytext) > sizeof(yylval.szVarName) - 2 )
				{
					yyerror("字符串超长");
				}
				Safe_CopyString(yylval.szStrNodeName, yytext, sizeof(yylval.szStrNodeName));
				/*printf("Lex:This is STRING %s\n", yytext);*/
				return STRING;	
			}

.			{
				char szMsg[256];
				if (strlen(yytext) < sizeof(szMsg) - 13)
				{
					sprintf(szMsg, "无效字符: \"%s\"", yytext);
				}
				yyerror(szMsg);
			}

%%

int yywrap(void)
{
    return 1;
}

