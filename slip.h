#ifndef SLIP_H
#define SLIP_H
#ifdef __cplusplus
extern "C"{
#endif

#define caar(obj)   car(car(obj))
#define cadr(obj)   car(cdr(obj))
#define cdar(obj)   cdr(car(obj))
#define cddr(obj)   cdr(cdr(obj))
#define caaar(obj)  car(car(car(obj)))
#define caadr(obj)  car(car(cdr(obj)))
#define cadar(obj)  car(cdr(car(obj)))
#define caddr(obj)  car(cdr(cdr(obj)))
#define cdaar(obj)  cdr(car(car(obj)))
#define cdadr(obj)  cdr(car(cdr(obj)))
#define cddar(obj)  cdr(cdr(car(obj)))
#define cdddr(obj)  cdr(cdr(cdr(obj)))
#define caaaar(obj) car(car(car(car(obj))))
#define caaadr(obj) car(car(car(cdr(obj))))
#define caadar(obj) car(car(cdr(car(obj))))
#define caaddr(obj) car(car(cdr(cdr(obj))))
#define cadaar(obj) car(cdr(car(car(obj))))
#define cadadr(obj) car(cdr(car(cdr(obj))))
#define caddar(obj) car(cdr(cdr(car(obj))))
#define cadddr(obj) car(cdr(cdr(cdr(obj))))
#define cdaaar(obj) cdr(car(car(car(obj))))
#define cdaadr(obj) cdr(car(car(cdr(obj))))
#define cdadar(obj) cdr(car(cdr(car(obj))))
#define cdaddr(obj) cdr(car(cdr(cdr(obj))))
#define cddaar(obj) cdr(cdr(car(car(obj))))
#define cddadr(obj) cdr(cdr(car(cdr(obj))))
#define cdddar(obj) cdr(cdr(cdr(car(obj))))
#define cddddr(obj) cdr(cdr(cdr(cdr(obj))))


#define S_TRUE				1
#define S_FALSE			0

#define SLIP_RUNNING		0
#define SLIP_SHUTDOWN		1
#define SLIP_ERROR		2

typedef struct udtToken
{
	int id;
	char *z;
	int line;
} uToken, *pToken;

typedef enum
{
	eType_NIL = 1,
	eType_INTNUM,
	eType_BOOL,
	eType_CHARACTER,
	eType_STRING,
	eType_EMPTY_LIST,
	eType_PAIR,
	eType_SYMBOL,
	eType_PRIMITIVE_PROC,

	eType_MAX_TYPES

} eSlipObjectType;

typedef struct udtSlipObject uSlipObject, *pSlipObject;
typedef struct udtSlip uSlip, *pSlip;

struct udtSlipObject
{
	uint32_t id;
	eSlipObjectType type;

	union
	{
		struct
		{
			int64_t value;
		} intnum;

		struct
		{
			int value;
		} boolean;

		struct
		{
			int value;
		} character;

		struct
		{
			int length;
			uint8_t *data;
		} string;

		struct
		{
			pSlipObject car;
			pSlipObject cdr;
		} pair;

		struct
		{
			char *value;
		} symbol;

		struct
		{
			pSlipObject (*func)(pSlip gd, pSlipObject args);
		} prim_proc;

	} data;
};

typedef struct udtSlipValue
{
	pSlipObject var;
	pSlipObject val;
} uSlipValue, *pSlipValue;

typedef struct udtSlipEnvironment
{
	DList *lstVars;
	DLElement *list_backtrack;
} uSlipEnvironment, *pSlipEnvironment;


#define USER_OBJECT_ID_START		32
struct udtSlip
{
	int         running;

	uint32_t    obj_id;		// object ID less than 32 are singleton objects

	pSlipObject singleton_True;
	pSlipObject singleton_False;
	pSlipObject singleton_EmptyList;

	pSlipObject singleton_QuoteSymbol;
	pSlipObject singleton_DefineSymbol;
	pSlipObject singleton_SetSymbol;
	pSlipObject singleton_OKSymbol;
	pSlipObject singleton_IFSymbol;

	DList       *lstSymbols;
	DList       *lstStrings;

	DList       *lstObjects;

	DList       *lstGlobalEnvironment;

	struct parser_data
	{
		void *pParser;

		DList *lstTokens;
		DLElement *eCurrentToken;

		int current_line;
		char *buff_start;
		int comment_depth;
	} parse_data;

};

extern pSlipObject slip_evaluate(pSlip gd, pSlipObject exp);
extern void slip_write(pSlip gd, pSlipObject obj);
extern pSlipObject slip_read(pSlip gd);

// setup + release of the interpreter data structure
extern pSlip slip_init(void);
extern void slip_release(pSlip gd);
extern void slip_reset_parser(pSlip ctx);

extern pToken re2c_NewToken(pSlip ctx, char *z, int len, int id);
extern void token_destructor(pSlip ctx, pToken t);



// from lemon
extern void slip_parser_Free(void *p, void(*freeProc)(void*));
extern void *slip_parser_Alloc(void *(*mallocProc)(size_t));

extern void slip_parser_(
						void *yyp,					/* The parser */
						int yymajor,				/* The major token code number */
						pToken yyminor,		  		/* The value for the token */
						pSlip ctx					/* Optional %extra_argument parameter */
						);


#ifdef __cplusplus
}
#endif
#endif // SLIP_H

