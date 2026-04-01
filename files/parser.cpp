/* Recursive descent parser for integer expressions
   which may include variables and function calls.
*/

#include <cmath>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include "globals.h"
#include "parser.h"
#include "interp.h"
#include "cache_memory.h"

static int eval_array_index_expression(char* index)
{
	int index_value = 0;
	int token_type_temp;
	char temp[SETTINGS_ID_LEN + 1];

	my_strcpy_s(temp, SETTINGS_ID_LEN, G_TOKEN_BUFFER);
	my_strcpy_s(G_TOKEN_BUFFER, SETTINGS_ID_LEN, index);
	token_type_temp = G_CURRENT_TOKEN_TYPE;
	G_CURRENT_TOKEN_TYPE = IDENTIFIER;

	char* prog_temp = G_PROGRAM_POINTER;
	G_PROGRAM_POINTER = index;
	char* p_zero = strchr(G_PROGRAM_POINTER, '\0');
	*p_zero++ = ';';
	*p_zero = '\0';

	// Переключаемся на посимвольный парсинг для индексного выражения
	int saved_token_pos = g_token_pos;
	bool saved_use_source = g_use_source_directly;
	g_use_source_directly = true;

	eval_exp(&index_value, 1);

	g_use_source_directly = saved_use_source;
	g_token_pos = saved_token_pos;
	G_PROGRAM_POINTER = prog_temp;
	G_CURRENT_TOKEN_TYPE = token_type_temp;
	my_strcpy_s(G_TOKEN_BUFFER, SETTINGS_ID_LEN, temp);

	return index_value;
}

static int read_array_value(const array_type& array, int index_value)
{
	switch (array.a_type)
	{
	case CHAR:
		return *((char*)array.adr + index_value);
	case INT:
		return *((int*)array.adr + index_value);
	case FLOAT:
		return (int)(*((float*)array.adr + index_value));
	case DOUBLE:
		return (int)(*((double*)array.adr + index_value));
	}

	sntx_err(SYNTAX);
	return 0;
}

static void write_array_value(array_type& array, int index_value, int value)
{
	switch (array.a_type)
	{
	case CHAR:
		*((char*)array.adr + index_value) = (char)value;
		return;
	case INT:
		*((int*)array.adr + index_value) = value;
		return;
	case FLOAT:
		*((float*)array.adr + index_value) = (float)value;
		return;
	case DOUBLE:
		*((double*)array.adr + index_value) = (double)value;
		return;
	}

	sntx_err(SYNTAX);
}

// ДОБАВИТЬ - ИЗМЕНИТЬ - НЕ ТОЛЬКО INT
/* Вход в парсер.*/
void eval_exp(int* value, int get_token_)
{
	if (get_token_)
		get_token();
	if (!*G_TOKEN_BUFFER) {
		sntx_err(NO_EXP);
		return;
	}
	if (*G_TOKEN_BUFFER == ';') {
		*value = 0; /* пустое выражение (оператор) */
		return;
	}
	eval_exp0(value);
	putback(); /* возвращает последний считанный токен в поток ввода */
}

// ИЗМЕНИТЬ ДЕСЬ - ДОБАВИТЬ ОБРАБОТКУ МАССИВОВ
/* Обработка присваивания */
void eval_exp0(int* value)
{
	char temp[SETTINGS_ID_LEN];  /* holds name of var receiving
						   the assignment */
	char temp_token_type;
	bool is_array_token = false;

	if (G_CURRENT_TOKEN_TYPE == IDENTIFIER) {

		char name[SETTINGS_ID_LEN + 1] = { 0 };
		char size[SETTINGS_ID_LEN + 1] = { 0 };
		char* pos;
		char token_temp[SETTINGS_ID_LEN + 1];
		my_strcpy_s(token_temp, SETTINGS_ID_LEN, G_TOKEN_BUFFER);
		if (pos = strchr(token_temp, '[')) {
			extract_array_name_index(name, size, token_temp, pos);
			is_array_token = ::is_array(name);
		}
		if (is_var(G_TOKEN_BUFFER) || is_array_token) {  /* if a var, see if assignment */
			my_strcpy_s(temp, SETTINGS_ID_LEN, G_TOKEN_BUFFER);
			temp_token_type = G_CURRENT_TOKEN_TYPE;
			get_token();
			if (*G_TOKEN_BUFFER == '=') {  /* is an assignment */
				not_rekurs_eval_exp0_sim = 0;
				get_token();
				eval_exp0(value);  /* get value to assign */
				if (G_SIM_MODE) {
					// В sim-режиме трассируем запись массива
					if (is_array_token) {
						assign_array(name, 0, size);  // g_sim_mode=true → trace_handler("w")
						if (first_iter)
						{
#ifdef NEW_M
							int temp_i = strlen(temp);
							char* pos_local = &oper_plan[oper_num][index_in_oper_plan];
							memcpy(pos_local, temp, temp_i);
							tokens_write_length[oper_num] = temp_i;
#else
							int i = 0;
							oper_plan[oper_num][index_in_oper_plan++] = '=';
							while (temp[i] != '\0')
								oper_plan[oper_num][index_in_oper_plan++] = temp[i++];
							oper_plan[oper_num][index_in_oper_plan++] = '\0';
#endif
						}
					}
					// Переменные пока не моделируем в sim-режиме
				}
				else {
					if (is_var(temp))
						assign_var_array(temp, *value, 0, 0);  /* assign the value */
					else
						assign_var_array(name, *value, 1, size);  /* assign the value */
				}
				not_rekurs_eval_exp0_sim = 1;
				return;
			}
			else {  /* not an assignment */
				putback();  /* restore original token */
				my_strcpy_s(G_TOKEN_BUFFER, 80, temp);
				G_CURRENT_TOKEN_TYPE = temp_token_type;
			}
		}
	}
	eval_exp1(value);
}

