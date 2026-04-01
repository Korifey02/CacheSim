//#define _CRT_SECURE_NO_WARNINGS // НУЖНО ПОТОМ БУДЕТ УБРАТЬ

#include <cstdio> 
#include <cmath>
#include <ctype.h>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include "globals.h"
#include "interp.h"
#include "parser.h"
#include "cache_memory.h"
#include "test.h"

using namespace std;

char* G_PROGRAM_POINTER = nullptr;
struct var_type G_GLOBAL_VARS_STORAGE[SETTINGS_NUM_GLOBAL_VARS];
struct array_type G_GLOBAL_ARRAYS_STORAGE[SETTINGS_NUM_GLOBAL_ARRAYS];
struct var_type G_STACK_FOR_LOCAL_VARS[SETTINGS_NUM_LOCAL_VARS];
struct array_type G_STACK_FOR_LOCAL_ARRAYS[SETTINGS_NUM_LOCAL_ARRAYS];
struct func_type G_FUNC_TABLE[SETTINGS_NUM_FUNC];
struct var_array_stack G_CALL_STACK[SETTINGS_NUM_FUNC];
char G_TOKEN_BUFFER[SETTINGS_MAX_TOKEN_LENGTH]; // одна переменная-буфер для одного текущего токена
char G_CURRENT_TOKEN_TYPE = 0;
char G_CURRENT_TOKEN = 0;
int functos = 0;
int func_index = 0;
int G_VAR_INDEX = 0;
int G_ARRAY_INDEX = 0;
int G_STACK_TOP_FOR_LOCAL_VARS = 0;
int G_STACK_TOP_FOR_LOCAL_ARRAYS = 0;
int ret_value = 0;
int ret_occurring = 0;
int break_occurring = 0;
int start_address_arrays = 0;
int in_cycle = 0;
struct intern_func_type intern_func[] = {
	{ "getche", call_getche },
	{ "putch", call_putch },
	{ "puts", call_puts },
	{ "print", print },
	{ "getnum", getnum },
	{ "", 0 } /* null terminate the list */
};
struct commands G_KEYWORD_TOKEN_TYPE_TABLE[] = { /* Commands must be entered lowercase */
{ "if", IF }, /* in this table. */
{ "else", ELSE },
{ "for", FOR },
{ "do", DO },
{ "while", WHILE },
{ "char", CHAR },
{ "int", INT },
{ "return", RETURN },
{ "continue", CONTINUE },
{ "break", BREAK },
{ "end", END },
{ "", END } /* mark end of table */
};
int DEBUG_COUNTER = 0;
int total_reads = 0;
bool G_SIM_MODE = false;
int not_rekurs_eval_exp0_sim = 1;
int in_operator = 0;
char oper_plan[SETTINGS_MAX_OPERATORS_IN_CYCLE][SETTINGS_MAX_OPERATOR_LENGTH];
int index_in_oper_plan = 0;
int oper_num = 0;
char oper[SETTINGS_MAX_OPERATORS_IN_CYCLE][SETTINGS_MAX_OPERATOR_LENGTH];
int tokens_read[SETTINGS_MAX_OPERATORS_IN_CYCLE];
int token_read_num = 0;
int tokens_read_length[SETTINGS_MAX_OPERATORS_IN_CYCLE][SETTINGS_MAX_TOKENS_IN_OPERATOR];
int tokens_write_length[SETTINGS_MAX_OPERATORS_IN_CYCLE];
char* operator_start = nullptr;

int first_iter = 1;

// Токенизированный поток
std::vector<TokenInfo> g_token_stream;
int g_token_pos = 0;
bool g_use_source_directly = false;

//std::map<std::string, std::uint32_t> arrays;
//std::map<std::string, std::uint32_t> vars;
std::map<std::string, int> var_values;

