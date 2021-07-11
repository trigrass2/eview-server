typedef union {
	double	fValue;		/* float value */ 
	char		szVarName[PK_LONGOBJTAGNAME_MAXLEN];		/* virable name */ 
	char		szStrNodeName[PK_LONGOBJTAGNAME_MAXLEN + 2];		/* string, +'2' for ' begin and after the string */ 
	ExprNode *pExprNode;		/* node pointer */ 
} YYSTYPE;
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


extern YYSTYPE yylval;