/* обработка условных операторов. */
void eval_exp1(int* value)
{
	int partial_value;
	char op;
	char relops[7] = {
	  LT, LE, GT, GE, EQ, NE, 0
	};

	eval_exp2(value);
	op = *G_TOKEN_BUFFER;
	if (strchr(relops, op)) {
		get_token();
		eval_exp2(&partial_value);
		if (!G_SIM_MODE) {
			switch (op) {  /* perform the relational operation */
			case LT:
				*value = *value < partial_value;
				break;
			case LE:
				*value = *value <= partial_value;
				break;
			case GT:
				*value = *value > partial_value;
				break;
			case GE:
				*value = *value >= partial_value;
				break;
			case EQ:
				*value = *value == partial_value;
				break;
			case NE:
				*value = *value != partial_value;
				break;
			}
		}
	}
}

/*  Сложение и вычитание. */
void eval_exp2(int* value)
{
	char  op;
	int partial_value;

	eval_exp3(value);
	while ((op = *G_TOKEN_BUFFER) == '+' || op == '-') {
		get_token();
		eval_exp3(&partial_value);
		if (!G_SIM_MODE) {
			switch (op) { /* add or subtract */
			case '-':
				*value = *value - partial_value;
				break;
			case '+':
				*value = *value + partial_value;
				break;
			}
		}
	}
}

/* Умножение и деление. */
void eval_exp3(int* value)
{
	char  op;
	int partial_value, t;

	eval_exp4(value);
	while ((op = *G_TOKEN_BUFFER) == '*' || op == '/' || op == '%') {
		get_token();
		eval_exp4(&partial_value);
		if (!G_SIM_MODE) {
			switch (op) { /* mul, div, or modulus */
			case '*':
				*value = *value * partial_value;
				break;
			case '/':
				if (partial_value == 0) sntx_err(DIV_BY_ZERO);
				*value = (*value) / partial_value;
				break;
			case '%':
				t = (*value) / partial_value;
				*value = *value - (t * partial_value);
				break;
			}
		}
	}
}

/* Унарный + или -. */
void eval_exp4(int* value)
{
	char  op;

	op = '\0';
	if (*G_TOKEN_BUFFER == '+' || *G_TOKEN_BUFFER == '-') {
		op = *G_TOKEN_BUFFER;
		get_token();
	}
	eval_exp5(value);
	if (!G_SIM_MODE && op)
		if (op == '-') *value = -(*value);
}

/* Обработка выражений в скобках. */
void eval_exp5(int* value)
{
	if (*G_TOKEN_BUFFER == '(') {
		get_token();
		eval_exp0(value);   /* get subexpression */
		if (*G_TOKEN_BUFFER != ')') sntx_err(PAREN_EXPECTED);
		get_token();
	}
	else
		atom(value);
}