int entry_interp(int argc, char* argv[])
{

	/* load the program to execute */
	if (!load_program(p_buf, argv[1]))
	{
		printf("Error loading program!\n");
		exit(1);
	}

	try {
		G_VAR_INDEX = 0;  /* initialize global variable index */
		// ДОБАВИЛ
		G_ARRAY_INDEX = 0;  /* initialize global массивы index */
		//

		// Initialize virtual addresses before prescan so locals continue after globals.
		start_address_arrays = 0;
		//
		/* set program pointer to start of program buffer */
		G_PROGRAM_POINTER = p_buf;

		/* Предварительная токенизация всего исходного текста */
		tokenize_source();
		g_token_pos = 0;

		prescan(); /* find the location of all functions
					  and global variables in the program */

		// Верхушка стека локальных переменных — сколько переменных сейчас на стеке
		G_STACK_TOP_FOR_LOCAL_VARS = 0;     /* initialize local variable stack index */
		// ДОБАВИЛ
		// То же для локальных массивов
		G_STACK_TOP_FOR_LOCAL_ARRAYS = 0;     /* initialize local массивы stack index */
		// Глубина стека вызовов функций
		functos = 0;     /* initialize the CALL stack index */
		// Флаг что встретился break — сейчас не активен
		break_occurring = 0; /* initialize the break occurring flag */

		/* setup call to main() */
		int main_idx = find_func((char *)"main"); /* find program starting point */

		if (main_idx < 0) { /* incorrect or missing main() function in program */
			printf("main() not found.\n");
			exit(1);
		}

		g_token_pos = main_idx - 1; /* back up to opening ( */
		
		my_strcpy_s(G_TOKEN_BUFFER, 80, "main");

		call(); /* call main() to start interpreting */
	}
	catch (const SyntaxError&) {
		return 1;
	}

	return 0;
}

/* Освобождает память, выделенную под глобальные массивы. */
void cleanup_global_arrays(void)
{
	for (int i = 0; i < G_ARRAY_INDEX; i++) {
		free(G_GLOBAL_ARRAYS_STORAGE[i].adr);
		G_GLOBAL_ARRAYS_STORAGE[i].adr = nullptr;
	}
}

