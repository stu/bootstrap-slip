#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#include "dlist.h"
#include "slip.h"
#include "slip_parser.h"
#include "slip_tokeniser.h"

static pSlipObject slip_primitive__add_proc(pSlip gd, pSlipObject args)
{
	int64_t result = 0;
	int count = 0;

	if (sIsObject_EmptyList(gd, args) == S_FALSE)
	{
		result = (car(args))->data.intnum.value;
		args = cdr(args);
	}

	while (sIsObject_EmptyList(gd, args) == S_FALSE)
	{
		if (car(args)->type != eType_INTNUM)
		{
			throw_error(gd, "Arithmatic on non numeric data\n");
			return s_NewInteger(gd, 0);
		}

		result += (car(args))->data.intnum.value;
		args = cdr(args);
	}

	return s_NewInteger(gd, result);
}

static pSlipObject slip_primitive__sub_proc(pSlip gd, pSlipObject args)
{
	int64_t result = 0;

	if (sIsObject_EmptyList(gd, args) == S_FALSE)
	{
		result = (car(args))->data.intnum.value;
		args = cdr(args);
	}

	while (sIsObject_EmptyList(gd, args) == S_FALSE)
	{
		if (car(args)->type != eType_INTNUM)
		{
			throw_error(gd, "Arithmatic on non numeric data\n");
			return s_NewInteger(gd, 0);
		}

		result -= (car(args))->data.intnum.value;
		args = cdr(args);
	}

	return s_NewInteger(gd, result);
}

static pSlipObject slip_primitive__mul_proc(pSlip gd, pSlipObject args)
{
	int64_t result = 1;

	if (sIsObject_EmptyList(gd, args) == S_FALSE)
	{
		result = (car(args))->data.intnum.value;
		args = cdr(args);
	}

	while (sIsObject_EmptyList(gd, args) == S_FALSE)
	{
		if (car(args)->type != eType_INTNUM)
		{
			throw_error(gd, "Arithmatic on non numeric data\n");
			return s_NewInteger(gd, 0);
		}

		result *= (car(args))->data.intnum.value;
		args = cdr(args);
	}

	return s_NewInteger(gd, result);
}


static pSlipObject slip_primitive__div_proc(pSlip gd, pSlipObject args)
{
	int64_t result = 0;

	if (sIsObject_EmptyList(gd, args) == S_FALSE)
	{
		result = (car(args))->data.intnum.value;
		args = cdr(args);
	}

	while (sIsObject_EmptyList(gd, args) == S_FALSE)
	{
		if (car(args)->type != eType_INTNUM)
		{
			throw_error(gd, "Arithmatic on non numeric data\n");
			return s_NewInteger(gd, 0);
		}

		if (car(args)->data.intnum.value == 0)
		{
			throw_error(gd, "Divide by zero\n");
			return s_NewInteger(gd, 0);
		}

		result /= (car(args))->data.intnum.value;
		args = cdr(args);
	}

	return s_NewInteger(gd, result);
}

static pSlipObject slip_primitive__convert_char_int_x(pSlip gd, pSlipObject args)
{
	if (args->type == eType_PAIR)
	{
		pSlipObject result = gd->singleton_EmptyList;

		if (car(args)->type == eType_PAIR)
			return slip_primitive__convert_char_int_x(gd, car(args));

		if (car(args)->type != eType_CHARACTER)
		{
			throw_error(gd, "Requested char->int conversion on non-char, type %i\n", car(args)->type);
			return s_NewInteger(gd, 0);
		}

		result = s_NewInteger(gd, car(args)->data.character.value);
		return cons(gd, result, slip_primitive__convert_char_int_x(gd, cdr(args)));

	}
	else if (args->type == eType_EMPTY_LIST)
	{
		return gd->singleton_EmptyList;
	}
	else if (args->type == eType_CHARACTER)
	{
		return s_NewInteger(gd, args->data.character.value);
	}
	else
	{
		throw_error(gd, "Requested char->int conversion on non-char\n");
		return s_NewInteger(gd, 0);
	}
}