/* определяется значение числа, переменной, элемента массива или вызова функции. */
void atom(int* value)
{
	int i;
	char name[SETTINGS_ID_LEN + 1];
	char size[SETTINGS_ID_LEN + 1];
	char token_temp[SETTINGS_ID_LEN + 1];

	switch (G_CURRENT_TOKEN_TYPE) {
	case IDENTIFIER:
	{
		char* pos;
		bool is_array_atom = false;
		my_strcpy_s(token_temp, SETTINGS_ID_LEN, G_TOKEN_BUFFER);
		pos = strchr(token_temp, '[');
		is_array_atom = (pos != nullptr);

		// В sim-режиме на первой итерации записываем oper_plan для массивов
		if (G_SIM_MODE && first_iter && is_array_atom)
		{
			char* token_t = G_TOKEN_BUFFER;
			while (*token_t != '\0')
				oper_plan[oper_num][index_in_oper_plan++] = *token_t++;
#ifndef NEW_M
			oper_plan[oper_num][index_in_oper_plan++] = SETTINGS_DELIMITER_OPER_PLAN;
#endif
			tokens_read[oper_num]++;
			tokens_read_length[oper_num][token_read_num++] = strlen(G_TOKEN_BUFFER);
		}

		i = internal_func(G_TOKEN_BUFFER);
		if (i != -1) {  /* call "standard library" function */
			if (!G_SIM_MODE)
				*value = (*intern_func[i].p)();
			else
				(*intern_func[i].p)();
		}
		else if (find_func(G_TOKEN_BUFFER) >= 0) { /* call user-defined function */
			call();
			if (!G_SIM_MODE)
				*value = ret_value;
		}
		else
		{
			if (is_array_atom)
				extract_array_name_index(name, size, token_temp, pos);

			if (G_SIM_MODE) {
				// В sim-режиме трассируем только массивы, переменные не моделируем
				if (is_array_atom)
					find_array(name, size);  // g_sim_mode=true → trace_handler("r")
			}
			else {
				if (is_var(G_TOKEN_BUFFER))
					*value = find_var_array(G_TOKEN_BUFFER, 0, 0);
				else
					*value = find_var_array(name, 1, size);
			}
		}
		get_token();
		return;
	}
	case NUMBER: /* is numeric constant */
		if (!G_SIM_MODE)
			*value = atoi(G_TOKEN_BUFFER);
		get_token();
		return;
	case DELIMITER: /* see if character constant */
		if (*G_TOKEN_BUFFER == '\'') {
			if (!G_SIM_MODE)
				*value = *G_PROGRAM_POINTER;
			G_PROGRAM_POINTER++;
			if (*G_PROGRAM_POINTER != '\'') sntx_err(QUOTE_EXPECTED);
			G_PROGRAM_POINTER++;
			get_token();
			return;
		}
		if (*G_TOKEN_BUFFER == ')') return; /* process empty expression */
		else sntx_err(SYNTAX); /* syntax error */
	default:
		sntx_err(SYNTAX); /* syntax error */
	}
}


/* Return index of internal library function or -1 if
   not found.
*/
int internal_func(char* s)
{
	int i;

	for (i = 0; intern_func[i].f_name[0]; i++) {
		if (!strcmp(intern_func[i].f_name, s))  return i;
	}
	return -1;
}




// ИЗМЕНИЛ - ДОРАБОТАТЬ - VALUE ТОЛЬКО INT
void assign_var_array(char* var_name, int value, int is_array, char* array_index)
{
	if (!is_array)
		assign_var(var_name, value);
	else
		assign_array(var_name, value, array_index);
}

// ДОБАВИЛ
int find_array(char* name, char* index)
{
	int i;
	int index_value;

	for (i = G_STACK_TOP_FOR_LOCAL_ARRAYS - 1; i >= G_CALL_STACK[functos - 1].arrays; i--) {
		if (!strcmp(G_STACK_FOR_LOCAL_ARRAYS[i].array_name, name)) {
			bool saved_sim = G_SIM_MODE;
			G_SIM_MODE = false;
			index_value = eval_array_index_expression(index);
			G_SIM_MODE = saved_sim;
			if (G_SIM_MODE) {
				cache.trace_handler((G_STACK_FOR_LOCAL_ARRAYS[i].start_address + index_value * G_STACK_FOR_LOCAL_ARRAYS[i].sizeofop), G_STACK_FOR_LOCAL_ARRAYS[i].array_name, "r", "");
				return 0;
			}
			return read_array_value(G_STACK_FOR_LOCAL_ARRAYS[i], index_value);
		}
	}

	for (i = 0; i < G_ARRAY_INDEX; i++) {
		if (!strcmp(G_GLOBAL_ARRAYS_STORAGE[i].array_name, name)) {
			bool saved_sim = G_SIM_MODE;
			G_SIM_MODE = false;
			index_value = eval_array_index_expression(index);
			G_SIM_MODE = saved_sim;
			if (G_SIM_MODE) {
				cache.trace_handler((G_GLOBAL_ARRAYS_STORAGE[i].start_address + index_value * G_GLOBAL_ARRAYS_STORAGE[i].sizeofop), G_GLOBAL_ARRAYS_STORAGE[i].array_name, "r", "");
				return 0;
			}
			return read_array_value(G_GLOBAL_ARRAYS_STORAGE[i], index_value);
		}
	}

	sntx_err(NOT_VAR);
	return -1;
}

/* Find the value of a variable. */
int find_var(char* s)
{
	int i;

	/* first, see if it's a local variable */
	for (i = G_STACK_TOP_FOR_LOCAL_VARS - 1; i >= G_CALL_STACK[functos - 1].vars; i--)
		if (!strcmp(G_STACK_FOR_LOCAL_VARS[i].var_name, s))
			return G_STACK_FOR_LOCAL_VARS[i].value;

	/* otherwise, try global vars */
	for (i = 0; i < G_VAR_INDEX; i++)
		if (!strcmp(G_GLOBAL_VARS_STORAGE[i].var_name, s))
			return G_GLOBAL_VARS_STORAGE[i].value;

	sntx_err(NOT_VAR); /* variable not found */
	return -1;
}