/* Interpret a single statement or block of code. When
   interp_block() returns from its initial call, the final
   brace (or a return) in main() has been encountered.
*/
void interp_block(void)
{
	int value;
	char block = 0;

	do {
		G_CURRENT_TOKEN_TYPE = get_token();

		/* If interpreting single statement, return on
		   first semicolon.
		*/

		/* see what kind of token is up */
		if (G_CURRENT_TOKEN_TYPE == IDENTIFIER) {
			bool use_fast_sim_statement = false;
			int stmt_token_idx = g_token_pos - 1; /* индекс токена-идентификатора */
			in_operator = 1;
			if (in_cycle) {
#ifdef FAST_SIMULATOR
				/* На повторных итерациях: проверяем, помечен ли токен как JIT */
				if (!first_iter && g_token_stream[stmt_token_idx].jit_op_index >= 0) {
					use_fast_sim_statement = true;
				}
				else
#endif
				{
					char name_local[SETTINGS_ID_LEN + 1] = { 0 };
					char size_local[SETTINGS_ID_LEN + 1] = { 0 };
					char token_temp[SETTINGS_ID_LEN + 1] = { 0 };
					my_strcpy_s(token_temp, SETTINGS_ID_LEN, G_TOKEN_BUFFER);
					char* pos_local = strchr(token_temp, '[');
					if (pos_local != nullptr) {
						extract_array_name_index(name_local, size_local, token_temp, pos_local);
						use_fast_sim_statement = ::is_array(name_local) != 0;
					}
				}
			}

			/* Not a keyword, so process expression. */
			putback();  /* restore token to input stream for
						   further processing by eval_exp() */
			// !!!!!!!!!!!!!!!!!
			// ЗДЕСЬ ДЛЯ УСКОРЕНИЯ  МОДЕЛИРВОАНИЯ НЕОБХОДИМО СДЕЛАТЬ ДРУГОЙ СТЕК eval_exp
			// !!!!!!!!!!!!!!!!!
#ifdef FAST_SIMULATOR
			if (use_fast_sim_statement)
			{
				if (first_iter)
				{
					in_operator = 1;
					G_SIM_MODE = true;
					eval_exp(&value, 1);
					G_SIM_MODE = false;
				}
				else
				{
					int op_idx = g_token_stream[stmt_token_idx].jit_op_index;
					if (op_idx >= 0) {
						eval_exp_sim_jit(op_idx);
					} else {
						// Оператор в условной ветке, не пройденной на первой итерации —
						// JIT-план не записан, трассируем через sim-mode eval_exp
						G_SIM_MODE = true;
						eval_exp(&value, 1);
						G_SIM_MODE = false;
					}
				}
			}
			else
				eval_exp(&value, 1);
#else
			eval_exp(&value, 1);  /* process the expression */
#endif
			if (*G_TOKEN_BUFFER != ';') sntx_err(SEMI_EXPECTED);
#ifdef FAST_SIMULATOR
			if (in_cycle && first_iter)
			{
				in_operator = 0;
				if (use_fast_sim_statement) {
					/* Помечаем токен для JIT вместо модификации source */
					g_token_stream[stmt_token_idx].jit_op_index = oper_num;
					oper_num++;
					index_in_oper_plan = 0;
				}
			}
#endif
		}
		else if (G_CURRENT_TOKEN_TYPE == BLOCK) { /* if block delimiter */
			if (*G_TOKEN_BUFFER == '{') /* is a block */
				block = 1; /* interpreting block, not statement */
			else return; /* is a }, so return */
		}
		else /* is keyword */
			switch (G_CURRENT_TOKEN) { // todo тут получается не хендлятся типы float double char
			case FLOAT:
			case DOUBLE:
			case CHAR:
			case INT:     /* declare local variables */
				putback();
				decl_local();
				break;
			case RETURN:  /* return from function call */
				func_ret();
				ret_occurring = 1;
				return;
			case CONTINUE:  /* continue loop execution */
				return;
			case BREAK:  /* break loop execution */
				break_occurring = 1;
				return;
			case IF:      /* process an if statement */
				exec_if();
				if (ret_occurring > 0 || break_occurring > 0) {
					return;
				}
				break;
			case ELSE:    /* process an else statement */
				find_eob(); /* find end of else block and continue execution */
				break;
			case WHILE:   /* process a while loop */
				exec_while();
				if (ret_occurring > 0) {
					return;
				}
				break;
			case DO:      /* process a do-while loop */
				exec_do();
				if (ret_occurring > 0) {
					return;
				}
				break;
			case FOR:     /* process a for loop */
				/*
				if (DEBUG_COUNTER == 1)
					for (int i = 0; i < 10; i++)
					{
						int *ip = (int*)local_array_stack[0].adr;
						printf("%d\n", *(ip + i));
					}
				DEBUG_COUNTER++;
				*/

				exec_for();
				if (ret_occurring > 0) {
					return;
				}
				break;
			case END:
				exit(0);
			}
	} while (G_CURRENT_TOKEN != FINISHED && block);
}

/* Load a program. */
int load_program(char* p, char* fname)
{
	FILE* fp;
	int i;

#ifdef UNIX
	fp = std::fopen(fname, "rb");
	if (fp == NULL) return 0;
#else
	if (fopen_s(&fp, fname, "rb") != 0 || fp == NULL) return 0;
#endif
	

	i = 0;
	do {
		*p = (char)getc(fp);
		p++; i++;
	} while (!feof(fp) && i < SETTINGS_PROG_SIZE);

	if (i >= SETTINGS_PROG_SIZE && !feof(fp)) {
		fclose(fp);
		printf("\nprogram too large (max %d bytes)\n", SETTINGS_PROG_SIZE);
		return 0;
	}

	if (*(p - 2) == 0x1a) *(p - 2) = '\0'; /* null terminate the program */
	else *(p - 1) = '\0';
	fclose(fp);
	return 1;
}

/* Find the location of all functions in the program
   and store global variables. */