static pSlipObject slip_primitive__convert_int_char_x(pSlip gd, pSlipObject args)
{
	if (args->type == eType_PAIR)
	{
		pSlipObject result = gd->singleton_EmptyList;

		if (car(args)->type == eType_PAIR)
			return slip_primitive__convert_int_char_x(gd, car(args));

		if (car(args)->type != eType_INTNUM)
		{
			throw_error(gd, "Requested int->char conversion on non-int\n");
			return s_NewInteger(gd, 0);
		}

		result = s_NewCharacter(gd, car(args)->data.character.value);
		return cons(gd, result, slip_primitive__convert_int_char_x(gd, cdr(args)));
	}
	else if (args->type == eType_EMPTY_LIST)
	{
		return gd->singleton_EmptyList;
	}
	else if (args->type == eType_INTNUM)
	{
		return s_NewCharacter(gd, args->data.character.value);
	}
	else
	{
		throw_error(gd, "Requested int->char conversion on non-int\n");
		return s_NewInteger(gd, 0);
	}
}

static pSlipObject slip_primitive__convert_num_string_x(pSlip gd, pSlipObject args)
{
	char buff[48];

	if (args->type == eType_PAIR)
	{
		pSlipObject result = gd->singleton_EmptyList;

		if (car(args)->type == eType_PAIR)
			return slip_primitive__convert_num_string_x(gd, car(args));

		if (car(args)->type != eType_INTNUM)
		{
			throw_error(gd, "Requested num->string conversion on non-numeric\n");
			return s_NewString(gd, "", 0);
		}

		sprintf(buff, "%"PRIi64, car(args)->data.intnum.value);
		result = s_NewString(gd, (uint8_t*)buff, strlen(buff));
		return cons(gd, result, slip_primitive__convert_num_string_x(gd, cdr(args)));
	}
	else if (args->type == eType_EMPTY_LIST)
	{
		return gd->singleton_EmptyList;
	}
	else if (args->type == eType_INTNUM)
	{
		sprintf(buff, "%"PRIi64, args->data.intnum.value);
		return s_NewString(gd, (uint8_t*)buff, strlen(buff));
	}
	else
	{
		throw_error(gd, "Requested num->string conversion on non-numeric\n");
		return s_NewString(gd, "", 0);
	}
}

static pSlipObject slip_primitive__convert_string_num_x(pSlip gd, pSlipObject args)
{
	if (args->type == eType_PAIR)
	{
		pSlipObject result = gd->singleton_EmptyList;

		if (car(args)->type == eType_PAIR)
			return slip_primitive__convert_string_num_x(gd, car(args));

		if (car(args)->type != eType_STRING)
		{
			throw_error(gd, "Requested string->num conversion on non-string\n");
			return s_NewInteger(gd, 0);
		}

		if (car(args)->data.string.data[0]=='-' && car(args)->data.string.data[1]=='0' && car(args)->data.string.data[2]=='x')
			result = s_NewInteger(gd, strtoll(car(args)->data.string.data, NULL, 16));
		else
			result = s_NewInteger(gd, strtoll(car(args)->data.string.data, NULL, 10));

		return cons(gd, result, slip_primitive__convert_string_num_x(gd, cdr(args)));
	}
	else if (args->type == eType_EMPTY_LIST)
	{
		return gd->singleton_EmptyList;
	}
	else if (args->type == eType_STRING)
	{
		if (args->data.string.data[0]=='-' && args->data.string.data[1]=='0' && args->data.string.data[2]=='x')
			return s_NewInteger(gd, strtoll(args->data.string.data, NULL, 16));
		else
			return s_NewInteger(gd, strtoll(args->data.string.data, NULL, 10));
	}
	else
	{
		throw_error(gd, "Requested string->num conversion on non-string\n");
		return s_NewInteger(gd, 0);
	}
}

static pSlipObject slip_primitive__convert_symbol_string_x(pSlip gd, pSlipObject args)
{
	if (args->type == eType_PAIR)
	{
		pSlipObject result = gd->singleton_EmptyList;

		if (car(args)->type == eType_PAIR)
			return slip_primitive__convert_symbol_string_x(gd, car(args));

		if (car(args)->type != eType_SYMBOL)
		{
			throw_error(gd, "Requested symbol->string conversion on non-symbol\n");
			return s_NewString(gd, "", 0);
		}

		result = s_NewString(gd, car(args)->data.symbol.value, strlen(car(args)->data.symbol.value));
		return cons(gd, result, slip_primitive__convert_symbol_string_x(gd, cdr(args)));
	}
	else if (args->type == eType_EMPTY_LIST)
	{
		return gd->singleton_EmptyList;
	}
	else if (args->type == eType_SYMBOL)
	{
		return s_NewString(gd, args->data.symbol.value, strlen(args->data.symbol.value));
	}
	else
	{
		throw_error(gd, "Requested symbol->string conversion on non-symbol\n");
		return s_NewString(gd, "", 0);
	}
}

