//#define _CRT_SECURE_NO_WARNINGS // Ќ”∆Ќќ ѕќ“ќћ Ѕ”ƒ≈“ ”Ѕ–ј“№

#include <cstdio> 
#include <csetjmp>
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

char* prog = nullptr;
jmp_buf e_buf;
struct var_type global_vars[NUM_GLOBAL_VARS];
struct array_type global_arrays[NUM_GLOBAL_ARRAYS];
struct var_type local_var_stack[NUM_LOCAL_VARS];
struct array_type local_array_stack[NUM_LOCAL_ARRAYS];
struct func_type func_table[NUM_FUNC];
struct func_type func_stack[NUM_FUNC];
struct var_array_stack call_stack[NUM_FUNC];
char token[MAX_TOKEN_LENGTH];
char token_type = 0;
char tok = 0;
int functos = 0;
int func_index = 0;
int gvar_index = 0;
int garray_index = 0;
int lvartos = 0;
int larraytos = 0;
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
struct commands table[] = { /* Commands must be entered lowercase */
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
int not_rekurs_eval_exp0_sim = 1;
int in_operator = 0;
char oper_plan[MAX_OPERATORS_IN_CYCLE][MAX_OPERATOR_LENGTH];
int index_in_oper_plan = 0;
int oper_num = 0;
char oper[MAX_OPERATORS_IN_CYCLE][MAX_OPERATOR_LENGTH];
int tokens_read[MAX_OPERATORS_IN_CYCLE];
int token_read_num = 0;
int tokens_read_length[MAX_OPERATORS_IN_CYCLE][MAX_TOKENS_IN_OPERATOR];
int tokens_write_length[MAX_OPERATORS_IN_CYCLE];
char* operator_start = nullptr;

int first_iter = 1;

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

	if (setjmp(e_buf)) exit(1); /* initialize long jump buffer */

	gvar_index = 0;  /* initialize global variable index */
	// ƒќЅј¬»Ћ
	garray_index = 0;  /* initialize global массивы index */
	//

	/* set program pointer to start of program buffer */
	prog = p_buf;
	prescan(); /* find the location of all functions
				  and global variables in the program */

	lvartos = 0;     /* initialize local variable stack index */
	// ƒќЅј¬»Ћ
	larraytos = 0;     /* initialize local массивы stack index */
	start_address_arrays = 0;
	// 
	functos = 0;     /* initialize the CALL stack index */
	break_occurring = 0; /* initialize the break occurring flag */

	/* setup call to main() */
	prog = find_func((char *)"main"); /* find program starting point */

	if (!prog) { /* incorrect or missing main() function in program */
		printf("main() not found.\n");
		exit(1);
	}

	prog--; /* back up to opening ( */
		
	my_strcpy_s(token, 80, "main");

	call(); /* call main() to start interpreting */

	// Test секци€
	/*/
	int* c_test = matr_mul_int();
	int* c_interp = (int*)local_array_stack[2].adr; //  ќ—“џЋ№
	if (!compare_int(c_interp, c_test, 25))
		printf("TST OK!!!");
		*/

	return 0;
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
		token_type = get_token();

		/* If interpreting single statement, return on
		   first semicolon.
		*/

