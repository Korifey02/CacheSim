#pragma once
#include <csetjmp>
#include <csetjmp>
#include <fstream>
#include<map>
#include <cstdint>
#include <stdint.h>


#include "loclib.h"

using namespace std;

//#define UNIX		// Для компиляции в Unix

//#define SIMULATOR	// Для "долгой" симуляции совместно с полной интерпретацией
#ifndef SIMULATOR
#define FAST_SIMULATOR  // Для "быстрой" симуляции без полноценногой интерпретации
#endif
#ifdef FAST_SIMULATOR
#define NUMBER_OPERATORS  // нумеруем операторы и ищем их по номеру
#endif
//#define TRACES

#define MAP_SIM 

#define SETTINGS_NUM_FUNC        100
#define SETTINGS_NUM_GLOBAL_VARS 100
#define SETTINGS_NUM_LOCAL_VARS  200
// ДОБАВИЛ
#define SETTINGS_NUM_GLOBAL_ARRAYS 10
#define SETTINGS_NUM_LOCAL_ARRAYS  20
//
#define SETTINGS_NUM_BLOCK       100
#define SETTINGS_ID_LEN          32
#define SETTINGS_FUNC_CALLS      31
#define SETTINGS_NUM_PARAMS      31
#define SETTINGS_PROG_SIZE       10000
#define SETTINGS_LOOP_NEST       31
#define SETTINGS_FOR_NEST        31
#define SETTINGS_MAX_TOKEN_LENGTH 80
#define SETTINGS_MAX_OPERATOR_LENGTH 500
#define SETTINGS_MAX_OPERATORS_IN_CYCLE 50
#define SETTINGS_MAX_TOKENS_IN_OPERATOR 50
#define SETTINGS_DELIMITER_OPER_PLAN '\\'

#define NEW_M   // метод выделения токенов через массивы 



/*
// Secure function compatibility
#if !defined(_MSC_VER) || _MSC_VER < 1400
#define strcpy_s(dest, count, source) strncpy( (dest), (source), (count) )
#define fopen_s(pFile,filename,mode) (((*(pFile))=fopen((filename),(mode)))==NULL)
#endif
*/
enum TokenType {
	DELIMITER, IDENTIFIER, NUMBER, KEYWORD,
	TEMP, STRING, BLOCK, ARRAY // ДОБАВИЛ
};

/* add additional C keyword tokens here */
enum Token {
	ARG, CHAR, INT, IF, ELSE, FOR, DO, WHILE,
	SWITCH, RETURN, CONTINUE, BREAK, EOL, FINISHED, END,
	// ДОБАВИЛ
	DOUBLE, FLOAT
	//
};

/* add additional double operators here (such as ->) */
enum double_ops { LT = 1, LE, GT, GE, EQ, NE };

/* These are the constants used to call sntx_err() when
   a syntax error occurs. Add more if you like.
   NOTE: SYNTAX is a generic error message used when
   nothing else seems appropriate.
*/
enum error_msg
{
	SYNTAX, UNBAL_PARENS, NO_EXP, EQUALS_EXPECTED,
	NOT_VAR, PARAM_ERR, SEMI_EXPECTED,
	UNBAL_BRACES, FUNC_UNDEF, TYPE_EXPECTED,
	NEST_FUNC, RET_NOCALL, PAREN_EXPECTED,
	WHILE_EXPECTED, QUOTE_EXPECTED, NOT_TEMP,
	TOO_MANY_LVARS, TOO_MANY_LARRAYS, DIV_BY_ZERO
};

extern char* G_PROGRAM_POINTER;    /* current location in source code */
extern char* p_buf;   /* points to start of program buffer */
extern jmp_buf e_buf; /* hold environment for longjmp() */

/* An array of these structures will hold the info
   associated with global variables.
*/
struct var_type {
	char var_name[SETTINGS_ID_LEN];
	int v_type;
	int value;
};
extern struct var_type global_vars[];
// ДОБАВИЛ
struct array_type {
	char array_name[SETTINGS_ID_LEN];
	int a_type;
	void* adr;
	int sizeofop;
	int size;
	int start_address;
};
extern struct array_type global_arrays[];
////////////
extern struct var_type local_var_stack[];
extern struct array_type local_array_stack[];

struct array_type_sim {
	char array_name[SETTINGS_ID_LEN];
	std::int64_t num;
};

struct func_type {
	char func_name[SETTINGS_ID_LEN];
	int ret_type;
	char* loc;  /* location of entry point in file */
};
extern struct func_type G_FUNC_TABLE[];
extern struct func_type func_stack[];

struct intern_func_type {
	const char* f_name; /* function name */
	int(*p)(void);   /* pointer to the function */
};
extern struct intern_func_type intern_func[];

struct var_array_stack {
	int vars;
	int arrays;
};
extern struct var_array_stack call_stack[];
// ДОБАВИТЬ ТИПЫ
struct commands { /* keyword lookup table */
	char command[20];
	char tok;
};
extern struct commands G_KEYWORD_TOKEN_TYPE_TABLE[];

extern char G_TOKEN_BUFFER[];
extern char G_CURRENT_TOKEN_TYPE, G_CURRENT_TOKEN;

extern int functos;  /* index to top of function call stack */
extern int func_index; /* index into function table */
extern int G_VAR_INDEX; /* index into global variable table */
// ДОБАВИЛ
extern int G_ARRAY_INDEX; /* index into global массивы table */
//
extern int lvartos; /* index into local variable stack */
// ДОБАВИЛ
extern int larraytos; /* index into local array stack */
//

extern int ret_value; /* function return value */
extern int ret_occurring; /* function return is occurring */
extern int break_occurring; /* loop break is occurring */
// ДОБАВИЛ
extern int start_address_arrays;
//
extern struct var_array_stack func_pop(void);
extern int DEBUG_COUNTER;
extern int total_reads;
extern int in_cycle;
//extern std::map<std::string, std::uint32_t> arrays;
//extern std::map<std::string, std::uint32_t> vars;
extern std::map<std::string, int> var_values;

extern int not_rekurs_eval_exp0_sim;
extern int in_operator;
extern int oper_num;
extern char oper[SETTINGS_MAX_OPERATORS_IN_CYCLE][SETTINGS_MAX_OPERATOR_LENGTH];
extern int index_in_oper_plan;
extern char oper_plan[SETTINGS_MAX_OPERATORS_IN_CYCLE][SETTINGS_MAX_OPERATOR_LENGTH];
extern int first_iter;
extern int tokens_read[SETTINGS_MAX_OPERATORS_IN_CYCLE];
extern int token_read_num;
extern int tokens_read_length[SETTINGS_MAX_OPERATORS_IN_CYCLE][SETTINGS_MAX_TOKENS_IN_OPERATOR];
extern int tokens_write_length[SETTINGS_MAX_OPERATORS_IN_CYCLE];

extern char* operator_start;

int my_strcpy_s(char* dst, size_t dstsz, const char* src);

std::uint32_t find_index(struct array_type_sim* array, char* str, int32_t max_num);