int find_var_array(char* name, int is_array, char* index)
{
	if (is_array)
		return find_array(name, index);
	else
		return find_var(name);
}

int find_func(char* name)
{
	int i;

	for (i = 0; i < func_index; i++)
		if (!strcmp(name, G_FUNC_TABLE[i].func_name))
			return G_FUNC_TABLE[i].token_index;

	return -1;
}

/* Call a function. */
void call(void)
{
	int loc_idx;
	int saved_pos;
	int stack_top_for_locacl_vars, stack_top_for_local_arrays;

	loc_idx = find_func(G_TOKEN_BUFFER); /* find entry point of function */
	if (loc_idx < 0)
		sntx_err(FUNC_UNDEF); /* function not defined */
	else {
		stack_top_for_locacl_vars = G_STACK_TOP_FOR_LOCAL_VARS;  /* save local var stack index */
		stack_top_for_local_arrays = G_STACK_TOP_FOR_LOCAL_ARRAYS;
		// НЕ !!! ДОБАВИЛ ПЕРЕДАЧУ ПАРАМЕТРОВ
		get_args();  /* get function arguments */
		saved_pos = g_token_pos; /* save return location */
		func_push(stack_top_for_locacl_vars, stack_top_for_local_arrays);  /* save local var stack index */
		g_token_pos = loc_idx;  /* reset prog to start of function */
		ret_occurring = 0; /* P the return occurring variable */
		get_params(); /* load the function's parameters with the values of the arguments */
		interp_block(); /* interpret the function */
		ret_occurring = 0; /* Clear the return occurring variable */
		g_token_pos = saved_pos; /* reset the program pointer */
		struct var_array_stack av = func_pop(); /* reset the local var stack */
		// Освобождаем память локальных массивов текущего фрейма
		for (int i = G_STACK_TOP_FOR_LOCAL_ARRAYS - 1; i >= av.arrays; i--) {
			free(G_STACK_FOR_LOCAL_ARRAYS[i].adr);
			G_STACK_FOR_LOCAL_ARRAYS[i].adr = nullptr;
		}
		G_STACK_TOP_FOR_LOCAL_VARS = av.vars;
		G_STACK_TOP_FOR_LOCAL_ARRAYS = av.arrays;
	}
}