static pSlipObject slip_primitive__convert_string_symbol_x(pSlip gd, pSlipObject args)
{
	if (args->type == eType_PAIR)
	{
		pSlipObject result = gd->singleton_EmptyList;

		if (car(args)->type == eType_PAIR)
			return slip_primitive__convert_string_symbol_x(gd, car(args));

		if (car(args)->type != eType_STRING)
		{
			throw_error(gd, "Requested string->symbol conversion on non-string\n");
			return s_NewInteger(gd, 0);
		}

		result = s_NewSymbol(gd, car(args)->data.string.data);
		return cons(gd, result, slip_primitive__convert_string_symbol_x(gd, cdr(args)));
	}
	else if (args->type == eType_EMPTY_LIST)
	{
		return gd->singleton_EmptyList;
	}
	else if (args->type == eType_STRING)
	{
		return s_NewSymbol(gd, args->data.string.data);
	}
	else
	{
		throw_error(gd, "Requested string->symbol conversion on non-string\n");
		return s_NewInteger(gd, 0);
	}
}

static pSlipObject slip_primitive__is_type_x(pSlip gd, pSlipObject args, int type)
{
	if (args->type == eType_PAIR)
	{
		pSlipObject result = gd->singleton_EmptyList;

		//if (cdr(args) == gd->singleton_EmptyList)
		//	return slip_primitive__is_type_x(gd, car(args), type);

		if (car(args)->type != type)
			result = gd->singleton_False;
		else
			result = gd->singleton_True;

		return cons(gd, result, slip_primitive__is_type_x(gd, cdr(args), type));
	}
	else if (args->type == eType_EMPTY_LIST)
	{
		return gd->singleton_EmptyList;
	}
	else
	{
		if (args->type != type)
			return gd->singleton_False;
		else
			return gd->singleton_True;
	}
}

static pSlipObject slip_primitive__convert_string_num(pSlip gd, pSlipObject args)
{
	if (sIsObject_EmptyList(gd, cdr(args)) == S_FALSE)
		return slip_primitive__convert_string_num_x(gd, args);
	else
		return slip_primitive__convert_string_num_x(gd, car(args));
}

static pSlipObject slip_primitive__convert_num_string(pSlip gd, pSlipObject args)
{
	if (sIsObject_EmptyList(gd, cdr(args)) == S_FALSE)
		return slip_primitive__convert_num_string_x(gd, args);
	else
		return slip_primitive__convert_num_string_x(gd, car(args));
}

static pSlipObject slip_primitive__convert_char_int(pSlip gd, pSlipObject args)
{
	if (sIsObject_EmptyList(gd, cdr(args)) == S_FALSE)
		return slip_primitive__convert_char_int_x(gd, args);
	else
		return slip_primitive__convert_char_int_x(gd, car(args));
}

static pSlipObject slip_primitive__convert_int_char(pSlip gd, pSlipObject args)
{
	if (sIsObject_EmptyList(gd, cdr(args)) == S_FALSE)
		return slip_primitive__convert_int_char_x(gd, args);
	else
		return slip_primitive__convert_int_char_x(gd, car(args));
}

static pSlipObject slip_primitive__convert_symbol_string(pSlip gd, pSlipObject args)
{
	if (sIsObject_EmptyList(gd, cdr(args)) == S_FALSE)
		return slip_primitive__convert_symbol_string_x(gd, args);
	else
		return slip_primitive__convert_symbol_string_x(gd, car(args));
}


static pSlipObject slip_primitive__convert_string_symbol(pSlip gd, pSlipObject args)
{
	if (sIsObject_EmptyList(gd, cdr(args)) == S_FALSE)
		return slip_primitive__convert_string_symbol_x(gd, args);
	else
		return slip_primitive__convert_string_symbol_x(gd, car(args));
}

static pSlipObject slip_primitive__is_bool(pSlip gd, pSlipObject args)
{
	if (sIsObject_EmptyList(gd, cdr(args)) == S_FALSE)
		return slip_primitive__is_type_x(gd, args, eType_BOOL);
	else
		return slip_primitive__is_type_x(gd, car(args), eType_BOOL);
}