void prescan(void)
{
	int saved_pos;
	int tp_idx;
	char temp_identifier_name[SETTINGS_ID_LEN + 1]; // temp storage for var name
	int remember_current_token;
	int opened_brace_counter = 0;  /* When 0, this var tells us that
					   current source position is outside
					   of any function. */

	saved_pos = g_token_pos;
	func_index = 0;
	do {
		while (opened_brace_counter) {  /* bypass code inside functions */
			get_token();
			if (*G_TOKEN_BUFFER == '{') opened_brace_counter++;
			if (*G_TOKEN_BUFFER == '}') opened_brace_counter--;
		}

		tp_idx = g_token_pos; /* save current position */
		get_token();
		/* global var type or function return type */
		if (G_CURRENT_TOKEN == CHAR || G_CURRENT_TOKEN == INT) {
			remember_current_token = G_CURRENT_TOKEN; /* save data type */
			get_token();
			if (G_CURRENT_TOKEN_TYPE == IDENTIFIER) {
				my_strcpy_s(temp_identifier_name, SETTINGS_ID_LEN + 1, G_TOKEN_BUFFER);
				get_token();
				if (*G_TOKEN_BUFFER != '(') { /* must be global var */
					g_token_pos = tp_idx; /* return to start of declaration */
					decl_global();
				}
				else if (*G_TOKEN_BUFFER == '(') {  /* must be a function */
					if (func_index >= SETTINGS_NUM_FUNC)
						sntx_err(TOO_MANY_FUNCS);
					G_FUNC_TABLE[func_index].token_index = g_token_pos;
					G_FUNC_TABLE[func_index].ret_type = remember_current_token;
					my_strcpy_s(G_FUNC_TABLE[func_index].func_name, SETTINGS_ID_LEN, temp_identifier_name);
					func_index++;
					/* skip tokens to ')' */
					while (*G_TOKEN_BUFFER != ')') get_token();
					/* now prog points to opening curly
					   brace of function */
				}
				else putback();
			}
		}
		else if (*G_TOKEN_BUFFER == '{') opened_brace_counter++;
	} while (G_CURRENT_TOKEN != FINISHED);
	g_token_pos = saved_pos;
}

/* Return the entry point of the specified function.
   Return NULL if not found.
*/


void* extract_array_decl(const char* name, char* pos, int vartype, char* token_temp, int* size, int* sizeofop)
{
	void* adr = NULL;
	char array_name[SETTINGS_ID_LEN + 1];
	char array_size[SETTINGS_ID_LEN + 1];
	extract_array_name_index(array_name, array_size, token_temp, pos);
	my_strcpy_s((char *)name, SETTINGS_ID_LEN, array_name);
	int i = atoi(array_size);
	*size = i;
	switch (vartype)
	{
	case CHAR:
		adr = (char*)malloc(i * sizeof(char));
		*sizeofop = sizeof(char);
		break;
	case INT:
		adr = (int*)malloc(i * sizeof(int));
		*sizeofop = sizeof(int);
		break;
	case FLOAT:
		adr = (float*)malloc(i * sizeof(float));
		*sizeofop = sizeof(float);
		break;
	case DOUBLE:
		adr = (double*)malloc(i * sizeof(double));
		*sizeofop = sizeof(double);
		break;
	}
	return adr;
}


/* Declare a global variable */  // ИЛИ МАССИВ
void decl_global(void) // todo с ней пока не раскуриливал
{
	int vartype;

	get_token();  /* get type */

	vartype = G_CURRENT_TOKEN; /* save var type */

	do { /* process comma-separated list */
		// ДОБАВИЛ - ИЗМЕНИЛ
		get_token();  /* get name */
		char* pos;
		char token_temp[SETTINGS_ID_LEN + 1];
		my_strcpy_s(token_temp, SETTINGS_ID_LEN, G_TOKEN_BUFFER);
		if (pos = strchr(token_temp, '['))
		{
			// МАССИВ			
			if (G_ARRAY_INDEX >= SETTINGS_NUM_GLOBAL_ARRAYS)
				sntx_err(TOO_MANY_GARRAYS);
			G_GLOBAL_ARRAYS_STORAGE[G_ARRAY_INDEX].a_type = vartype;
			int size, sizeofop;
			G_GLOBAL_ARRAYS_STORAGE[G_ARRAY_INDEX].adr = extract_array_decl(G_GLOBAL_ARRAYS_STORAGE[G_ARRAY_INDEX].array_name, pos, vartype, token_temp, &size, &sizeofop);
			G_GLOBAL_ARRAYS_STORAGE[G_ARRAY_INDEX].size = size;
			G_GLOBAL_ARRAYS_STORAGE[G_ARRAY_INDEX].sizeofop = sizeofop;
			G_GLOBAL_ARRAYS_STORAGE[G_ARRAY_INDEX].start_address = start_address_arrays;
			start_address_arrays += size * sizeofop;
			G_ARRAY_INDEX++;
#ifdef SIMULATOR
			cache.map_init_sim(global_arrays[G_ARRAY_INDEX - 1].array_name);
#endif
		}
		else
		{
			// ПЕРЕМЕННАЯ
			if (G_VAR_INDEX >= SETTINGS_NUM_GLOBAL_VARS)
				sntx_err(TOO_MANY_GVARS);
			G_GLOBAL_VARS_STORAGE[G_VAR_INDEX].v_type = vartype;
			G_GLOBAL_VARS_STORAGE[G_VAR_INDEX].value = 0;  /* init to 0 */
			my_strcpy_s(G_GLOBAL_VARS_STORAGE[G_VAR_INDEX].var_name, SETTINGS_ID_LEN, G_TOKEN_BUFFER);
			G_VAR_INDEX++;
		}
		// 
		get_token();

	} while (*G_TOKEN_BUFFER == ',');
	if (*G_TOKEN_BUFFER != ';') sntx_err(SEMI_EXPECTED);
}