/* Get a token from source text (old behavior, used for tokenization and index expressions). */
char get_token_from_source(void)
{

	char* temp;

	G_CURRENT_TOKEN_TYPE = 0; G_CURRENT_TOKEN = 0; // make 0 global vars...

	temp = G_TOKEN_BUFFER; // save pointer which linked with global TOKEN array start
	*temp = '\0'; // save '\0' to the first byte of global array G_TOKEN

	/* skip over white space */
	while (iswhite(*G_PROGRAM_POINTER) && *G_PROGRAM_POINTER) ++G_PROGRAM_POINTER;

	/* Handle Windows and Mac newlines */
	if (*G_PROGRAM_POINTER == '\r') {
		++G_PROGRAM_POINTER;
		/* Only skip \n if it exists (if it doesn't, we are running on mac) */
		if (*G_PROGRAM_POINTER == '\n') {
			++G_PROGRAM_POINTER;
		}
		/* skip over white space */
		while (iswhite(*G_PROGRAM_POINTER) && *G_PROGRAM_POINTER) ++G_PROGRAM_POINTER;
	}



	/* Handle Unix newlines */
	if (*G_PROGRAM_POINTER == '\n') {
		++G_PROGRAM_POINTER;
		/* skip over white space */
		while (iswhite(*G_PROGRAM_POINTER) && *G_PROGRAM_POINTER) ++G_PROGRAM_POINTER;
	}

	if (*G_PROGRAM_POINTER == '\0') { /* end of file */
		*G_TOKEN_BUFFER = '\0';
		G_CURRENT_TOKEN = FINISHED;
		return (G_CURRENT_TOKEN_TYPE = DELIMITER);
	}

	if (strchr("{}", *G_PROGRAM_POINTER)) { /* block delimiters */
		*temp = *G_PROGRAM_POINTER;
		temp++;
		*temp = '\0';
		G_PROGRAM_POINTER++;
		return (G_CURRENT_TOKEN_TYPE = BLOCK);
	}

	/* look for comments */
	if (*G_PROGRAM_POINTER == '/')
		if (*(G_PROGRAM_POINTER + 1) == '*') { /* is a comment */
			G_PROGRAM_POINTER += 2;
			do { /* find end of comment */
				while (*G_PROGRAM_POINTER != '*' && *G_PROGRAM_POINTER != '\0') G_PROGRAM_POINTER++;
				if (*G_PROGRAM_POINTER == '\0') {
					G_PROGRAM_POINTER--;
					break;
				}
				G_PROGRAM_POINTER++;
			} while (*G_PROGRAM_POINTER != '/');
			G_PROGRAM_POINTER++;
		}

	/* look for C++ style comments */
	if (*G_PROGRAM_POINTER == '/')
		if (*(G_PROGRAM_POINTER + 1) == '/') { /* is a comment */
			G_PROGRAM_POINTER += 2;
			/* find end of line */
			while (*G_PROGRAM_POINTER != '\r' && *G_PROGRAM_POINTER != '\n' && *G_PROGRAM_POINTER != '\0') G_PROGRAM_POINTER++;
			if (*G_PROGRAM_POINTER == '\r' && *(G_PROGRAM_POINTER + 1) == '\n') {
				G_PROGRAM_POINTER++;
			}
		}

	/* look for the end of file after a comment */
	if (*G_PROGRAM_POINTER == '\0') { /* end of file */
		*G_TOKEN_BUFFER = '\0';
		G_CURRENT_TOKEN = FINISHED;
		return (G_CURRENT_TOKEN_TYPE = DELIMITER);
	}

	if (strchr("!<>=", *G_PROGRAM_POINTER)) { /* is or might be
								   a relational operator */
		switch (*G_PROGRAM_POINTER) {
		case '=': if (*(G_PROGRAM_POINTER + 1) == '=') {
			G_PROGRAM_POINTER++; G_PROGRAM_POINTER++;
			*temp = EQ;
			temp++; *temp = EQ; temp++;
			*temp = '\0';
		}
				break;
		case '!': if (*(G_PROGRAM_POINTER + 1) == '=') {
			G_PROGRAM_POINTER++; G_PROGRAM_POINTER++;
			*temp = NE;
			temp++; *temp = NE; temp++;
			*temp = '\0';
		}
				break;
		case '<': if (*(G_PROGRAM_POINTER + 1) == '=') {
			G_PROGRAM_POINTER++; G_PROGRAM_POINTER++;
			*temp = LE; temp++; *temp = LE;
		}
				else {
			G_PROGRAM_POINTER++;
			*temp = LT;
		}
				temp++;
				*temp = '\0';
				break;
		case '>': if (*(G_PROGRAM_POINTER + 1) == '=') {
			G_PROGRAM_POINTER++; G_PROGRAM_POINTER++;
			*temp = GE; temp++; *temp = GE;
		}
				else {
			G_PROGRAM_POINTER++;
			*temp = GT;
		}
				temp++;
				*temp = '\0';
				break;
		}
		if (*G_TOKEN_BUFFER) return(G_CURRENT_TOKEN_TYPE = DELIMITER);
	}

	if (strchr("+-*^/%=;(),'", *G_PROGRAM_POINTER)) { /* delimiter */
		*temp = *G_PROGRAM_POINTER;
		G_PROGRAM_POINTER++; /* advance to next position */
		temp++;
		*temp = '\0';
		return (G_CURRENT_TOKEN_TYPE = DELIMITER);
	}

	if (*G_PROGRAM_POINTER == '"') { /* quoted string */
		G_PROGRAM_POINTER++;
		while ((*G_PROGRAM_POINTER != '"' && *G_PROGRAM_POINTER != '\r' && *G_PROGRAM_POINTER != '\n' && *G_PROGRAM_POINTER != '\0') || (*G_PROGRAM_POINTER == '"' && *(G_PROGRAM_POINTER - 1) == '\\')) *temp++ = *G_PROGRAM_POINTER++;
		if (*G_PROGRAM_POINTER == '\r' || *G_PROGRAM_POINTER == '\n' || *G_PROGRAM_POINTER == '\0') sntx_err(SYNTAX);
		G_PROGRAM_POINTER++; *temp = '\0';
		str_replace(G_TOKEN_BUFFER, "\\a", "\a");
		str_replace(G_TOKEN_BUFFER, "\\b", "\b");
		str_replace(G_TOKEN_BUFFER, "\\f", "\f");
		str_replace(G_TOKEN_BUFFER, "\\n", "\n");
		str_replace(G_TOKEN_BUFFER, "\\r", "\r");
		str_replace(G_TOKEN_BUFFER, "\\t", "\t");
		str_replace(G_TOKEN_BUFFER, "\\v", "\v");
		str_replace(G_TOKEN_BUFFER, "\\\\", "\\");
		str_replace(G_TOKEN_BUFFER, "\\\'", "\'");
		str_replace(G_TOKEN_BUFFER, "\\\"", "\"");
		return (G_CURRENT_TOKEN_TYPE = STRING);
	}

	if (isdigit((int)*G_PROGRAM_POINTER)) { /* number */
		while (!isdelim(*G_PROGRAM_POINTER)) *temp++ = *G_PROGRAM_POINTER++;
		*temp = '\0';
		return (G_CURRENT_TOKEN_TYPE = NUMBER);
	}

	if (isalpha((int)*G_PROGRAM_POINTER)) { /* var or command */
		while (!isdelim(*G_PROGRAM_POINTER))
		{
			*temp++ = *G_PROGRAM_POINTER++; // add token into G_TOKEN array
			if (*G_PROGRAM_POINTER == '[') {
				*temp = *G_PROGRAM_POINTER;
				while (*G_PROGRAM_POINTER != ']') *temp++ = *G_PROGRAM_POINTER++;
				*temp = *G_PROGRAM_POINTER;
				//return (token_type = IDENTIFIER);
			}
		}
		G_CURRENT_TOKEN_TYPE = TEMP;
	}

	*temp = '\0';

	/* see if a string is a command or a variable */
	if (G_CURRENT_TOKEN_TYPE == TEMP) {
		G_CURRENT_TOKEN = look_up(G_TOKEN_BUFFER); /* convert to internal rep */
		if (G_CURRENT_TOKEN) G_CURRENT_TOKEN_TYPE = KEYWORD; /* is a keyword */
		//else if (strchr(token, '[')) token_type = ARRAY;
		else G_CURRENT_TOKEN_TYPE = IDENTIFIER;
	}
	return G_CURRENT_TOKEN_TYPE;
}