static pSlipObject slip_primitive__is_string(pSlip gd, pSlipObject args)
{
	if (sIsObject_EmptyList(gd, cdr(args)) == S_FALSE)
		return slip_primitive__is_type_x(gd, args, eType_STRING);
	else
		return slip_primitive__is_type_x(gd, car(args), eType_STRING);
}

static pSlipObject slip_primitive__is_nil(pSlip gd, pSlipObject args)
{
	if (sIsObject_EmptyList(gd, cdr(args)) == S_FALSE)
		return slip_primitive__is_type_x(gd, args, eType_NIL);
	else
		return slip_primitive__is_type_x(gd, car(args), eType_NIL);
}

static pSlipObject slip_primitive__is_symbol(pSlip gd, pSlipObject args)
{
	if (sIsObject_EmptyList(gd, cdr(args)) == S_FALSE)
		return slip_primitive__is_type_x(gd, args, eType_SYMBOL);
	else
		return slip_primitive__is_type_x(gd, car(args), eType_SYMBOL);
}

static pSlipObject slip_primitive__is_int(pSlip gd, pSlipObject args)
{
	if (sIsObject_EmptyList(gd, cdr(args)) == S_FALSE)
		return slip_primitive__is_type_x(gd, args, eType_INTNUM);
	else
		return slip_primitive__is_type_x(gd, car(args), eType_INTNUM);
}

static pSlipObject slip_primitive__is_char(pSlip gd, pSlipObject args)
{
	if (sIsObject_EmptyList(gd, cdr(args)) == S_FALSE)
		return slip_primitive__is_type_x(gd, args, eType_CHARACTER);
	else
		return slip_primitive__is_type_x(gd, car(args), eType_CHARACTER);
}

static pSlipObject slip_primitive__is_proc(pSlip gd, pSlipObject args)
{
	if (sIsObject_EmptyList(gd, cdr(args)) == S_FALSE)
		return slip_primitive__is_type_x(gd, args, eType_PRIMITIVE_PROC);
	else
		return slip_primitive__is_type_x(gd, car(args), eType_PRIMITIVE_PROC);
}

static pSlipObject slip_primitive__is_pair(pSlip gd, pSlipObject args)
{
	if (args->type == eType_PAIR)
		return gd->singleton_True;
	else
		return gd->singleton_False;
}

pSlipObject slip_primitive__quotient(pSlip gd, pSlipObject args)
{
	return s_NewInteger(gd, ((car(args) )->data.intnum.value) / ((cadr(args))->data.intnum.value));
}

pSlipObject slip_primitive__remainder(pSlip gd, pSlipObject args)
{
	return s_NewInteger(gd, ((car(args) )->data.intnum.value) % ((cadr(args))->data.intnum.value));
}

pSlipObject slip_primitive__is_num_equal(pSlip gd, pSlipObject args)
{
	int64_t value;

	if (car(args)->type != eType_INTNUM)
	{
		throw_error(gd, "equal on non-integer");
		return gd->singleton_False;
	}

	value = (car(args))->data.intnum.value;

	args = cdr(args);

	while (sIsObject_EmptyList(gd, args) == S_FALSE)
	{
		if (car(args)->type != eType_INTNUM)
		{
			throw_error(gd, "equal on non-integer");
			return gd->singleton_False;
		}

		if (value != car(args)->data.intnum.value)
		{
			return gd->singleton_False;
		}
	}
	return gd->singleton_True;
}

pSlipObject slip_primitive__quotient_proc(pSlip gd, pSlipObject args)
{
	div_t nn;
	int64_t v1;
	int64_t v2;

	if (car(args)->type != eType_INTNUM)
	{
		throw_error(gd, "equal on non-integer");
		return gd->singleton_False;
	}

	v1 = (car(args))->data.intnum.value;

	args = cdr(args);
	if (car(args)->type != eType_INTNUM)
	{
		throw_error(gd, "equal on non-integer");
		return gd->singleton_False;
	}
	v2 = (car(args))->data.intnum.value;

	nn = div(v1, v2);

	return s_NewInteger(gd, nn.quot);
}

pSlipObject slip_primitive__remainder_proc(pSlip gd, pSlipObject args)
{
	div_t nn;
	int64_t v1;
	int64_t v2;

	if (car(args)->type != eType_INTNUM)
	{
		throw_error(gd, "equal on non-integer");
		return gd->singleton_False;
	}

	v1 = (car(args))->data.intnum.value;

	args = cdr(args);
	if (car(args)->type != eType_INTNUM)
	{
		throw_error(gd, "equal on non-integer");
		return gd->singleton_False;
	}
	v2 = (car(args))->data.intnum.value;

	nn = div(v1, v2);

	return s_NewInteger(gd, nn.rem);
}