		/* see what kind of token is up */
		if (token_type == IDENTIFIER) {	
			in_operator = 1;
			/* Not a keyword, so process expression. */
			putback();  /* restore token to input stream for
						   further processing by eval_exp() */
			// !!!!!!!!!!!!!!!!!
			// «ƒ≈—№ ƒЋя ”— ќ–≈Ќ»я  ћќƒ≈Ћ»–¬ќјЌ»я Ќ≈ќЅ’ќƒ»ћќ —ƒ≈Ћј“№ ƒ–”√ќ… —“≈  eval_exp
			// !!!!!!!!!!!!!!!!!
#ifdef FAST_SIMULATOR		// ѕќ ј "быстра€" —»ћ”Ћя÷»я только в это мрежиме
							// значит только циклы
			if (in_cycle && first_iter)
			{
#endif
#ifdef FAST_SIMULATOR
				in_operator = 1;
				if (not_rekurs_eval_exp0_sim && first_iter)
				{
#ifdef NUMBER_OPERATORS
					//get_token();
					operator_start = prog;
					//putback();
#else
				
					// Ќужно прочитать оператор целиком				
					//putback();
					char* temp = prog;
					char* temp_oper = oper[oper_num];
					while (*temp != ';')
						*temp_oper++ = *temp++;
					*temp_oper = '\0';
					//printf("%s\n", oper);
					//get_token();				
#endif
				}
#ifdef FAST_SIMULATOR		// ѕќ ј "быстра€" —»ћ”Ћя÷»я только в это мрежиме
							// значит только циклы
			}
#endif
			if (in_cycle)
			{
				if (first_iter)
					eval_exp_sim();
				else
					eval_exp_sim_jit();
			}
			else
				eval_exp(&value, 1);
#else
			eval_exp(&value, 1);  /* process the expression */
#endif
			if (*token != ';') sntx_err(SEMI_EXPECTED);
#ifdef FAST_SIMULATOR		// ѕќ ј "быстра€" —»ћ”Ћя÷»я только в это мрежиме
							// значит только циклы
			if (in_cycle && first_iter)
			{
#endif
#ifdef FAST_SIMULATOR
				in_operator = 0;
				//printf("%s\n", oper[oper_num]);
				//printf("%s\n", oper_plan[oper_num]);
#ifdef NUMBER_OPERATORS
				// помечаем оператор
				* operator_start++ = 'o';
				 snprintf(operator_start, 10, "%d; ", oper_num);
				//_itoa(oper_num, operator_start, 10);
#endif
				oper_num++;
				index_in_oper_plan = 0;			
#endif
#ifdef FAST_SIMULATOR		// ѕќ ј "быстра€" —»ћ”Ћя÷»я только в это мрежиме
							// значит только циклы	
			}
#endif
		}
		else if (token_type == BLOCK) { /* if block delimiter */
			if (*token == '{') /* is a block */
				block = 1; /* interpreting block, not statement */
			else return; /* is a }, so return */
		}
		else /* is keyword */
			switch (tok) {
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
	} while (tok != FINISHED && block);
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
	} while (!feof(fp) && i < PROG_SIZE);

	if (*(p - 2) == 0x1a) *(p - 2) = '\0'; /* null terminate the program */
	else *(p - 1) = '\0';
	fclose(fp);
	return 1;
}

/* Find the location of all functions in the program
   and store global variables. */
void prescan(void)
{
	char* p, * tp;
	char temp[ID_LEN + 1];
	int datatype;
	int brace = 0;  /* When 0, this var tells us that
					   current source position is outside
					   of any function. */

	p = prog;
	func_index = 0;
	do {
		while (brace) {  /* bypass code inside functions */
			get_token();
			if (*token == '{') brace++;
			if (*token == '}') brace--;
		}

		tp = prog; /* save current position */
		get_token();
		/* global var type or function return type */
		if (tok == CHAR || tok == INT) {
			datatype = tok; /* save data type */
			get_token();
			if (token_type == IDENTIFIER) {
				my_strcpy_s(temp, ID_LEN + 1, token);
				get_token();
				if (*token != '(') { /* must be global var */
					prog = tp; /* return to start of declaration */
					decl_global();
				}
				else if (*token == '(') {  /* must be a function */
					func_table[func_index].loc = prog;
					func_table[func_index].ret_type = datatype;
					my_strcpy_s(func_table[func_index].func_name, ID_LEN, temp);
					func_index++;
					while (*prog != ')') prog++;
					prog++;
					/* now prog points to opening curly
					   brace of function */
				}
				else putback();
			}
		}
		else if (*token == '{') brace++;
	} while (tok != FINISHED);
	prog = p;
}

/* Return the entry point of the specified function.
   Return NULL if not found.
*/