/* Declare a local variable. */
void decl_local(void)
{
	struct var_type i;
	// ДОБАВИЛ
	struct array_type a;
	//
	get_token();  /* get type */

	i.v_type = G_CURRENT_TOKEN;
	i.value = 0;  /* init to 0 */
	// ДОБАВИЛ
	a.a_type = G_CURRENT_TOKEN;
	my_strcpy_s(a.array_name, SETTINGS_ID_LEN, "");
	a.adr = (void *)G_CURRENT_TOKEN; // заглушка, т.к. еще не знаем массив это или нет
	//
	do { /* process comma-separated list */
		get_token(); /* get var name */
		// ИЗМЕНИЛ
		char* pos;
		char token_temp[SETTINGS_ID_LEN + 1];
		my_strcpy_s(token_temp, SETTINGS_ID_LEN, G_TOKEN_BUFFER);
		if (pos = strchr(token_temp, '['))
		{
			// МАССИВ
			int size, sizeofop;
			a.adr = extract_array_decl(a.array_name, pos, a.a_type, token_temp, &size, &sizeofop);
			a.size = size;
			a.sizeofop = sizeofop;
			a.start_address = start_address_arrays;
			start_address_arrays += size * sizeofop;
			local_push_array(a);
#ifdef SIMULATOR
			cache.map_init_sim(a.array_name);
#endif
		}
		else
		{
			// ПЕРЕМЕННАЯ
			my_strcpy_s(i.var_name, SETTINGS_ID_LEN, G_TOKEN_BUFFER);
			local_push(i);
		}
		//

		get_token();
	} while (*G_TOKEN_BUFFER == ',');
	if (*G_TOKEN_BUFFER != ';') sntx_err(SEMI_EXPECTED);
}

/* Push the arguments to a function onto the local
   variable stack. */
void get_args(void)
{
	int value, count, temp[SETTINGS_NUM_PARAMS];
	struct var_type i;

	count = 0;
	get_token();
	if (*G_TOKEN_BUFFER != '(') sntx_err(PAREN_EXPECTED);

	/* process a comma-separated list of values */
	do {
		eval_exp(&value, 1);
		if (count >= SETTINGS_NUM_PARAMS)
			sntx_err(TOO_MANY_PARAMS);
		temp[count] = value;  /* save temporarily */
		get_token();
		count++;
	} while (*G_TOKEN_BUFFER == ',');
	count--;
	/* now, push on local_var_stack in reverse order */
	for (; count >= 0; count--) {
		i.value = temp[count];
		i.v_type = ARG;
		local_push(i);
	}
}

