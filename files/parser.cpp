/* Recursive descent parser for integer expressions
   which may include variables and function calls.
*/

#include <csetjmp>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include "globals.h"
#include "parser.h"
#include "interp.h"
#include "cache_memory.h"

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
	char temp_tok;

	if (G_CURRENT_TOKEN_TYPE == IDENTIFIER) {

		char name[SETTINGS_ID_LEN + 1];
		char size[SETTINGS_ID_LEN + 1];
		char* pos;
		char token_temp[SETTINGS_ID_LEN + 1];
		my_strcpy_s(token_temp, SETTINGS_ID_LEN, G_TOKEN_BUFFER);
		if (pos = strchr(token_temp, '['))
			extract_array_name_index(name, size, token_temp, pos);
		if (is_var(G_TOKEN_BUFFER) || ::is_array(name)) {  /* if a var, see if assignment */
			my_strcpy_s(temp, SETTINGS_ID_LEN, G_TOKEN_BUFFER);
			/*
			if (is_var(token))
				my_strcpy_s(temp, ID_LEN, token);
			else
				my_strcpy_s(temp, ID_LEN, name);
				*/
			temp_tok = G_CURRENT_TOKEN_TYPE;
			get_token();
			if (*G_TOKEN_BUFFER == '=') {  /* is an assignment */
				not_rekurs_eval_exp0_sim = 0;
				get_token();
				eval_exp0(value);  /* get value to assign */
				if (is_var(temp))
					assign_var_array(temp, *value, 0, 0);  /* assign the value */
				else
					assign_var_array(name, *value, 1, size);  /* assign the value */
				not_rekurs_eval_exp0_sim = 1;
				return;
			}
			else {  /* not an assignment */
				putback();  /* restore original token */
				my_strcpy_s(G_TOKEN_BUFFER, 80, temp);
				G_CURRENT_TOKEN_TYPE = temp_tok;
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

/*  Сложение и вычитание. */
void eval_exp2(int* value)
{
	char  op;
	int partial_value;

	eval_exp3(value);
	while ((op = *G_TOKEN_BUFFER) == '+' || op == '-') {
		get_token();
		eval_exp3(&partial_value);
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

/* Умножение и деление. */
void eval_exp3(int* value)
{
	char  op;
	int partial_value, t;

	eval_exp4(value);
	while ((op = *G_TOKEN_BUFFER) == '*' || op == '/' || op == '%') {
		get_token();
		eval_exp4(&partial_value);
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
	if (op)
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

/* определяется значение числа, перемноой, элемента массива или ызова функции. */
void atom(int* value)
{
	int i;
	char name[SETTINGS_ID_LEN + 1];
	char size[SETTINGS_ID_LEN + 1];
	char token_temp[SETTINGS_ID_LEN + 1];

	switch (G_CURRENT_TOKEN_TYPE) {
	case IDENTIFIER:
		i = internal_func(G_TOKEN_BUFFER);
		if (i != -1) {  /* call "standard library" function */
			*value = (*intern_func[i].p)();
		}
		else if (find_func(G_TOKEN_BUFFER)) { /* call user-defined function */
			call();
			*value = ret_value;
		}
		else
		{
			char* pos;
			my_strcpy_s(token_temp, SETTINGS_ID_LEN, G_TOKEN_BUFFER);
			if (pos = strchr(token_temp, '['))
				extract_array_name_index(name, size, token_temp, pos);
			if (is_var(G_TOKEN_BUFFER))
				*value = find_var_array(G_TOKEN_BUFFER, 0, 0);
			else
				*value = find_var_array(name, 1, size);
		}
		get_token();
		return;
	case NUMBER: /* is numeric constant */
		*value = atoi(G_TOKEN_BUFFER);
		get_token();
		return;
	case DELIMITER: /* see if character constant */
		if (*G_TOKEN_BUFFER == '\'') {
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
	int* ip;
	char* cp;
	float* fp;
	double* dp;
	int adr_to_file;

	for (i = larraytos - 1; i >= call_stack[functos - 1].arrays; i--) {
		if (!strcmp(local_array_stack[i].array_name, name)) {
			int index_value, token_type_temp;
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
			eval_exp(&index_value, 1);
			G_PROGRAM_POINTER = prog_temp;
			G_CURRENT_TOKEN_TYPE = token_type_temp;
			my_strcpy_s(G_TOKEN_BUFFER, SETTINGS_ID_LEN, temp);
#ifdef SIMULATOR
			/* ЧТЕНИЕ*/
			cache.trace_handler((local_array_stack[i].start_address + index_value * local_array_stack[i].sizeofop), local_array_stack[i].array_name, "r", "");
#endif
			switch (local_array_stack[i].a_type)
			{
			case CHAR:
				cp = (char*)local_array_stack[i].adr;
				return *(cp + index_value);
				break;
			case INT:
				ip = (int*)local_array_stack[i].adr;
				return *(ip + index_value);
				break;
			case FLOAT:
				fp = (float*)local_array_stack[i].adr;
				return *(fp + index_value);
				break;
			case DOUBLE:
				dp = (double*)local_array_stack[i].adr;
				return *(dp + index_value);
				break;
			}
		}
	}
}

/* Find the value of a variable. */
int find_var(char* s)
{
	int i;

	/* first, see if it's a local variable */
	for (i = lvartos - 1; i >= call_stack[functos - 1].vars; i--)
		if (!strcmp(local_var_stack[i].var_name, G_TOKEN_BUFFER))
			return local_var_stack[i].value;

	/* otherwise, try global vars */
	for (i = 0; i < SETTINGS_NUM_GLOBAL_VARS; i++)
		if (!strcmp(global_vars[i].var_name, s))
			return global_vars[i].value;

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

char* find_func(char* name)
{
	int i;

	for (i = 0; i < func_index; i++)
		if (!strcmp(name, G_FUNC_TABLE[i].func_name))
			return G_FUNC_TABLE[i].loc;

	return NULL;
}

/* Call a function. */
void call(void)
{
	char* loc, * temp;
	int lvartemp, larraytemp;

	loc = find_func(G_TOKEN_BUFFER); /* find entry point of function */
	if (loc == NULL)
		sntx_err(FUNC_UNDEF); /* function not defined */
	else {
		lvartemp = lvartos;  /* save local var stack index */
		larraytemp = larraytos;
		// НЕ !!! ДОБАВИЛ ПЕРЕДАЧУ ПАРАМЕТРОВ
		get_args();  /* get function arguments */
		temp = G_PROGRAM_POINTER; /* save return location */
		func_push(lvartemp, larraytemp);  /* save local var stack index */
		G_PROGRAM_POINTER = loc;  /* reset prog to start of function */
		ret_occurring = 0; /* P the return occurring variable */
		get_params(); /* load the function's parameters with the values of the arguments */
		interp_block(); /* interpret the function */
		ret_occurring = 0; /* Clear the return occurring variable */
		G_PROGRAM_POINTER = temp; /* reset the program pointer */
		struct var_array_stack av = func_pop(); /* reset the local var stack */
		lvartos = av.vars;
		larraytos = av.arrays;
	}
}


/* Get a token. */
char get_token(void)
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
	  "division by zero"
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

	longjmp(e_buf, 1); /* return to safe point */
}

/* Return a token to input stream. */
void putback(void)
{
	char* t;

	t = G_TOKEN_BUFFER;
	for (; *t; t++) G_PROGRAM_POINTER--;
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
	for (i = lvartos - 1; i >= call_stack[functos - 1].vars; i--)
		if (!strcmp(local_var_stack[i].var_name, s))
			return 1;

	/* otherwise, try global vars */
	for (i = 0; i < SETTINGS_NUM_GLOBAL_VARS; i++)
		if (!strcmp(global_vars[i].var_name, s))
			return 1;

	return 0;
}
// ДОБАВИЛ то же самое для массива
int is_array(char* s)
{
	int i;

	/* first, see if it's a local variable */
	for (i = larraytos - 1; i >= call_stack[functos - 1].arrays; i--)
		if (!strcmp(local_array_stack[i].array_name, s))
			return 1;

	/* otherwise, try global arrays */
	for (i = 0; i < SETTINGS_NUM_GLOBAL_ARRAYS; i++)
		if (!strcmp(global_arrays[i].array_name, s))
			return 1;

	return 0;
}

//ДОБАВИЛ
/* Присваивает значение элемнету массива. */
void assign_array(char* array_name, int value, char* index)
{
	int i;
	int* ip;
	char* cp;
	float* fp;
	double* dp;

	int i_debug;
	int* ip_debug;

	int com = strcmp(array_name, "c");

	for (i = larraytos - 1; i >= call_stack[functos - 1].arrays; i--) {
		if (!strcmp(local_array_stack[i].array_name, array_name)) {

			//printf("Found - %s\n", array_name);
			//i_debug = i;

			//////////////   ДОБАВИТЬ ТУТ   int index = 5;
			int index_value, token_type_temp;
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


			eval_exp(&index_value, 1);

			//if (!com)
			//	printf("DEBUG - array=%s index=%d\n", local_array_stack[i].array_name, index_value);

			//printf("Index - %d, value - %d\n", index_value, value);

			G_PROGRAM_POINTER = prog_temp;
			G_CURRENT_TOKEN_TYPE = token_type_temp;
			my_strcpy_s(G_TOKEN_BUFFER, SETTINGS_ID_LEN, temp);
#ifdef SIMULATOR
			/* ЗАПИСЬ*/
			cache.trace_handler((local_array_stack[i].start_address + index_value * local_array_stack[i].sizeofop), local_array_stack[i].array_name, "w", "");
#endif

			switch (local_array_stack[i].a_type)
			{
			case CHAR:
				cp = (char*)local_array_stack[i].adr;
				*(cp + index_value) = (char)value;
				break;
			case INT:
				ip = (int*)local_array_stack[i].adr;
				*(ip + index_value) = (int)value;
				break;
			case FLOAT:
				fp = (float*)local_array_stack[i].adr;
				*(fp + index_value) = (float)value;
				break;
			case DOUBLE:
				dp = (double*)local_array_stack[i].adr;
				*(dp + index_value) = (double)value;
				break;
			}

			//ip_debug = (int*)local_array_stack[i_debug].adr;
			//printf("Value - %d\n",  * (ip + index_value));

			return;
		}
	}

	// ДОБАВИТЬ  - Я ТУТ НЕ ПОИСКАЛ В ГЛОБАЛЬНЫХ МАССИВАХ
}

/* Assign a value to a variable. */
void assign_var(char* var_name, int value)
{
	int i;

	/* first, see if it's a local variable */
	for (i = lvartos - 1; i >= call_stack[functos - 1].vars; i--) {
		if (!strcmp(local_var_stack[i].var_name, var_name)) {
			local_var_stack[i].value = value;
			return;
		}
	}
	if (i < call_stack[functos - 1].vars)
		/* if not local, try global var table */
		for (i = 0; i < SETTINGS_NUM_GLOBAL_VARS; i++)
			if (!strcmp(global_vars[i].var_name, var_name)) {
				global_vars[i].value = value;
				return;
			}
	sntx_err(NOT_VAR); /* variable not found */
} 