/* Предварительная токенизация всего исходного текста. */
void tokenize_source(void)
{
	g_token_stream.clear();
	char* saved_prog = G_PROGRAM_POINTER;
	G_PROGRAM_POINTER = p_buf;

	while (true) {
		char* pos_before = G_PROGRAM_POINTER;
		get_token_from_source();

		TokenInfo tok;
		tok.token_type = G_CURRENT_TOKEN_TYPE;
		tok.token = G_CURRENT_TOKEN;
		my_strcpy_s(tok.text, SETTINGS_MAX_TOKEN_LENGTH, G_TOKEN_BUFFER);
		tok.source_pos = G_PROGRAM_POINTER; // позиция ПОСЛЕ токена
		tok.jit_op_index = -1;

		g_token_stream.push_back(tok);

		if (G_CURRENT_TOKEN == FINISHED) break;
	}

	G_PROGRAM_POINTER = saved_prog;
}

/* Получить следующий токен из предварительно токенизированного потока. */
char get_token(void)
{
	// Для eval_array_index_expression: парсим из source напрямую
	if (g_use_source_directly) {
		return get_token_from_source();
	}

	if (g_token_pos >= (int)g_token_stream.size()) {
		G_TOKEN_BUFFER[0] = '\0';
		G_CURRENT_TOKEN = FINISHED;
		G_CURRENT_TOKEN_TYPE = DELIMITER;
		return G_CURRENT_TOKEN_TYPE;
	}

	const TokenInfo& t = g_token_stream[g_token_pos++];
	my_strcpy_s(G_TOKEN_BUFFER, SETTINGS_MAX_TOKEN_LENGTH, t.text);
	G_CURRENT_TOKEN_TYPE = t.token_type;
	G_CURRENT_TOKEN = t.token;
	G_PROGRAM_POINTER = t.source_pos; // для sntx_err
	return G_CURRENT_TOKEN_TYPE;
}

/* Вернуть токен в поток. */
void putback(void)
{
	if (g_use_source_directly) {
		// Старое поведение: посимвольный откат
		char* t = G_TOKEN_BUFFER;
		for (; *t; t++) G_PROGRAM_POINTER--;
		return;
	}
	if (g_token_pos > 0) g_token_pos--;
}

/* Look up a token's internal representation in the
   token table.
*/
char look_up(char* s)
{
	int i;
	char* p;

	/* convert to lowercase */
	p = s;
	while (*p) { *p = (char)tolower(*p); p++; }

	/* see if token is in table */
	for (i = 0; *G_KEYWORD_TOKEN_TYPE_TABLE[i].command; i++) {
		if (!strcmp(G_KEYWORD_TOKEN_TYPE_TABLE[i].command, s)) return G_KEYWORD_TOKEN_TYPE_TABLE[i].tok;
	}
	return 0; /* unknown command */
}

/* Return true if c is a delimiter. */
int isdelim(char c)
{
	if (strchr(" !;,+-<>'/*%^=()", c) || c == 9 ||
		c == '\r' || c == '\n' || c == 0) return 1;
	return 0;
}

/* Return 1 if c is space or tab. */
int iswhite(char c)
{
	if (c == ' ' || c == '\t') return 1;
	else return 0;
}



/* An in-place modification find and replace of the string.
   Assumes the buffer pointed to by line is large enough to hold the resulting string.*/
static void str_replace(char* line, const char* search, const char* replace)
{
	char* sp;
	while ((sp = strstr(line, search)) != NULL) {
		int search_len = (int)strlen(search);
		int replace_len = (int)strlen(replace);
		int tail_len = (int)strlen(sp + search_len);

		memmove(sp + replace_len, sp + search_len, tail_len + 1);
		memcpy(sp, replace, replace_len);
	}
}