/* Get function parameters. */
void get_params(void)
{
	struct var_type* p;
	int i;

	i = G_STACK_TOP_FOR_LOCAL_VARS - 1;
	do { /* process comma-separated list of parameters */
		get_token();
		p = &G_STACK_FOR_LOCAL_VARS[i];
		if (*G_TOKEN_BUFFER != ')') {
			if (G_CURRENT_TOKEN != INT && G_CURRENT_TOKEN != CHAR)
				sntx_err(TYPE_EXPECTED);

			p->v_type = G_CURRENT_TOKEN_TYPE;
			get_token();

			/* link parameter name with argument already on
			   local var stack */
			my_strcpy_s(p->var_name, SETTINGS_ID_LEN, G_TOKEN_BUFFER);
			get_token();
			i--;
		}
		else break;
	} while (*G_TOKEN_BUFFER == ',');
	if (*G_TOKEN_BUFFER != ')') sntx_err(PAREN_EXPECTED);
}

/* Return from a function. */
void func_ret(void)
{
	int value;

	value = 0;
	/* get return value, if any */
	eval_exp(&value, 1);

	ret_value = value;
}

/* Push a local variable. */
void local_push(struct var_type i)
{
	if (G_STACK_TOP_FOR_LOCAL_VARS >= SETTINGS_NUM_LOCAL_VARS) {
		sntx_err(TOO_MANY_LVARS);
	}
	else {
		G_STACK_FOR_LOCAL_VARS[G_STACK_TOP_FOR_LOCAL_VARS] = i;
		//vars[i.var_name] = lvartos;
		G_STACK_TOP_FOR_LOCAL_VARS++;
	}
}

/* Push a local массив. */
void local_push_array(struct array_type a)
{
	if (G_STACK_TOP_FOR_LOCAL_ARRAYS >= SETTINGS_NUM_LOCAL_ARRAYS) {
		sntx_err(TOO_MANY_LARRAYS);
	}
	else {
		G_STACK_FOR_LOCAL_ARRAYS[G_STACK_TOP_FOR_LOCAL_ARRAYS] = a;
		//arrays[a.array_name] = larraytos;
		G_STACK_TOP_FOR_LOCAL_ARRAYS++;
	}
}

/* Pop index into local variable stack. */
struct var_array_stack func_pop(void)
{
	struct var_array_stack index = { 0, 0 };
	functos--;
	if (functos < 0) {
		sntx_err(RET_NOCALL);
	}
	else if (functos >= SETTINGS_NUM_FUNC) {
		sntx_err(NEST_FUNC);
	}
	else {
		index = G_CALL_STACK[functos];
	}

	return index;
}

/* Push index of local variable stack. */
void func_push(int vars, int arrays)
{
	if (functos >= SETTINGS_NUM_FUNC) {
		sntx_err(NEST_FUNC);
	}
	else {
		G_CALL_STACK[functos].vars = vars;
		G_CALL_STACK[functos].arrays = arrays;
		functos++;
	}
}

// ДОБАВИЛ
void* find_array_addr(char* name)
{
	int i;
	for (i = G_STACK_TOP_FOR_LOCAL_ARRAYS - 1; i >= G_CALL_STACK[functos - 1].arrays; i--)
		if (!strcmp(G_STACK_FOR_LOCAL_ARRAYS[i].array_name, name))
			return G_STACK_FOR_LOCAL_ARRAYS[i].adr;
}

/* Execute an if statement. */
void exec_if(void)
{
	int cond;

	eval_exp(&cond, 1); /* get if expression */

	if (cond) { /* is true so process target of IF */
		interp_block();
	}
	else { /* otherwise skip around IF block and
		   process the ELSE, if present */
		find_eob(); /* find start of next line */
		get_token();

		if (G_CURRENT_TOKEN != ELSE) {
			putback();  /* restore token if
						   no ELSE is present */
			return;
		}
		interp_block();
	}
}

/* Execute a while loop. */
void exec_while(void)
{
	int cond;
	int saved_pos;

	break_occurring = 0; /* clear the break flag */
	putback();
	saved_pos = g_token_pos;  /* save location of top of while loop */
	get_token();
	eval_exp(&cond, 1);  /* check the conditional expression */
	if (cond) {
		interp_block();  /* if true, interpret */
		if (break_occurring > 0) {
			break_occurring = 0;
			return;
		}
	}
	else {  /* otherwise, skip around loop */
		find_eob();
		return;
	}
	g_token_pos = saved_pos;  /* loop back to top */
}