pSlipObject slip_primitive__cons(pSlip gd, pSlipObject args)
{
	return cons(gd, car(args), cadr(args));
}

pSlipObject slip_primitive__car(pSlip gd, pSlipObject args)
{
	return caar(args);
}

pSlipObject slip_primitive__cdr(pSlip gd, pSlipObject args)
{
	return cdar(args);
}

pSlipObject slip_primitive__set_car(pSlip gd, pSlipObject args)
{
	set_car(car(args), cadr(args));
	return gd->singleton_OKSymbol;
}

pSlipObject slip_primitive__set_cdr(pSlip gd, pSlipObject args)
{
	set_cdr(car(args), cadr(args));
	return gd->singleton_OKSymbol;
}

pSlipObject slip_primitive__list(pSlip gd, pSlipObject args)
{
	return args;
}

pSlipObject slip_primitive__eq(pSlip gd, pSlipObject args)
{
	pSlipObject o1;
	pSlipObject o2;

	o1 = car(args);
	o2 = cadr(args);

	if (o1->type != o2->type)
		return gd->singleton_False;

	switch (o1->type)
	{
		case eType_INTNUM:
			return(o1->data.intnum.value == o2->data.intnum.value) ? gd->singleton_True : gd->singleton_False;
			break;

		case eType_CHARACTER:
			return(o1->data.character.value == o2->data.character.value) ? gd->singleton_True : gd->singleton_False;
			break;

		case eType_STRING:
			if (o1->data.string.length == o2->data.string.length)
			{
				return(memcmp(o1->data.string.data, o2->data.string.data, o1->data.string.length) == 0) ? gd->singleton_True : gd->singleton_False;
			}
			else
				return gd->singleton_False;
			break;

		default:
			return(o1 == o2) ? gd->singleton_True : gd->singleton_False;
	}
}


void slip_install_primitives(pSlip gd, pSlipEnvironment env)
{
	slip_add_procedure(gd, env, "+", slip_primitive__add_proc);
	slip_add_procedure(gd, env, "-", slip_primitive__sub_proc);
	slip_add_procedure(gd, env, "*", slip_primitive__mul_proc);
	slip_add_procedure(gd, env, "/", slip_primitive__div_proc);
	slip_add_procedure(gd, env, "=", slip_primitive__is_num_equal);
	slip_add_procedure(gd, env, "quotient", slip_primitive__quotient_proc);
	slip_add_procedure(gd, env, "remainder", slip_primitive__remainder_proc);

	slip_add_procedure(gd, env, "char->int", slip_primitive__convert_char_int);
	slip_add_procedure(gd, env, "int->char", slip_primitive__convert_int_char);
	slip_add_procedure(gd, env, "number->string", slip_primitive__convert_num_string);
	slip_add_procedure(gd, env, "string->number", slip_primitive__convert_string_num);
	slip_add_procedure(gd, env, "symbol->string", slip_primitive__convert_symbol_string);
	slip_add_procedure(gd, env, "string->symbol", slip_primitive__convert_string_symbol);

	slip_add_procedure(gd, env, "boolean?", slip_primitive__is_bool);
	slip_add_procedure(gd, env, "string?", slip_primitive__is_string);
	slip_add_procedure(gd, env, "nil?", slip_primitive__is_nil);
	slip_add_procedure(gd, env, "int?", slip_primitive__is_int);
	slip_add_procedure(gd, env, "char?", slip_primitive__is_char);
	slip_add_procedure(gd, env, "symbol?", slip_primitive__is_symbol);
	slip_add_procedure(gd, env, "procedure?", slip_primitive__is_proc);
	slip_add_procedure(gd, env, "pair?", slip_primitive__is_pair);

	slip_add_procedure(gd, env, "eq?", slip_primitive__eq);

	slip_add_procedure(gd, env, "cons", slip_primitive__cons);
	slip_add_procedure(gd, env, "car", slip_primitive__car);
	slip_add_procedure(gd, env, "cdr", slip_primitive__cdr);
	slip_add_procedure(gd, env, "set-car!", slip_primitive__set_car);
	slip_add_procedure(gd, env, "set-cdr!", slip_primitive__set_cdr);
	slip_add_procedure(gd, env, "list", slip_primitive__list);

}

