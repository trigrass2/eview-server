
/*  A Bison parser, made from syntex.y with Bison version GNU Bison version 1.24
  */

#define YYBISON 1  /* Identify Bison output.  */

#define	NUMBER	258
#define	VARIABLE	259
#define	STRING	260
#define	QC	261
#define	OR	262
#define	AND	263
#define	XOR	264
#define	NEQ	265
#define	EQ	266
#define	LE_EQ	267
#define	GR_EQ	268
#define	SHIFT_L	269
#define	SHIFT_R	270
#define	UMINUS	271
#define	NOT	272
#define	SIN	273
#define	COS	274
#define	TAN	275
#define	LOG	276
#define	ASIN	277
#define	ACOS	278
#define	ATAN	279
#define	LOG10	280
#define	GETINT	281
#define	GETBOOL	282
#define	ABS	283
#define	SQRT	284
#define	EXP	285
#define	POW_FUNC	286
#define	FMOD	287

#line 1 "syntex.y"

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

#line 22 "syntex.y"
typedef union {
	double	fValue;		/* float value */ 
	char		szVarName[PK_LONGOBJTAGNAME_MAXLEN];		/* virable name */ 
	char		szStrNodeName[PK_LONGOBJTAGNAME_MAXLEN + 2];		/* string, +'2' for ' begin and after the string */ 
	ExprNode *pExprNode;		/* node pointer */ 
} YYSTYPE;

#ifndef YYLTYPE
typedef
  struct yyltype
    {
      int timestamp;
      int first_line;
      int first_column;
      int last_line;
      int last_column;
      char *text;
   }
  yyltype;

#define YYLTYPE yyltype
#endif

#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		122
#define	YYFLAG		-32768
#define	YYNTBASE	49

#define YYTRANSLATE(x) ((unsigned)(x) <= 287 ? yytranslate[x] : 51)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,    27,    14,     2,    46,
    47,    25,    23,    48,    24,     2,    26,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     8,     2,    17,
     2,    18,     7,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,    13,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,    12,     2,    30,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     2,     3,     4,     5,
     6,     9,    10,    11,    15,    16,    19,    20,    21,    22,
    28,    29,    31,    32,    33,    34,    35,    36,    37,    38,
    39,    40,    41,    42,    43,    44,    45
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     2,     4,     6,     8,    14,    18,    21,    24,    28,
    32,    36,    40,    44,    48,    52,    56,    60,    64,    68,
    72,    76,    80,    84,    88,    93,    98,   103,   108,   113,
   118,   123,   128,   133,   138,   143,   148,   153,   160,   167,
   171,   175,   179
};

static const short yyrhs[] = {    50,
     0,     3,     0,     4,     0,     5,     0,    50,     7,    50,
     8,    50,     0,    46,    50,    47,     0,    24,    50,     0,
    30,    50,     0,    50,    14,    50,     0,    50,    12,    50,
     0,    50,    13,    50,     0,    50,    17,    50,     0,    50,
    18,    50,     0,    50,    19,    50,     0,    50,    20,    50,
     0,    50,    15,    50,     0,    50,    16,    50,     0,    50,
    23,    50,     0,    50,    24,    50,     0,    50,    25,    50,
     0,    50,    26,    50,     0,    50,    27,    50,     0,    50,
    22,    50,     0,    50,    21,    50,     0,    31,    46,    50,
    47,     0,    32,    46,    50,    47,     0,    33,    46,    50,
    47,     0,    34,    46,    50,    47,     0,    35,    46,    50,
    47,     0,    36,    46,    50,    47,     0,    37,    46,    50,
    47,     0,    38,    46,    50,    47,     0,    39,    46,    50,
    47,     0,    40,    46,    50,    47,     0,    41,    46,    50,
    47,     0,    42,    46,    50,    47,     0,    43,    46,    50,
    47,     0,    45,    46,    50,    48,    50,    47,     0,    44,
    46,    50,    48,    50,    47,     0,    50,    10,    50,     0,
    50,     9,    50,     0,    50,    11,    50,     0,    29,    50,
     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
    55,    57,    61,    65,    69,    74,    78,    82,    86,    90,
    94,    98,   103,   107,   111,   115,   119,   123,   127,   131,
   135,   139,   143,   147,   152,   156,   160,   164,   168,   172,
   176,   180,   184,   188,   192,   196,   200,   204,   208,   212,
   216,   220,   224
};