/* Display an error message. */
void sntx_err(int error)
{
	char* p, * temp;
	int linecount = 0;
	int i;

	static const char* e[] = {
	  "syntax error",
	  "unbalanced parentheses",
	  "no expression present",
	  "equals sign expected",
	  "not a variable",
	  "parameter error",
	  "semicolon expected",
	  "unbalanced braces",
	  "function undefined",
	  "type specifier expected",
	  "too many nested function calls",
	  "return without call",
	  "parentheses expected",
	  "while expected",
	  "closing quote expected",
	  "not a string",
	  "too many local variables",
	  "too many local arrays",
	  "division by zero",
	  "too many global variables",
	  "too many global arrays",
	  "too many functions",
	  "too many function parameters",
	  "program too large"
	};
	printf("\n%s", e[error]);
	p = p_buf;
	while (p != G_PROGRAM_POINTER && *p != '\0') {  /* find line number of error */
		p++;
		if (*p == '\r') {
			linecount++;
			if (p == G_PROGRAM_POINTER) {
				break;
			}
			/* See if this is a Windows or Mac newline */
			p++;
			/* If we are a mac, backtrack */
			if (*p != '\n') {
				p--;
			}
		}
		else if (*p == '\n') {
			linecount++;
		}
		else if (*p == '\0') {
			linecount++;
		}
	}
	printf(" in line %d\n", linecount);

	temp = p--;
	for (i = 0; i < 20 && p > p_buf && *p != '\n' && *p != '\r'; i++, p--);
	for (i = 0; i < 30 && p <= temp; i++, p++) printf("%c", *p);

	throw SyntaxError(error);
}

void extract_array_name_index(const char* name, const char* size, const char* token, char* pos)
{
	char array_name_size[SETTINGS_ID_LEN + 1];
#ifdef NEW_M
	char temp = *pos;
#endif
	*pos = '\0';
	//strcpy(array_name_size, token);
	my_strcpy_s((char*)array_name_size, SETTINGS_ID_LEN, token);
	my_strcpy_s((char*)name, SETTINGS_ID_LEN, array_name_size);
#ifdef NEW_M
	*pos = temp;
#endif
	pos++;
	int i = 0;
	while (*pos != ']')
		array_name_size[i++] = *pos++;
	array_name_size[i] = '\0';
	my_strcpy_s((char*)size, SETTINGS_ID_LEN, array_name_size);
}

/* Проверяет, является ли идентификатор переменной. Вовзращает 1 если является
и 0 в противном случае*/
int is_var(char* s)
{
	int i;

	/* first, see if it's a local variable */
	for (i = G_STACK_TOP_FOR_LOCAL_VARS - 1; i >= G_CALL_STACK[functos - 1].vars; i--)
		if (!strcmp(G_STACK_FOR_LOCAL_VARS[i].var_name, s))
			return 1;

	/* otherwise, try global vars */
	for (i = 0; i < G_VAR_INDEX; i++)
		if (!strcmp(G_GLOBAL_VARS_STORAGE[i].var_name, s))
			return 1;

	return 0;
}
// ДОБАВИЛ то же самое для массива
int is_array(char* s)
{
	int i;

	/* first, see if it's a local variable */
	for (i = G_STACK_TOP_FOR_LOCAL_ARRAYS - 1; i >= G_CALL_STACK[functos - 1].arrays; i--)
		if (!strcmp(G_STACK_FOR_LOCAL_ARRAYS[i].array_name, s))
			return 1;

	/* otherwise, try global arrays */
	for (i = 0; i < SETTINGS_NUM_GLOBAL_ARRAYS; i++)
		if (!strcmp(G_GLOBAL_ARRAYS_STORAGE[i].array_name, s))
			return 1;

	return 0;
}

//ДОБАВИЛ
/* Присваивает значение элемнету массива. */
void assign_array(char* array_name, int value, char* index)
{
	int i;
	int index_value;

	for (i = G_STACK_TOP_FOR_LOCAL_ARRAYS - 1; i >= G_CALL_STACK[functos - 1].arrays; i--) {
		if (!strcmp(G_STACK_FOR_LOCAL_ARRAYS[i].array_name, array_name)) {
			bool saved_sim = G_SIM_MODE;
			G_SIM_MODE = false;
			index_value = eval_array_index_expression(index);
			G_SIM_MODE = saved_sim;
			if (G_SIM_MODE) {
				cache.trace_handler((G_STACK_FOR_LOCAL_ARRAYS[i].start_address + index_value * G_STACK_FOR_LOCAL_ARRAYS[i].sizeofop), G_STACK_FOR_LOCAL_ARRAYS[i].array_name, "w", "");
			} else {
				write_array_value(G_STACK_FOR_LOCAL_ARRAYS[i], index_value, value);
			}
			return;
		}
	}

	for (i = 0; i < G_ARRAY_INDEX; i++) {
		if (!strcmp(G_GLOBAL_ARRAYS_STORAGE[i].array_name, array_name)) {
			bool saved_sim = G_SIM_MODE;
			G_SIM_MODE = false;
			index_value = eval_array_index_expression(index);
			G_SIM_MODE = saved_sim;
			if (G_SIM_MODE) {
				cache.trace_handler((G_GLOBAL_ARRAYS_STORAGE[i].start_address + index_value * G_GLOBAL_ARRAYS_STORAGE[i].sizeofop), G_GLOBAL_ARRAYS_STORAGE[i].array_name, "w", "");
			} else {
				write_array_value(G_GLOBAL_ARRAYS_STORAGE[i], index_value, value);
			}
			return;
		}
	}

	sntx_err(NOT_VAR);
}