/* Execute a do loop. */
void exec_do(void)
{
	int cond;
	int saved_pos;

	putback();
	saved_pos = g_token_pos;  /* save location of top of do loop */
	break_occurring = 0; /* clear the break flag */

	get_token(); /* get start of loop */
	interp_block(); /* interpret loop */
	if (ret_occurring > 0) {
		return;
	}
	else if (break_occurring > 0) {
		break_occurring = 0;
		return;
	}
	get_token();
	if (G_CURRENT_TOKEN != WHILE) sntx_err(WHILE_EXPECTED);
	eval_exp(&cond, 1); /* check the loop condition */
	if (cond) g_token_pos = saved_pos; /* if true loop; otherwise,
						 continue on */
}

/* Find the end of a block. */
void find_eob(void)
{
	int brace;

	get_token();
	brace = 1;
	do {
		get_token();
		if (*G_TOKEN_BUFFER == '{') brace++;
		else if (*G_TOKEN_BUFFER == '}') brace--;
	} while (brace);
}

/* Execute a for loop. */
void exec_for(void)
{
	int loop_condition_value;
	int condition_pos, increment_pos;
	int opened_brace_counter;

	if (!in_cycle)
	{
		/// !!!!!!!!!!!!!!!!!
		/// Здесь устанавливаем переменуую "первая итерация цикла"
		first_iter = 1;			// ПОКА ЭТО КОСТЫЛЬ - НЕ ОБРАБАТЫВАЮТСЯ
		// ВЛОЖЕННЫЕ ЦИКЛЫ хотя может и правльно ?
		/// !!!!!!!!!!!!!!!!!
		oper_num = 0;
		tokens_read[oper_num] = 0;
		token_read_num = 0;

	}
	in_cycle++;
	break_occurring = 0; /* clear the break flag */
	get_token();
	eval_exp(&loop_condition_value, 1);  /* initialization expression */
	if (*G_TOKEN_BUFFER != ';') sntx_err(SEMI_EXPECTED);
	g_token_pos++; /* get past the ; */
	condition_pos = g_token_pos;
	for (;;) {
		eval_exp(&loop_condition_value, 1);  /* check the condition */
		if (*G_TOKEN_BUFFER != ';') sntx_err(SEMI_EXPECTED);
		g_token_pos++; /* get past the ; */
		increment_pos = g_token_pos;
		// НЕ !!! ДОБАВИЛ УСЛОВИЯ С МАССИВАМИ
		/* find the start of the for block */
		opened_brace_counter = 1;
		while (opened_brace_counter) {
			get_token();
			if (*G_TOKEN_BUFFER == '(') opened_brace_counter++;
			if (*G_TOKEN_BUFFER == ')') opened_brace_counter--;
		}

		if (loop_condition_value) {
			interp_block();  /* if true, interpret */
			/// !!!!!!!!!!!!!!!!!
			/// Здесь сбрасываем переменуую "первая итерация цикла"
			first_iter = 0;
			/// !!!!!!!!!!!!!!!!!
			if (ret_occurring > 0) {
				return;
			}
			else if (break_occurring > 0) {
				break_occurring = 0;
				return;
			}
		}
		else {  /* otherwise, skip around loop */
			// ПРОБЛЕМА ЗДЕСЬ
			find_eob();
			// !!!!!!!!!!!!!!!!
			// Здесь унитожаем все структуры, связанные с циклом
			// !!!!!!!!!!!!!!!!
			in_cycle--;
			/*
			if (!in_cycle)
				for (int i = 0; i < oper_num; i++)
				{
					printf("%s\n", oper[i]);
					printf("%s\n", oper_plan[i]);
					//printf("%s - %d\n", oper[oper_num], strlen(oper[oper_num]));
					//printf("%s - %d\n", oper_plan[oper_num], strlen(oper_plan[oper_num]));
				}
			*/
			return;
		}
		g_token_pos = increment_pos;
		eval_exp(&loop_condition_value, 1); /* do the increment */
		g_token_pos = condition_pos;  /* loop back to top */
	}
}