static const char * const yytname[] = {   "$","error","$undefined.","NUMBER",
"VARIABLE","STRING","QC","'?'","':'","OR","AND","XOR","'|'","'^'","'&'","NEQ",
"EQ","'<'","'>'","LE_EQ","GR_EQ","SHIFT_L","SHIFT_R","'+'","'-'","'*'","'/'",
"'%'","UMINUS","NOT","'~'","SIN","COS","TAN","LOG","ASIN","ACOS","ATAN","LOG10",
"GETINT","GETBOOL","ABS","SQRT","EXP","POW_FUNC","FMOD","'('","')'","','","stat",
"expr",""
};
#endif

static const short yyr1[] = {     0,
    49,    50,    50,    50,    50,    50,    50,    50,    50,    50,
    50,    50,    50,    50,    50,    50,    50,    50,    50,    50,
    50,    50,    50,    50,    50,    50,    50,    50,    50,    50,
    50,    50,    50,    50,    50,    50,    50,    50,    50,    50,
    50,    50,    50
};

static const short yyr2[] = {     0,
     1,     1,     1,     1,     5,     3,     2,     2,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     4,     4,     4,     4,     4,     4,
     4,     4,     4,     4,     4,     4,     4,     6,     6,     3,
     3,     3,     2
};

static const short yydefact[] = {     0,
     2,     3,     4,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     1,     7,    43,     8,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     6,     0,    41,
    40,    42,    10,    11,     9,    16,    17,    12,    13,    14,
    15,    24,    23,    18,    19,    20,    21,    22,    25,    26,
    27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
    37,     0,     0,     0,     0,     0,     5,    39,    38,     0,
     0,     0
};

static const short yydefgoto[] = {   120,
    23
};

static const short yypact[] = {    56,
-32768,-32768,-32768,    56,    56,    56,   -24,    -8,    35,    36,
    37,    38,    66,   143,   182,   183,   219,   220,   221,   222,
   257,    56,   831,    -6,-32768,-32768,    56,    56,    56,    56,
    56,    56,    56,    56,    56,    56,    56,    56,    56,    56,
    56,   184,    56,    56,    56,    56,    56,    56,    56,    56,
    56,    56,    56,    56,    56,    56,    56,    56,    56,    56,
    56,    56,   223,   262,   301,   340,   379,   418,   457,   496,
   535,   574,   613,   652,   691,   104,   144,-32768,   810,    52,
   161,   200,   -10,   237,   275,   312,   312,   319,   319,   319,
   319,    80,    80,    -6,    -6,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,    56,    56,    56,   730,   769,   123,-32768,-32768,   190,
   304,-32768
};

static const short yypgoto[] = {-32768,
    -4
};


#define	YYLAST		858