void* extract_array_decl(const char* name, char* pos, int vartype, char* token_temp, int* size, int* sizeofop)
{
	void* adr = NULL;
	char array_name[ID_LEN + 1];
	char array_size[ID_LEN + 1];
	extract_array_name_index(array_name, array_size, token_temp, pos);
	my_strcpy_s((char *)name, ID_LEN, array_name);
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


/* Declare a global variable */  // »Ћ» ћј——»¬
void decl_global(void)
{
	int vartype;

	get_token();  /* get type */

	vartype = tok; /* save var type */

	do { /* process comma-separated list */
		// ƒќЅј¬»Ћ - »«ћ≈Ќ»Ћ
		get_token();  /* get name */
		char* pos;
		char token_temp[ID_LEN + 1];
		my_strcpy_s(token_temp, ID_LEN, token);
		if (pos = strchr(token_temp, '['))
		{
			// ћј——»¬			
			global_arrays[garray_index].a_type = vartype;
			int size, sizeofop;
			global_arrays[garray_index].adr = extract_array_decl(global_arrays[garray_index].array_name, pos, vartype, token_temp, &size, &sizeofop);
			global_arrays[garray_index].size = size;
			global_arrays[garray_index].sizeofop = sizeofop;
			global_arrays[garray_index].start_address = start_address_arrays;
			start_address_arrays += size * sizeofop;
			garray_index++;
#ifdef SIMULATOR
			cache.map_init_sim(global_arrays[garray_index].array_name);
#endif
		}
		else
		{
			// ѕ≈–≈ћ≈ЌЌјя
			global_vars[gvar_index].v_type = vartype;
			global_vars[gvar_index].value = 0;  /* init to 0 */
			my_strcpy_s(global_vars[gvar_index].var_name, ID_LEN, token);
			gvar_index++;
		}
		// 
		get_token();

	} while (*token == ',');
	if (*token != ';') sntx_err(SEMI_EXPECTED);
}

/* Declare a local variable. */
void decl_local(void)
{
	struct var_type i;
	// ƒќЅј¬»Ћ
	struct array_type a;
	//
	get_token();  /* get type */

	i.v_type = tok;
	i.value = 0;  /* init to 0 */
	// ƒќЅј¬»Ћ
	a.a_type = tok;
	my_strcpy_s(a.array_name, ID_LEN, "");
	a.adr = (void *)tok;
	//
	do { /* process comma-separated list */
		get_token(); /* get var name */
		// »«ћ≈Ќ»Ћ
		char* pos;
		char token_temp[ID_LEN + 1];
		my_strcpy_s(token_temp, ID_LEN, token);
		if (pos = strchr(token_temp, '['))
		{
			// ћј——»¬
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
			// ѕ≈–≈ћ≈ЌЌјя
			my_strcpy_s(i.var_name, ID_LEN, token);
			local_push(i);
		}
		//

		get_token();
	} while (*token == ',');
	if (*token != ';') sntx_err(SEMI_EXPECTED);
}

/* Push the arguments to a function onto the local
   variable stack. */
void get_args(void)
{
	int value, count, temp[NUM_PARAMS];
	struct var_type i;

	count = 0;
	get_token();
	if (*token != '(') sntx_err(PAREN_EXPECTED);

	/* process a comma-separated list of values */
	do {
		eval_exp(&value, 1);
		temp[count] = value;  /* save temporarily */
		get_token();
		count++;
	} while (*token == ',');
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

	i = lvartos - 1;
	do { /* process comma-separated list of parameters */
		get_token();
		p = &local_var_stack[i];
		if (*token != ')') {
			if (tok != INT && tok != CHAR)
				sntx_err(TYPE_EXPECTED);

			p->v_type = token_type;
			get_token();

			/* link parameter name with argument already on
			   local var stack */
			my_strcpy_s(p->var_name, ID_LEN, token);
			get_token();
			i--;
		}
		else break;
	} while (*token == ',');
	if (*token != ')') sntx_err(PAREN_EXPECTED);
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
	if (lvartos >= NUM_LOCAL_VARS) {
		sntx_err(TOO_MANY_LVARS);
	}
	else {
		local_var_stack[lvartos] = i;
		//vars[i.var_name] = lvartos;
		lvartos++;
	}
}

/* Push a local массив. */
void local_push_array(struct array_type a)
{
	if (larraytos >= NUM_LOCAL_ARRAYS) {
		sntx_err(TOO_MANY_LARRAYS);
	}
	else {
		local_array_stack[larraytos] = a;
		//arrays[a.array_name] = larraytos;
		larraytos++;
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
	else if (functos >= NUM_FUNC) {
		sntx_err(NEST_FUNC);
	}
	else {
		index = call_stack[functos];
	}

	return index;
}

/* Push index of local variable stack. */
void func_push(int vars, int arrays)
{
	if (functos >= NUM_FUNC) {
		sntx_err(NEST_FUNC);
	}
	else {
		call_stack[functos].vars = vars;
		call_stack[functos].arrays = arrays;
		functos++;
	}
}

// ƒќЅј¬»Ћ
void* find_array_addr(char* name)
{
	int i;
	for (i = larraytos - 1; i >= call_stack[functos - 1].arrays; i--)
		if (!strcmp(local_array_stack[i].array_name, name))
			return local_array_stack[i].adr;
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

		if (tok != ELSE) {
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
	char* temp;

	break_occurring = 0; /* clear the break flag */
	putback();
	temp = prog;  /* save location of top of while loop */
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
	prog = temp;  /* loop back to top */
}

/* Execute a do loop. */
void exec_do(void)
{
	int cond;
	char* temp;

	putback();
	temp = prog;  /* save location of top of do loop */
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
	if (tok != WHILE) sntx_err(WHILE_EXPECTED);
	eval_exp(&cond, 1); /* check the loop condition */
	if (cond) prog = temp; /* if true loop; otherwise,
							 continue on */
}

/* Find the end of a block. */
void find_eob(void)
{
	int brace;

	get_token();
	brace = 1;
	do {
#ifdef NUMBER_OPERATORS
		if (*prog == '{') brace++;
		else if (*prog == '}') brace--;
		prog++;
#else
		get_token();
		if (*token == '{') brace++;
		else if (*token == '}') brace--;
#endif
	} while (brace);
}

/* Execute a for loop. */
void exec_for(void)
{
	int cond;
	char* temp, * temp2;
	int brace;

	if (!in_cycle)
	{
		/// !!!!!!!!!!!!!!!!!
		/// «десь устанавливаем переменуую "перва€ итераци€ цикла"
		first_iter = 1;			// ѕќ ј Ё“ќ  ќ—“џЋ№ - Ќ≈ ќЅ–јЅј“џ¬јё“—я 
		// ¬Ћќ∆≈ЌЌџ≈ ÷» Ћџ хот€ может и правльно ?
		/// !!!!!!!!!!!!!!!!!
		oper_num = 0;
		tokens_read[oper_num] = 0;
		token_read_num = 0;

	}
	in_cycle++;
	break_occurring = 0; /* clear the break flag */
	get_token();
	eval_exp(&cond, 1);  /* initialization expression */
	if (*token != ';') sntx_err(SEMI_EXPECTED);
	prog++; /* get past the ; */
	temp = prog;	
	for (;;) {
		eval_exp(&cond, 1);  /* check the condition */
		if (*token != ';') sntx_err(SEMI_EXPECTED);
		prog++; /* get past the ; */
		temp2 = prog;
		// Ќ≈ !!! ƒќЅј¬»Ћ ”—Ћќ¬»я — ћј——»¬јћ»
		/* find the start of the for block */
		brace = 1;
		while (brace) {
			get_token();
			if (*token == '(') brace++;
			if (*token == ')') brace--;
		}

		if (cond) {
			interp_block();  /* if true, interpret */
			/// !!!!!!!!!!!!!!!!!
			/// «десь сбрасываем переменуую "перва€ итераци€ цикла"
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
			// ѕ–ќЅЋ≈ћј «ƒ≈—№
			find_eob();
			// !!!!!!!!!!!!!!!!
			// «десь унитожаем все структуры, св€занные с циклом
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
		prog = temp2;
		eval_exp(&cond, 1); /* do the increment */
		prog = temp;  /* loop back to top */
	}
}
