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
#define S_FALSE				0

#define SLIP_RUNNING		0
#define SLIP_SHUTDOWN		1
#define SLIP_ERROR			2

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
	eType_COMPOUND_PROC,

	eType_MAX_TYPES

} eSlipObjectType;

typedef struct udtSlipEnvironment uSlipEnvironment, *pSlipEnvironment;
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

		struct
		{
			pSlipObject code;
			pSlipObject params;
			pSlipEnvironment env;
		} comp_proc;

	} data;
};

typedef struct udtSlipValue
{
	pSlipObject var;
	pSlipObject val;
} uSlipValue, *pSlipValue;

struct udtSlipEnvironment
{
	DList *lstVars;
	pSlipEnvironment parent;
};


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
	pSlipObject singleton_Nil;
	pSlipObject singleton_Lambda;

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

extern pSlipObject s_NewObject(pSlip gd);
extern pSlipObject s_NewInteger(pSlip gd, int64_t value);
extern pSlipObject s_NewCharacter(pSlip gd, int64_t value);
extern pSlipObject s_NewString(pSlip gd, uint8_t *data, int length);
extern pSlipObject s_NewSymbol(pSlip gd, uint8_t *data);
extern pSlipObject s_NewBool(pSlip gd, int value);
extern pSlipObject s_NewCompoundProc(pSlip gd, pSlipObject params, pSlipObject code, pSlipEnvironment env);

extern int sIsObject_EmptyList(pSlip gd, pSlipObject obj);
extern int sIsObject_String(pSlipObject obj);
extern int sIsObject_Character(pSlipObject obj);
extern int sIsObject_Pair(pSlipObject obj);
extern int sIsObject_Boolean(pSlipObject obj);
extern int sIsObject_Symbol(pSlipObject obj);
extern int sIsObject_Integer(pSlipObject obj);
extern int sIsObject_CompoundProc(pSlipObject obj);

extern void throw_error(pSlip gd, char *s, ...);
extern void slip_add_procedure(pSlip gd, pSlipEnvironment env, char *sym, pSlipObject (*func)(pSlip gd, pSlipObject args));

extern pSlipObject cons(pSlip gd, pSlipObject car, pSlipObject cdr);
extern pSlipObject car(pSlipObject pair);
extern pSlipObject cdr(pSlipObject pair);
extern void set_car(pSlipObject obj, pSlipObject value);
extern void set_cdr(pSlipObject obj, pSlipObject value);

extern pSlipObject slip_evaluate(pSlip gd, pSlipObject exp);
extern void slip_write(pSlip gd, pSlipObject obj);
extern pSlipObject slip_read(pSlip gd);

// setup + release of the interpreter data structure
extern pSlip slip_init(void);
extern void slip_release(pSlip gd);
extern void slip_reset_parser(pSlip ctx);

extern pToken re2c_NewToken(pSlip ctx, char *z, int len, int id);

#ifdef __cplusplus
}
#endif
#endif // SLIP_H