static const short yytable[] = {    24,
    25,    26,    48,    49,    50,    51,    52,    53,    54,    55,
    56,    57,    58,    59,    60,    61,    62,    42,    60,    61,
    62,    27,    63,    64,    65,    66,    67,    68,    69,    70,
    71,    72,    73,    74,    75,    76,    77,    28,    79,    80,
    81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
    91,    92,    93,    94,    95,    96,    97,    98,     1,     2,
     3,    45,    46,    47,    48,    49,    50,    51,    52,    53,
    54,    55,    56,    57,    58,    59,    60,    61,    62,     4,
    29,    30,    31,    32,     5,     6,     7,     8,     9,    10,
    11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
    21,    22,    58,    59,    60,    61,    62,   115,   116,   117,
    43,    33,    44,    45,    46,    47,    48,    49,    50,    51,
    52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
    62,    44,    45,    46,    47,    48,    49,    50,    51,    52,
    53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
    43,   112,    44,    45,    46,    47,    48,    49,    50,    51,
    52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
    62,    46,    47,    48,    49,    50,    51,    52,    53,    54,
    55,    56,    57,    58,    59,    60,    61,    62,    34,   121,
    43,   113,    44,    45,    46,    47,    48,    49,    50,    51,
    52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
    62,    47,    48,    49,    50,    51,    52,    53,    54,    55,
    56,    57,    58,    59,    60,    61,    62,    35,    36,    43,
    78,    44,    45,    46,    47,    48,    49,    50,    51,    52,
    53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
    49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
    59,    60,    61,    62,    37,    38,    39,    40,    43,    99,
    44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
    54,    55,    56,    57,    58,    59,    60,    61,    62,    50,
    51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
    61,    62,    41,   122,     0,     0,     0,    43,   100,    44,
    45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
    55,    56,    57,    58,    59,    60,    61,    62,    52,    53,
    54,    55,    56,    57,    58,    59,    60,    61,    62,    56,
    57,    58,    59,    60,    61,    62,    43,   101,    44,    45,
    46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
    56,    57,    58,    59,    60,    61,    62,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,    43,   102,    44,    45,    46,
    47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
    57,    58,    59,    60,    61,    62,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,    43,   103,    44,    45,    46,    47,
    48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
    58,    59,    60,    61,    62,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,    43,   104,    44,    45,    46,    47,    48,
    49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
    59,    60,    61,    62,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,    43,   105,    44,    45,    46,    47,    48,    49,
    50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
    60,    61,    62,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,    43,   106,    44,    45,    46,    47,    48,    49,    50,
    51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
    61,    62,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    43,   107,    44,    45,    46,    47,    48,    49,    50,    51,
    52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
    62,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,    43,
   108,    44,    45,    46,    47,    48,    49,    50,    51,    52,
    53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,    43,   109,
    44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
    54,    55,    56,    57,    58,    59,    60,    61,    62,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,    43,   110,    44,
    45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
    55,    56,    57,    58,    59,    60,    61,    62,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,    43,   111,    44,    45,
    46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
    56,    57,    58,    59,    60,    61,    62,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,    43,   118,    44,    45,    46,
    47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
    57,    58,    59,    60,    61,    62,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,   119,    43,   114,    44,    45,
    46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
    56,    57,    58,    59,    60,    61,    62,    43,     0,    44,
    45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
    55,    56,    57,    58,    59,    60,    61,    62
};

static const short yycheck[] = {     4,
     5,     6,    13,    14,    15,    16,    17,    18,    19,    20,
    21,    22,    23,    24,    25,    26,    27,    22,    25,    26,
    27,    46,    27,    28,    29,    30,    31,    32,    33,    34,
    35,    36,    37,    38,    39,    40,    41,    46,    43,    44,
    45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
    55,    56,    57,    58,    59,    60,    61,    62,     3,     4,
     5,    10,    11,    12,    13,    14,    15,    16,    17,    18,
    19,    20,    21,    22,    23,    24,    25,    26,    27,    24,
    46,    46,    46,    46,    29,    30,    31,    32,    33,    34,
    35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
    45,    46,    23,    24,    25,    26,    27,   112,   113,   114,
     7,    46,     9,    10,    11,    12,    13,    14,    15,    16,
    17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
    27,     9,    10,    11,    12,    13,    14,    15,    16,    17,
    18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
     7,    48,     9,    10,    11,    12,    13,    14,    15,    16,
    17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
    27,    11,    12,    13,    14,    15,    16,    17,    18,    19,
    20,    21,    22,    23,    24,    25,    26,    27,    46,     0,
     7,    48,     9,    10,    11,    12,    13,    14,    15,    16,
    17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
    27,    12,    13,    14,    15,    16,    17,    18,    19,    20,
    21,    22,    23,    24,    25,    26,    27,    46,    46,     7,
    47,     9,    10,    11,    12,    13,    14,    15,    16,    17,
    18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
    14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
    24,    25,    26,    27,    46,    46,    46,    46,     7,    47,
     9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
    19,    20,    21,    22,    23,    24,    25,    26,    27,    15,
    16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
    26,    27,    46,     0,    -1,    -1,    -1,     7,    47,     9,
    10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
    20,    21,    22,    23,    24,    25,    26,    27,    17,    18,
    19,    20,    21,    22,    23,    24,    25,    26,    27,    21,
    22,    23,    24,    25,    26,    27,     7,    47,     9,    10,
    11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
    21,    22,    23,    24,    25,    26,    27,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,     7,    47,     9,    10,    11,
    12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
    22,    23,    24,    25,    26,    27,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,     7,    47,     9,    10,    11,    12,
    13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
    23,    24,    25,    26,    27,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,     7,    47,     9,    10,    11,    12,    13,
    14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
    24,    25,    26,    27,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,     7,    47,     9,    10,    11,    12,    13,    14,
    15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
    25,    26,    27,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,     7,    47,     9,    10,    11,    12,    13,    14,    15,
    16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
    26,    27,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     7,    47,     9,    10,    11,    12,    13,    14,    15,    16,
    17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
    27,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     7,
    47,     9,    10,    11,    12,    13,    14,    15,    16,    17,
    18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     7,    47,
     9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
    19,    20,    21,    22,    23,    24,    25,    26,    27,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,     7,    47,     9,
    10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
    20,    21,    22,    23,    24,    25,    26,    27,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,     7,    47,     9,    10,
    11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
    21,    22,    23,    24,    25,    26,    27,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,     7,    47,     9,    10,    11,
    12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
    22,    23,    24,    25,    26,    27,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    47,     7,     8,     9,    10,
    11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
    21,    22,    23,    24,    25,    26,    27,     7,    -1,     9,
    10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
    20,    21,    22,    23,    24,    25,    26,    27
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "bison.simple"

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

#ifndef alloca
#ifdef __GNUC__
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi)
#include <alloca.h>
#else /* not sparc */
#if defined (MSDOS) && !defined (__TURBOC__)
#include <malloc.h>
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
#include <malloc.h>
 #pragma alloca