/* Assign a value to a variable. */
void assign_var(char* var_name, int value)
{
	int i;

	/* first, see if it's a local variable */
	for (i = G_STACK_TOP_FOR_LOCAL_VARS - 1; i >= G_CALL_STACK[functos - 1].vars; i--) {
		if (!strcmp(G_STACK_FOR_LOCAL_VARS[i].var_name, var_name)) {
			G_STACK_FOR_LOCAL_VARS[i].value = value;
			return;
		}
	}
	if (i < G_CALL_STACK[functos - 1].vars)
		/* if not local, try global var table */
		for (i = 0; i < G_VAR_INDEX; i++)
			if (!strcmp(G_GLOBAL_VARS_STORAGE[i].var_name, var_name)) {
				G_GLOBAL_VARS_STORAGE[i].value = value;
				return;
			}
	sntx_err(NOT_VAR); /* variable not found */
}

/* ===== Функции для JIT-режима (последующие итерации цикла) ===== */

// Тонкая обёртка: трассирует чтение массива (для eval_exp_sim_jit)
void find_array_sim(char* name, char* index)
{
	bool saved = G_SIM_MODE;
	G_SIM_MODE = true;
	find_array(name, index);
	G_SIM_MODE = saved;
}

// Тонкая обёртка: трассирует запись массива (для eval_exp_sim_jit)
void assign_array_sim(char* array_name, char* index)
{
	bool saved = G_SIM_MODE;
	G_SIM_MODE = true;
	assign_array(array_name, 0, index);
	G_SIM_MODE = saved;
}

// Глобальные переменные для eval_exp_sim_jit
static char oper_local[SETTINGS_MAX_OPERATOR_LENGTH];
static char name_local[SETTINGS_ID_LEN + 1];
static char size_local[SETTINGS_ID_LEN + 1];
static char* pos_local_jit;
static char* token_l;
static char temp_c;
static char token_local[SETTINGS_ID_LEN + 1];
static int temp_i_jit;
static int pos_int;

// Быстрый прогон: воспроизводит записанный план операций
void eval_exp_sim_jit(int op_index)
{
	// Пропускаем токены оператора до ';' (токены не модифицированы)
	do {
		get_token();
	} while (*G_TOKEN_BUFFER != ';');
	// ';' прочитан (как в старом NUMBER_OPERATORS — НЕ делаем putback)

	int index = op_index;
	// выполняем план под номером index
	// сначала чтения
#ifdef NEW_M
	token_l = oper_plan[index];
	for (int i = 0; i < tokens_read[index]; i++) // цикл по токенам чтения
	{
		temp_c = token_l[tokens_read_length[index][i]];
		token_l[tokens_read_length[index][i]] = '\0';
		pos_local_jit = strchr(token_l, '[');
		extract_array_name_index(name_local, size_local, token_l, pos_local_jit);
		// Верим, считаем, что все массивы объявлены
		find_array_sim(name_local, size_local);
		token_l[tokens_read_length[index][i]] = temp_c;
		token_l += tokens_read_length[index][i];
	}
#else
	pos_int = 0;
	char* token_local_ptr = NULL;
	while (oper_plan[index][pos_int] != '=')
	{
		token_local_ptr = token_local;
		while (oper_plan[index][pos_int] != DELIMITER_OPER_PLAN)
			*token_local_ptr++ = oper_plan[index][pos_int++];
		*token_local_ptr = '\0';
		pos_int++;
		pos_local_jit = strchr(token_local, '[');
		extract_array_name_index(name_local, size_local, token_local, pos_local_jit);
		// Верим, считаем, что все массивы объявлены
		find_array_sim(name_local, size_local);
	}
#endif
	// теперь запись
#ifdef NEW_M
	pos_local_jit = strchr(token_l, '[');
	extract_array_name_index(name_local, size_local, token_l, pos_local_jit);
	assign_array_sim(name_local, size_local);
#else
	pos_int++;
	token_local_ptr = token_local;
	while (oper_plan[index][pos_int] != '\0')
		*token_local_ptr++ = oper_plan[index][pos_int++];
	*token_local_ptr = '\0';
	pos_local_jit = strchr(token_local, '[');
	extract_array_name_index(name_local, size_local, token_local, pos_local_jit);
	assign_array_sim(name_local, size_local);
#endif
}