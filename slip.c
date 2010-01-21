/*
	Bootstrap slip.

	Ideas from bootstrap scheme from Peter Michaux (http://peter.michaux.ca/)
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>
#include <stdarg.h>
#include <assert.h>

#include "dlist.h"
#include "slip.h"
#include "slip_parser.h"
#include "slip_primitives.h"

static pSlipObject slip_eval(pSlip gd, pSlipObject exp, pSlipEnvironment env);

///////////////////////////////////////////////////////////////////////////////////////////

void FreeToken(void *t)
{
	pToken tt = t;

	//LogInfo("free token (%i)[%s]\n", tt->id, tt->z);
	if (tt == NULL)
		return;

	if (tt->z != NULL)
		free(tt->z);

	free(tt);
}

pToken re2c_NewToken(pSlip ctx, char *z, int len, int id)
{
	pToken x;

	assert(ctx != NULL);

	x = calloc(1, sizeof(uToken));

	x->id = id;
	x->z = calloc(1, len + 4);
	x->line = ctx->parse_data.current_line;

	memmove(x->z, z, len);

	return x;
}



///////////////////////////////////////////////////////////////////////////////////////////

static void FreeEnvironmentVariable(void *data)
{
	pSlipValue v = data;

	// dont free contents, juts container
	free(v);
}

static void FreeEnvironment(void *data)
{
	pSlipEnvironment env;

	env = data;
	FreeDList(env->lstVars);

	free(env);
}

static pSlipValue NewValue(void)
{
	pSlipValue v;

	v = calloc(1, sizeof(uSlipValue));
	return v;
}

static pSlipEnvironment NewEnvironment(void)
{
	pSlipEnvironment env;

	env = calloc(1, sizeof(uSlipEnvironment));
	env->lstVars = NewDList(FreeEnvironmentVariable);

	return env;
}


// gd can be null here, as we initialise the singleton TRUE/FALSE objects
pSlipObject s_NewObject(pSlip gd)
{
	pSlipObject o;

	o = calloc(1, sizeof(uSlipObject));

	if (o == NULL)
	{
		fflush(stdout);
		fflush(stdin);
		fprintf(stderr, "out of memory\n");
		exit(1);
	}

	o->id = gd->obj_id++;
	o->type = eType_NIL;

	assert(gd->lstObjects != NULL);
	dlist_ins(gd->lstObjects, o);

	return o;
}

static int s_IsSingleton(pSlip gd, pSlipObject obj)
{
	if (obj->id < USER_OBJECT_ID_START)
		return S_TRUE;

	return S_FALSE;
}

static void s_ReleaseObject(pSlip gd, pSlipObject obj)
{
	if (obj == NULL)
		return;

	assert(obj->type >= 1 && obj->type < eType_MAX_TYPES);

	// dont free up our base objects if context is in run mode.
	if (gd->running == SLIP_RUNNING && obj->id < USER_OBJECT_ID_START)
		return;


	switch (obj->type)
	{
		case eType_STRING:
			free(obj->data.string.data);
			break;

		case eType_SYMBOL:
			free(obj->data.symbol.value);
			break;

		case eType_PAIR:
		case eType_EMPTY_LIST:
		case eType_INTNUM:
		case eType_NIL:
		case eType_BOOL:
		case eType_CHARACTER:
		case eType_PRIMITIVE_PROC:
		case eType_COMPOUND_PROC:
			break;


		default:
			fflush(stdout);
			fflush(stdin);
			fprintf(stderr, "cannot free unknown type %i\n", obj->type);
			exit(1);
	}

	memset(obj, 0x0, sizeof(uSlipObject));
	free(obj);
}

pSlipObject s_NewInteger(pSlip gd, int64_t value)
{
	pSlipObject obj;

	obj = s_NewObject(gd);

	obj->type = eType_INTNUM;
	obj->data.intnum.value = value;

	return obj;
}

pSlipObject s_NewCharacter(pSlip gd, int64_t value)
{
	pSlipObject obj;

	obj = s_NewObject(gd);

	obj->type = eType_CHARACTER;
	obj->data.character.value = value;

	return obj;
}

pSlipObject s_NewString(pSlip gd, uint8_t *data, int length)
{
	pSlipObject obj;
	DLElement *e;

	e = dlist_head(gd->lstStrings);
	while (e != NULL)
	{
		obj = dlist_data(e);
		e = dlist_next(e);

		if (obj->data.string.length == length)
		{
			if ( memcmp(obj->data.string.data, data, length) == 0)
				return obj;
		}
	}

	obj = s_NewObject(gd);

	obj->type = eType_STRING;
	obj->data.string.data = malloc(length + 4);
	memmove(obj->data.string.data, data, 1+length);
	obj->data.string.length = length;

	dlist_ins(gd->lstStrings, obj);

	return obj;
}


pSlipObject s_NewSymbol(pSlip gd, uint8_t *data)
{
	pSlipObject obj;
	DLElement *e;

	e = dlist_head(gd->lstSymbols);
	while (e != NULL)
	{
		obj = dlist_data(e);
		e = dlist_next(e);

		if (strcmp(obj->data.symbol.value, data) == 0)
			return obj;
	}

	obj = s_NewObject(gd);

	obj->type = eType_SYMBOL;
	obj->data.symbol.value = strdup(data);

	dlist_ins(gd->lstSymbols, obj);

	return obj;
}

pSlipObject s_NewBool(pSlip gd, int value)
{
	pSlipObject obj;

	assert(value == S_TRUE || value == S_FALSE);

	obj = s_NewObject(gd);

	obj->type = eType_BOOL;
	obj->data.boolean.value = value;

	return obj;
}

pSlipObject s_NewCompoundProc(pSlip gd, pSlipObject params, pSlipObject code, pSlipEnvironment env)
{
	pSlipObject obj;

	obj = s_NewObject(gd);
	obj->type = eType_COMPOUND_PROC;
	obj->data.comp_proc.params = params;
	obj->data.comp_proc.code = code;
	obj->data.comp_proc.env = env;

	return obj;
}

static int IsObjectX(pSlipObject o, int type)
{
	if (o->type == type)
		return S_TRUE;
	else
		return S_FALSE;
}

int sIsObject_CompoundProc(pSlipObject obj)
{
	return IsObjectX(obj, eType_COMPOUND_PROC);
}

int sIsObject_EmptyList(pSlip gd, pSlipObject obj)
{
	return IsObjectX(obj, eType_EMPTY_LIST);
}

int sIsObject_String(pSlipObject obj)
{
	return IsObjectX(obj, eType_STRING);
}

int sIsObject_Character(pSlipObject obj)
{
	return IsObjectX(obj, eType_CHARACTER);
}

int sIsObject_Pair(pSlipObject obj)
{
	return IsObjectX(obj, eType_PAIR);
}

int sIsObject_Boolean(pSlipObject obj)
{
	return IsObjectX(obj, eType_BOOL);
}

int sIsObject_Symbol(pSlipObject obj)
{
	return IsObjectX(obj, eType_SYMBOL);
}

int sIsObject_Integer(pSlipObject obj)
{
	return IsObjectX(obj, eType_INTNUM);
}

pSlipObject s_NewPair(pSlip gd, pSlipObject car, pSlipObject cdr)
{
	pSlipObject obj;

	obj = s_NewObject(gd);
	obj->type = eType_PAIR;
	obj->data.pair.car = car;
	obj->data.pair.cdr = cdr;

	return obj;
}

pSlipObject cons(pSlip gd, pSlipObject car, pSlipObject cdr)
{
	return s_NewPair(gd, car, cdr);
}

pSlipObject car(pSlipObject pair)
{
	return pair->data.pair.car;
}

void set_car(pSlipObject obj, pSlipObject value)
{
	obj->data.pair.car = value;
}

pSlipObject cdr(pSlipObject pair)
{
	return pair->data.pair.cdr;
}

void set_cdr(pSlipObject obj, pSlipObject value)
{
	obj->data.pair.cdr = value;
}

static pSlipEnvironment enclosing_environment(pSlipEnvironment env)
{
	return env->parent;
}

static pSlipValue ScanForVar(pSlipObject o, pSlipEnvironment env)
{
	DLElement *e;
	pSlipValue v;

	e = dlist_head(env->lstVars);
	while (e != NULL)
	{
		v = dlist_data(e);
		e = dlist_next(e);

		if (v->var == o)
		{
			return v;
		}
	}

	return NULL;
}

static pSlipObject lookup_variable_value(pSlip gd, pSlipObject var, pSlipEnvironment env)
{
	pSlipValue v;

	do
	{
		v = ScanForVar(var, env);
		if (v != NULL)
			return v->val;

		env = enclosing_environment(env);
	}while (env != NULL);

	throw_error(gd, "unbound variable %s for lookup\n", (char*)var->data.symbol.value);
	return NULL;
}

static void set_variable_value(pSlip gd, pSlipObject var, pSlipObject val, pSlipEnvironment env)
{
	pSlipValue v;

	do
	{
		v = ScanForVar(var, env);
		if (v != NULL)
		{
			v->val = val;
			return;
		}

		env = enclosing_environment(env);
	}while (env != NULL);

	throw_error(gd, "unbound variable %s for assignment\n", (char*)var->data.symbol.value);
}

static pSlipObject make_primitive_proc(pSlip gd, pSlipObject (*func)(pSlip gd, pSlipObject args))
{
	pSlipObject obj;

	obj = s_NewObject(gd);
	obj->type = eType_PRIMITIVE_PROC;
	obj->data.prim_proc.func = func;

	return obj;
}

static void define_variable(pSlip gd, pSlipObject var, pSlipObject val, pSlipEnvironment env)
{
	pSlipValue v;

	v = ScanForVar(var, env);
	if (v != NULL)
	{
		v->val = val;
		return;
	}

	v = NewValue();
	v->var = var;
	v->val = val;
	dlist_ins(env->lstVars, v);
}

void slip_add_procedure(pSlip gd, pSlipEnvironment env, char *sym, pSlipObject (*func)(pSlip gd, pSlipObject args))
{
	define_variable(gd, s_NewSymbol(gd, sym), make_primitive_proc(gd, func), env);
}

static pSlipEnvironment setup_environment(pSlip gd, pSlipEnvironment parent, pSlipObject params, pSlipObject args)
{
	pSlipEnvironment env;

	env = NewEnvironment();
	dlist_ins(gd->lstGlobalEnvironment, env);

	env->parent = parent;

	// propagate args+params across time and space!

	while (args != gd->singleton_EmptyList && params != gd->singleton_EmptyList)
	{
		define_variable(gd, car(params), car(args), env);

		args = cdr(args);
		params = cdr(params);
	}

	return env;
}

///////////////////////////////////////////////////////////////////////////


static int is_self_evaluating(pSlipObject exp)
{
	return sIsObject_Boolean(exp) == S_TRUE || sIsObject_Integer(exp) == S_TRUE  || sIsObject_Character(exp) == S_TRUE || sIsObject_String(exp) == S_TRUE ;
}

static int is_tagged_list(pSlipObject expression, pSlipObject tag)
{
	pSlipObject the_car;

	if (sIsObject_Pair(expression) == S_TRUE)
	{
		the_car = car(expression);

		if (sIsObject_Symbol(the_car) == S_TRUE && (the_car == tag))
			return S_TRUE;
	}

	return S_FALSE;
}

static int is_quoted(pSlip gd, pSlipObject expression)
{
	return is_tagged_list(expression, gd->singleton_QuoteSymbol);
}

static int is_variable(pSlipObject expression)
{
	return sIsObject_Symbol(expression);
}

static pSlipObject text_of_quotation(pSlipObject exp)
{
	return cadr(exp);
}

static int is_assignment(pSlip gd, pSlipObject exp)
{
	return is_tagged_list(exp, gd->singleton_SetSymbol);
}

static pSlipObject assignment_variable(pSlipObject exp)
{
	return car(cdr(exp));
}

static pSlipObject assignment_value(pSlipObject exp)
{
	return car(cdr(cdr(exp)));
}

static char is_definition(pSlip gd, pSlipObject exp)
{
	return is_tagged_list(exp, gd->singleton_DefineSymbol);
}

static pSlipObject definition_variable(pSlipObject exp)
{
	if (sIsObject_Symbol(cadr(exp)) == S_TRUE)
	{
		return cadr(exp);
	}
	else
	{
		return caadr(exp);
	}
}

static pSlipObject make_lambda(pSlip gd, pSlipObject params, pSlipObject code)
{
	return cons(gd, gd->singleton_Lambda, cons(gd, params, code));
}

static int is_lambda(pSlip gd, pSlipObject exp)
{
	return is_tagged_list(exp, gd->singleton_Lambda);
}

static pSlipObject lambda_parameters(pSlipObject exp)
{
	return cadr(exp);
}

static pSlipObject lambda_body(pSlipObject exp)
{
	return cddr(exp);
}

static pSlipObject definition_value(pSlip gd, pSlipObject exp, pSlipEnvironment env)
{
	if (sIsObject_Symbol(cadr(exp)) == S_TRUE)
	{
		return caddr(exp);
	}
	else
	{
		return make_lambda(gd, cdadr(exp), cddr(exp));
	}
}

void throw_error(pSlip gd, char *s, ...)
{
	va_list args;
	char *x;

	x = calloc(1, 1024);

	va_start(args, s);
	vsnprintf(x, 1023, s, args);
	va_end(args);

	fflush(stdout);
	fflush(stdin);

	fprintf(stderr, "%s", x);
	fflush(stderr);
	free(x);

	gd->running = SLIP_ERROR;
}

static int is_last_exp(pSlip gd, pSlipObject seq)
{
	return sIsObject_EmptyList(gd, cdr(seq));
}

static pSlipObject first_exp(pSlipObject seq)
{
	return car(seq);
}

static pSlipObject rest_exps(pSlipObject seq)
{
	return cdr(seq);
}

static int sIsObject_PrimitiveProc(pSlipObject obj)
{
	if (obj->type == eType_PRIMITIVE_PROC)
		return S_TRUE;
	else
		return S_FALSE;
}

static pSlipObject make_begin(pSlip gd, pSlipObject exp)
{
    return cons(gd, gd->singleton_Begin, exp);
}

pSlipObject eval_assignment(pSlip gd, pSlipObject exp, pSlipEnvironment env)
{
	pSlipObject a1;
	pSlipObject z1;
	pSlipObject z2;

	z1 = assignment_value(exp);
	assert(z1 != NULL);

	a1 = slip_eval(gd, z1, env);
	assert(a1 != NULL);

	z2 = assignment_variable(exp);
	assert(z2 != NULL);

	set_variable_value(gd, z2, a1, env);

	if (gd->running == SLIP_RUNNING)
		return gd->singleton_OKSymbol;
	else
		return gd->singleton_False;
}

pSlipObject eval_definition(pSlip gd, pSlipObject exp, pSlipEnvironment env)
{
	pSlipObject a2;
	pSlipObject x1;
	pSlipObject x2;

	x1 = definition_value(gd, exp, env);
	assert(x1 != NULL);

	a2 = slip_eval(gd, x1, env);
	assert(a2 != NULL);

	x2 = definition_variable(exp);
	assert(x2 != NULL);

	define_variable(gd, x2, a2, env);

	if (gd->running == SLIP_RUNNING)
		return gd->singleton_OKSymbol;
	else
		return gd->singleton_False;
}

static int is_if(pSlip gd, pSlipObject expression)
{
	return is_tagged_list(expression, gd->singleton_IFSymbol);
}

static pSlipObject if_predicate(pSlipObject exp)
{
	return cadr(exp);
}

static pSlipObject if_consequent(pSlipObject exp)
{
	return caddr(exp);
}

static int is_true(pSlip gd, pSlipObject obj)
{
	// all values are true except for false itself.
	if (gd->singleton_False == obj)
		return S_FALSE;
	else
		return S_TRUE;
}

static pSlipObject if_alternative(pSlip gd, pSlipObject exp)
{
	if (sIsObject_EmptyList(gd, cdddr(exp)) == S_TRUE)
	{
		return gd->singleton_False;
	}
	else
	{
		return cadddr(exp);
	}
}

static int is_begin(pSlip gd, pSlipObject exp)
{
	return is_tagged_list(exp, gd->singleton_Begin);
}

static pSlipObject begin_actions(pSlipObject exp)
{
	return cdr(exp);
}

static int is_application(pSlipObject exp)
{
	return sIsObject_Pair(exp);
}

static pSlipObject slip_operator(pSlipObject exp)
{
	return car(exp);
}

static pSlipObject operands(pSlipObject exp)
{
	return cdr(exp);
}

int is_no_operands(pSlip gd, pSlipObject ops)
{
	return sIsObject_EmptyList(gd, ops);
}

static pSlipObject first_operand(pSlipObject ops)
{
	return car(ops);
}

static pSlipObject rest_operands(pSlipObject ops)
{
	return cdr(ops);
}

static pSlipObject list_of_values(pSlip gd, pSlipObject exps, pSlipEnvironment env)
{
	if (is_no_operands(gd, exps) == S_TRUE)
	{
		return gd->singleton_EmptyList;
	}
	else
	{
		pSlipObject x;

		x = slip_eval(gd, first_operand(exps), env);
		if (x == NULL)
			x = gd->singleton_Nil;

		return cons(gd, x, list_of_values(gd, rest_operands(exps), env));
	}
}

static pSlipObject slip_eval(pSlip gd, pSlipObject exp, pSlipEnvironment env)
{
	pSlipObject proc;
	pSlipObject args;

	tailcall:
	if (is_self_evaluating(exp) == S_TRUE)
	{
		return exp;
	}
	else if (is_variable(exp) == S_TRUE)
	{
		return lookup_variable_value(gd, exp, env);
	}
	else if (is_quoted(gd, exp) == S_TRUE)
	{
		return text_of_quotation(exp);
	}
	else if (is_assignment(gd, exp) == S_TRUE)
	{
		return eval_assignment(gd, exp, env);
	}
	else if (is_definition(gd, exp) == S_TRUE)
	{
		return eval_definition(gd, exp, env);
	}
	else if (is_if(gd, exp) == S_TRUE)
	{
		exp = is_true(gd, slip_eval(gd, if_predicate(exp), env)) == S_TRUE ? if_consequent(exp) : if_alternative(gd, exp);
		goto tailcall;
	}
	else if (is_lambda(gd, exp) == S_TRUE)
	{
		return s_NewCompoundProc(gd, lambda_parameters(exp), lambda_body(exp), env);
	}
	else if (is_begin(gd, exp) == S_TRUE)
	{
		exp = begin_actions(exp);
		while (!is_last_exp(gd, exp))
		{
			slip_eval(gd, first_exp(exp), env);
			exp = rest_exps(exp);
		}
		exp = first_exp(exp);
		goto tailcall;
	}
	else if (is_application(exp))
	{
		proc = slip_eval(gd, slip_operator(exp), env);
		if (proc == NULL)
			return gd->singleton_False;

		if (proc->type == eType_PRIMITIVE_PROC || proc->type == eType_COMPOUND_PROC)
		{
			args = list_of_values(gd, operands(exp), env);
			if (args == NULL)
				return gd->singleton_False;

			if (sIsObject_PrimitiveProc(proc) == S_TRUE)
			{
				return proc->data.prim_proc.func(gd, args);
			}
			else if (sIsObject_CompoundProc(proc) == S_TRUE)
			{
				env = setup_environment(gd, proc->data.comp_proc.env, proc->data.comp_proc.params, args);
				exp = make_begin(gd, proc->data.comp_proc.code);
				goto tailcall;
			}
			else
			{
				throw_error(gd, "unknown procedure type\n");
				return gd->singleton_False;
			}
		}
		else
			return proc;
	}
	else
	{
		throw_error(gd, "cannot eval unknown expression type\n");
		return NULL;
	}

	throw_error(gd, "what??\n");
	return NULL;
}

static pSlipEnvironment get_global_environment(pSlip gd)
{
	pSlipEnvironment env;
	DLElement *e;

	e = dlist_head(gd->lstGlobalEnvironment);
	env = dlist_data(e);

	return env;
}

pSlipObject slip_evaluate(pSlip gd, pSlipObject exp)
{
	pSlipEnvironment env;

	env = get_global_environment(gd);

	return slip_eval(gd, exp, env);
}

/////////////////////////////////////////////////////////////////////////

static void write_pair(pSlip gd, pSlipObject pair)
{
	pSlipObject car_obj;
	pSlipObject cdr_obj;

	car_obj = car(pair);
	cdr_obj = cdr(pair);

	slip_write(gd, car_obj);

	if (cdr_obj->type == eType_PAIR)
	{
		printf(" ");
		write_pair(gd, cdr_obj);
	}
	else if (cdr_obj->type == eType_EMPTY_LIST)
	{
		return;
	}
	else
	{
		printf(" . ");
		slip_write(gd, cdr_obj);
	}
}

void slip_write(pSlip gd, pSlipObject obj)
{
	if (gd->running != SLIP_RUNNING || obj == NULL)
		return;

	switch (obj->type)
	{
		case eType_PRIMITIVE_PROC:
		case eType_COMPOUND_PROC:
			printf("#<procedure> %p", obj->data.prim_proc.func);
			break;

		case eType_EMPTY_LIST:
			printf("()");
			break;

		case eType_PAIR:
			printf("(");
			write_pair(gd, obj);
			printf(")");
			break;

		case eType_SYMBOL:
			printf("%s", obj->data.symbol.value);
			break;

		case eType_INTNUM:
			printf("%"PRIi64, obj->data.intnum.value);
			break;

		case eType_NIL:
			printf("<NIL>");
			break;

		case eType_BOOL:
			printf("#%c", obj == gd->singleton_True ? 't' : 'f');
			break;

		case eType_STRING:
			{
				char *str;

				str = obj->data.string.data;
				printf("\"");
				while (*str != '\0')
				{
					switch (*str)
					{
						case '\n':
							printf("\\n");
							break;

						case '\\':
							printf("\\\\");
							break;

						case '"':
							printf("\\\"");
							break;

						default:
							printf("%c", *str);
							break;
					}
					str++;
				}
				printf("\"");
			}
			break;

		case eType_CHARACTER:
			printf("#\\");
			switch (obj->data.character.value)
			{
				case '\t':
					printf("tab");
					break;

				case '\n':
					printf("newline");
					break;

				case ' ':
					printf("space");
					break;

				default:
					printf("%c", obj->data.character.value);
					break;
			}
			break;

		default:
			throw_error(gd, "cannot eval unknown expression type\n");
			break;
	}
}

static int is_delimiter(pToken tok)
{
	if (tok == NULL)
		return S_TRUE;

	switch (tok->id)
	{
		case kOPAREN:
		case kCPAREN:
		case kSTRING:
		case kQUOTE:
		case kNEWLINE:
		case kCHAR:
		case kHEX_NUMBER:
		case kOCT_NUMBER:
		case kINT_NUMBER:
			return S_TRUE;
			break;
	}

	return S_FALSE;
}

static int is_initial(pToken tok)
{
	//return isalpha(c) || c == '*' || c == '/' || c == '>' || c == '<' || c == '=' || c == '?' || c == '!';

	if (tok == NULL)
		return S_FALSE;

	switch (tok->id)
	{
		case kID:
			return S_TRUE;
			break;
	}

	return S_FALSE;
}

static pToken read_input(pSlip gd)
{
	pToken t;

	if (gd->parse_data.eCurrentToken == NULL)
		return NULL;

	t = dlist_data(gd->parse_data.eCurrentToken);
	gd->parse_data.eCurrentToken = dlist_next(gd->parse_data.eCurrentToken);

	return t;
}

static pToken peek_input(pSlip gd)
{
	pToken t;

	if (gd->parse_data.eCurrentToken == NULL)
		return NULL;

	t = dlist_data(gd->parse_data.eCurrentToken);

	return t;
}

static void expect_delimiter(pSlip gd)
{
	if (!is_delimiter(peek_input(gd)))
	{
		throw_error(gd, "character not followed by delimiter\n");
	}
}

static pSlipObject read_pair(pSlip gd)
{
	pToken tok;
	pSlipObject car_obj;
	pSlipObject cdr_obj;

	if (gd->running != SLIP_RUNNING)
		return NULL;

	tok = peek_input(gd);
	if (tok->id == kCPAREN)
	{
		tok = read_input(gd);
		return gd->singleton_EmptyList;
	}

	car_obj = slip_read(gd);

	tok = peek_input(gd);
	if (tok == NULL)
	{
		throw_error(gd, "Unclosed list\n");
		return cons(gd, car_obj, gd->singleton_EmptyList);
	}
	else if (tok->id == kDOT)
	{
		tok = read_input(gd); // skip over dot

		cdr_obj = slip_read(gd);

		tok = read_input(gd);
		if (tok->id != kCPAREN)
		{
			throw_error(gd, "where was the trailing right paren?\n");
			return NULL;
		}

		return cons(gd, car_obj, cdr_obj);
	}
	else
	{
		cdr_obj = read_pair(gd);
		return cons(gd, car_obj, cdr_obj);
	}
}

pSlipObject slip_read(pSlip gd)
{
	pToken tok;
	pSlipObject obj;

	int buff_idx;
	uint8_t *buff;

	if (gd->running != SLIP_RUNNING)
		return NULL;

	while ((tok = read_input(gd)) != NULL && gd->running == SLIP_RUNNING)
	{
		switch (tok->id)
		{
			case kCHAR:
				{
					expect_delimiter(gd);
					return s_NewCharacter(gd, tok->z[2]);
				}
				break;

			case kTRUE:
				return gd->singleton_True;
				break;

			case kFALSE:
				return gd->singleton_False;
				break;

			case kCHAR_NEWLINE:
				return s_NewCharacter(gd, '\n');
				break;
			case kCHAR_TAB:
				return s_NewCharacter(gd, '\t');
			case kCHAR_SPACE:
				return s_NewCharacter(gd, ' ');
				break;

			case kINT_NUMBER:
				return s_NewInteger(gd, strtoll(tok->z, NULL, 10));
				break;
			case kOCT_NUMBER:
				return s_NewInteger(gd, strtoll(tok->z, NULL, 8));
				break;
			case kHEX_NUMBER:
				return s_NewInteger(gd, strtoll(tok->z, NULL, 16));
				break;

			case kID:
				if (strcmp(tok->z, "quit") == 0)
					gd->running = SLIP_SHUTDOWN;

				return s_NewSymbol(gd, tok->z);
				break;

			case kSTRING:
				{
					buff_idx = 0;
					buff = malloc(strlen(tok->z)  + 1);
					buff[0] = 0;

					char *p;

					p = tok->z;
					p++;

					while (*p != 0)
					{
						if (*p == '\\')
						{
							p++;
							switch (*p)
							{
								case 'n':
									buff[buff_idx++] = '\n';
									p++;
									break;
								case 'r':
									buff[buff_idx++] = '\r';
									p++;
									break;
								case 't':
									buff[buff_idx++] = '\t';
									p++;
									break;
								default:
									buff[buff_idx++] = *p++;
									break;
							}
						}
						else
						{
							buff[buff_idx++] = *p++;
						}

						buff[buff_idx] = 0;
					}

					buff[buff_idx-1] = 0;


					obj = s_NewString(gd, buff, buff_idx);

					free(buff);
					return obj;
				}
				break;

			case kOPAREN:
				return read_pair(gd);
				break;

			case kQUOTE:
				return cons(gd, gd->singleton_QuoteSymbol, cons(gd, slip_read(gd), gd->singleton_EmptyList));
				break;

			default:
				throw_error(gd, "bad input. Unexpected \"%s\"\n", tok->z);
				return NULL;

		}
	}

	//throw_error(gd, "read illegal state\n");
	gd->running = SLIP_SHUTDOWN;
	return gd->singleton_OKSymbol;
}

pSlip slip_init(void)
{
	pSlip s;
	pSlipEnvironment env;

	s = calloc(1, sizeof(uSlip));
	assert(s != NULL);

	s->lstObjects = NewDList(NULL);
	s->lstSymbols = NewDList(NULL);
	s->lstStrings = NewDList(NULL);
	s->lstGlobalEnvironment = NewDList(FreeEnvironment);
	env = setup_environment(s, NULL, s->singleton_EmptyList, s->singleton_EmptyList);

	s->singleton_False = s_NewBool(s, S_FALSE);
	s->singleton_True = s_NewBool(s, S_TRUE);
	s->singleton_EmptyList = s_NewObject(s);
	s->singleton_EmptyList->type = eType_EMPTY_LIST;

	s->singleton_QuoteSymbol = s_NewSymbol(s, "quote");
	s->singleton_DefineSymbol = s_NewSymbol(s, "define");
	s->singleton_OKSymbol = s_NewSymbol(s, "ok");
	s->singleton_SetSymbol = s_NewSymbol(s, "set!");
	s->singleton_IFSymbol = s_NewSymbol(s, "if");
	s->singleton_Nil = s_NewSymbol(s, "nil");
	define_variable(s, s->singleton_Nil, s_NewObject(s), env);

	s->singleton_Lambda = s_NewSymbol(s, "lambda");
	s->singleton_Begin = s_NewSymbol(s, "begin");

	s->obj_id = USER_OBJECT_ID_START;
	s->running = SLIP_RUNNING;

	s->parse_data.lstTokens = NewDList(FreeToken);

	slip_install_primitives(s, env);

	return s;
}

void slip_release(pSlip gd)
{
	DLElement *e;

	e = dlist_tail(gd->lstObjects);

	while (e != NULL)
	{
		pSlipObject obj;

		obj = dlist_data(e);
		e = dlist_prev(e);

		s_ReleaseObject(gd, obj);
	}

	FreeDList(gd->lstObjects);

	FreeDList(gd->lstSymbols);
	FreeDList(gd->lstStrings);
	FreeDList(gd->lstGlobalEnvironment);

	FreeDList(gd->parse_data.lstTokens);

	free(gd);
}

void slip_reset_parser(pSlip slip)
{
	dlist_empty(slip->parse_data.lstTokens);
	slip->parse_data.current_line = 0;
	slip->parse_data.eCurrentToken = NULL;
	slip->parse_data.comment_depth = 0;
}