#else /* not MSDOS, __TURBOC__, or _AIX */
#ifdef __hpux
#ifdef __cplusplus
extern "C" {
void *alloca (unsigned int);
};
#else /* not __cplusplus */
void *alloca ();
#endif /* not __cplusplus */
#endif /* __hpux */
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc.  */
#endif /* not GNU C.  */
#endif /* alloca not defined.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	return(0)
#define YYABORT 	return(1)
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, &yylloc, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval, &yylloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
int yyparse (void);
#endif

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_memcpy(FROM,TO,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (from, to, count)
     char *from;
     char *to;
     int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (char *from, char *to, int count)
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 192 "bison.simple"

/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
#else
#define YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#endif

int
yyparse(YYPARSE_PARAM)
     YYPARSE_PARAM_DECL
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
      yyss = (short *) alloca (yystacksize * sizeof (*yyssp));
      __yy_memcpy ((char *)yyss1, (char *)yyss, size * sizeof (*yyssp));
      yyvs = (YYSTYPE *) alloca (yystacksize * sizeof (*yyvsp));
      __yy_memcpy ((char *)yyvs1, (char *)yyvs, size * sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) alloca (yystacksize * sizeof (*yylsp));
      __yy_memcpy ((char *)yyls1, (char *)yyls, size * sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 1:
#line 55 "syntex.y"
{ g_pCurExprCalc->SaveRootNode(yyvsp[0].pExprNode); ;
    break;}
case 2:
#line 57 "syntex.y"
{ 
					//printf("Syntex:NUMBER %f\n", $1); 
					yyval.pExprNode = CreateConstNode(yyvsp[0].fValue);
				;
    break;}
case 3:
#line 61 "syntex.y"
{ 
								//printf("Syntex:VARIABLE %s\n", $1); 
								yyval.pExprNode = CreateVariNode(yyvsp[0].szVarName);
							;
    break;}
case 4:
#line 65 "syntex.y"
{
								//printf("Syntex: STRING %s\n", $1);
								yyval.pExprNode = CreateStrNode(yyvsp[0].szStrNodeName);
							;
    break;}
case 5:
#line 69 "syntex.y"
{ 
								//printf("Syntex:NOT \n");
								yyval.pExprNode = CreateOperNode(QC, 3, yyvsp[-4].pExprNode, yyvsp[-2].pExprNode, yyvsp[0].pExprNode);
							;
    break;}
case 6:
#line 74 "syntex.y"
{ 
								//printf("Syntex:() \n");  
								yyval.pExprNode = yyvsp[-1].pExprNode;
							;
    break;}
case 7:
#line 78 "syntex.y"
{ 
								//printf("Syntex: - \n");  
								yyval.pExprNode = CreateOperNode(UMINUS, 1, yyvsp[0].pExprNode);  
							;
    break;}
case 8:
#line 82 "syntex.y"
{ 
								//printf("Syntex: ~ (bit not) \n");  
								yyval.pExprNode = CreateOperNode('~', 1, yyvsp[0].pExprNode);  
							;
    break;}
case 9:
#line 86 "syntex.y"
{ 
								//printf("Syntex: & \n");
								yyval.pExprNode = CreateOperNode('&', 2, yyvsp[-2].pExprNode, yyvsp[0].pExprNode);  
							;
    break;}
case 10:
#line 90 "syntex.y"
{ 
								//printf("Syntex: | \n");
								yyval.pExprNode = CreateOperNode('|', 2, yyvsp[-2].pExprNode, yyvsp[0].pExprNode);  
							;
    break;}
case 11:
#line 94 "syntex.y"
{ 
								//printf("Syntex: ^ \n");
								yyval.pExprNode = CreateOperNode('^', 2, yyvsp[-2].pExprNode, yyvsp[0].pExprNode);  
							;
    break;}
case 12:
#line 98 "syntex.y"
{ 
								//printf("Syntex:<  \n");
								yyval.pExprNode = CreateOperNode('<', 2, yyvsp[-2].pExprNode, yyvsp[0].pExprNode);  
							;
    break;}
case 13:
#line 103 "syntex.y"
{ 
								//printf("Syntex:>  \n");  
								yyval.pExprNode = CreateOperNode('>', 2, yyvsp[-2].pExprNode, yyvsp[0].pExprNode);
							;
    break;}
case 14:
#line 107 "syntex.y"
{ 
								//printf("Syntex:<= \n"); 
								yyval.pExprNode = CreateOperNode(LE_EQ, 2, yyvsp[-2].pExprNode, yyvsp[0].pExprNode); 
							;
    break;}
case 15:
#line 111 "syntex.y"
{ 
								//printf("Syntex:>= \n"); 
								yyval.pExprNode = CreateOperNode(GR_EQ, 2, yyvsp[-2].pExprNode, yyvsp[0].pExprNode);
							;
    break;}
case 16:
#line 115 "syntex.y"
{ 
								//printf("Syntex:!= \n"); 
								yyval.pExprNode = CreateOperNode(NEQ, 2, yyvsp[-2].pExprNode, yyvsp[0].pExprNode);
							;
    break;}
case 17:
#line 119 "syntex.y"
{ 
								//printf("Syntex:== \n"); 
								yyval.pExprNode = CreateOperNode(EQ, 2, yyvsp[-2].pExprNode, yyvsp[0].pExprNode);
							;
    break;}
case 18:
#line 123 "syntex.y"
{ 
								//printf("Syntex:+  \n"); 
								yyval.pExprNode = CreateOperNode('+', 2, yyvsp[-2].pExprNode, yyvsp[0].pExprNode); 
							;
    break;}
case 19:
#line 127 "syntex.y"
{ 
								//printf("Syntex:-  \n"); 
								yyval.pExprNode = CreateOperNode('-', 2, yyvsp[-2].pExprNode, yyvsp[0].pExprNode);
							;
    break;}
case 20:
#line 131 "syntex.y"
{ 
								//printf("Syntex:*  \n"); 
								yyval.pExprNode = CreateOperNode('*', 2, yyvsp[-2].pExprNode, yyvsp[0].pExprNode); 
							;
    break;}
case 21:
#line 135 "syntex.y"
{ 
								//printf("Syntex:/  \n"); 
								yyval.pExprNode = CreateOperNode('/', 2, yyvsp[-2].pExprNode, yyvsp[0].pExprNode);
							;
    break;}
case 22:
#line 139 "syntex.y"
{ 
								//printf("Syntex:%  \n"); 
								yyval.pExprNode = CreateOperNode('%', 2, yyvsp[-2].pExprNode, yyvsp[0].pExprNode);
							;
    break;}
case 23:
#line 143 "syntex.y"
{ 
								//printf("Syntex:<< \n"); 
								yyval.pExprNode = CreateOperNode(SHIFT_R, 2, yyvsp[-2].pExprNode, yyvsp[0].pExprNode);
							;
    break;}
case 24:
#line 147 "syntex.y"
{ 
								//printf("Syntex:>> \n"); 
								yyval.pExprNode = CreateOperNode(SHIFT_L, 2, yyvsp[-2].pExprNode, yyvsp[0].pExprNode);
							;
    break;}
case 25:
#line 152 "syntex.y"
{
								//printf("Syntex:SIN  \n");
								yyval.pExprNode = CreateOperNode(SIN, 1, yyvsp[-1].pExprNode); 
							;
    break;}
case 26:
#line 156 "syntex.y"
{
								//printf("Syntex:COS  \n");
								yyval.pExprNode = CreateOperNode(COS, 1, yyvsp[-1].pExprNode); 
							;
    break;}
case 27:
#line 160 "syntex.y"
{
								//printf("Syntex:TAN  \n");
								yyval.pExprNode = CreateOperNode(TAN, 1, yyvsp[-1].pExprNode);
							;
    break;}
case 28:
#line 164 "syntex.y"
{
								//printf("Syntex:LOG  \n");
								yyval.pExprNode = CreateOperNode(LOG, 1, yyvsp[-1].pExprNode);
							;
    break;}
case 29:
#line 168 "syntex.y"
{
								//printf("Syntex:ASIN  \n");
								yyval.pExprNode = CreateOperNode(ASIN, 1, yyvsp[-1].pExprNode);
							;
    break;}
case 30:
#line 172 "syntex.y"
{
								//printf("Syntex:ACOS  \n");
								yyval.pExprNode = CreateOperNode(ACOS, 1, yyvsp[-1].pExprNode);
							;
    break;}
case 31:
#line 176 "syntex.y"
{
								//printf("Syntex:ATAN  \n");
								yyval.pExprNode = CreateOperNode(ATAN, 1, yyvsp[-1].pExprNode);
							;
    break;}
case 32:
#line 180 "syntex.y"
{
									//printf("Syntex:LOG10  \n");
									yyval.pExprNode = CreateOperNode(LOG10, 1, yyvsp[-1].pExprNode);
								;
    break;}
case 33:
#line 184 "syntex.y"
{
									//printf("Syntex:INT  \n");
									yyval.pExprNode = CreateOperNode(GETINT, 1, yyvsp[-1].pExprNode);
								;
    break;}
case 34:
#line 188 "syntex.y"
{
									//printf("Syntex:BOOL  \n");
									yyval.pExprNode = CreateOperNode(GETBOOL, 1, yyvsp[-1].pExprNode);
								;
    break;}
case 35:
#line 192 "syntex.y"
{
								//printf("Syntex:ABS  \n");
								yyval.pExprNode = CreateOperNode(ABS, 1, yyvsp[-1].pExprNode);
							;
    break;}
case 36:
#line 196 "syntex.y"
{
								//printf("Syntex:SQRT  \n");
								yyval.pExprNode = CreateOperNode(SQRT, 1, yyvsp[-1].pExprNode);
							;
    break;}
case 37:
#line 200 "syntex.y"
{
								//printf("Syntex:EXP  \n");
								yyval.pExprNode = CreateOperNode(EXP, 1, yyvsp[-1].pExprNode);
							;
    break;}
case 38:
#line 204 "syntex.y"
{ 
								//printf("Syntex:&& \n"); 
								yyval.pExprNode = CreateOperNode(FMOD, 2, yyvsp[-3].pExprNode, yyvsp[-1].pExprNode);
							;
    break;}
case 39:
#line 208 "syntex.y"
{ 
								//printf("Syntex:&& \n"); 
								yyval.pExprNode = CreateOperNode(POW_FUNC, 2, yyvsp[-3].pExprNode, yyvsp[-1].pExprNode);
							;
    break;}
case 40:
#line 212 "syntex.y"
{ 
								//printf("Syntex:&& \n"); 
								yyval.pExprNode = CreateOperNode(AND, 2, yyvsp[-2].pExprNode, yyvsp[0].pExprNode);
							;
    break;}
case 41:
#line 216 "syntex.y"
{ 
								//printf("Syntex:|| \n"); 
								yyval.pExprNode = CreateOperNode(OR, 2, yyvsp[-2].pExprNode, yyvsp[0].pExprNode);
							;
    break;}
case 42:
#line 220 "syntex.y"
{	
								//printf("Syntex:XOR \n"); 
								yyval.pExprNode = CreateOperNode(XOR, 2, yyvsp[-2].pExprNode, yyvsp[0].pExprNode);
							;
    break;}
case 43:
#line 224 "syntex.y"
{ 
								//printf("Syntex:NOT \n");
								yyval.pExprNode = CreateOperNode(NOT, 1, yyvsp[0].pExprNode);
							;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 487 "bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      free(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;
}
#line 229 "syntex.y"


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



