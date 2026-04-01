#pragma once
#include <stdexcept>
#include <fstream>
#include <map>
#include <vector>
#include <cstdint>
#include <stdint.h>


#include "loclib.h"

using namespace std;

//#define UNIX		// ��� ���������� � Unix

//#define SIMULATOR	// ��� "������" ��������� ��������� � ������ ��������������
#ifndef SIMULATOR
#define FAST_SIMULATOR  // ��� "�������" ��������� ��� ������������� �������������
#endif
#ifdef FAST_SIMULATOR
#define NUMBER_OPERATORS  // �������� ��������� � ���� �� �� ������
#endif
//#define TRACES

#define MAP_SIM 

#define SETTINGS_NUM_FUNC        100
#define SETTINGS_NUM_GLOBAL_VARS 100
#define SETTINGS_NUM_LOCAL_VARS  200
// �������
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

#define NEW_M   // ����� ��������� ������� ����� ������� 



/*
// Secure function compatibility
#if !defined(_MSC_VER) || _MSC_VER < 1400
#define strcpy_s(dest, count, source) strncpy( (dest), (source), (count) )
#define fopen_s(pFile,filename,mode) (((*(pFile))=fopen((filename),(mode)))==NULL)
#endif
*/
enum TokenType {
	DELIMITER, // 0
	IDENTIFIER, // 1
	NUMBER, // 2
	KEYWORD, // 3
	TEMP, // 4
	STRING, // 5
	BLOCK, // 6
	ARRAY // ������� // 7
};

/* add additional C keyword tokens here */
enum Token {
	ARG, // 0
	CHAR,// 1
	INT,// 2
	IF,// 3
	ELSE,// 4
	FOR,// 5
	DO,// 6
	WHILE,// 7
	SWITCH,// 8
	RETURN,// 9
	CONTINUE,// 10
	BREAK,// 11
	EOL,// 12
	FINISHED,// 13
	END,// 14
	// �������
	DOUBLE,// 15
	FLOAT// 16
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
	TOO_MANY_LVARS, TOO_MANY_LARRAYS, DIV_BY_ZERO,
	TOO_MANY_GVARS, TOO_MANY_GARRAYS, TOO_MANY_FUNCS,
	TOO_MANY_PARAMS, PROG_TOO_LARGE
};

// Исключение для синтаксических ошибок (замена longjmp)
class SyntaxError : public std::runtime_error {
public:
	explicit SyntaxError(int error_code)
		: std::runtime_error("syntax error"), code(error_code) {}
	int code;
};

extern char* G_PROGRAM_POINTER;    /* current location in source code */
extern char* p_buf;   /* points to start of program buffer */

/* An array of these structures will hold the info
   associated with global variables.
*/
struct var_type {
	char var_name[SETTINGS_ID_LEN];
	int v_type;
	int value;
};
extern struct var_type G_GLOBAL_VARS_STORAGE[];
// �������
struct array_type {
	char array_name[SETTINGS_ID_LEN];
	int a_type;
	void* adr;
	int sizeofop;
	int size;
	int start_address;
};
extern struct array_type G_GLOBAL_ARRAYS_STORAGE[];
////////////
extern struct var_type G_STACK_FOR_LOCAL_VARS[];
extern struct array_type G_STACK_FOR_LOCAL_ARRAYS[];

struct array_type_sim {
	char array_name[SETTINGS_ID_LEN];
	std::int64_t num;
};

struct func_type {
	char func_name[SETTINGS_ID_LEN];
	int ret_type;
	int token_index;  /* position in token stream */
};

// Токен для предварительно токенизированного потока
struct TokenInfo {
	char token_type;     // DELIMITER, IDENTIFIER, NUMBER, etc.
	char token;          // keyword token (IF, WHILE...) or 0
	char text[SETTINGS_MAX_TOKEN_LENGTH];
	char* source_pos;    // position in source (for sntx_err line counting)
	int jit_op_index;    // FAST_SIMULATOR: JIT operator plan index, -1 if not replaced
};
extern struct func_type G_FUNC_TABLE[];

struct intern_func_type {
	const char* f_name; /* function name */
	int(*p)(void);   /* pointer to the function */
};
extern struct intern_func_type intern_func[];

struct var_array_stack {
	int vars;
	int arrays;
};
extern struct var_array_stack G_CALL_STACK[];
// �������� ����
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
// �������
extern int G_ARRAY_INDEX; /* index into global ������� table */
//
extern int G_STACK_TOP_FOR_LOCAL_VARS; /* index into local variable stack */
// �������
extern int G_STACK_TOP_FOR_LOCAL_ARRAYS; /* index into local array stack */
//

extern int ret_value; /* function return value */
extern int ret_occurring; /* function return is occurring */
extern int break_occurring; /* loop break is occurring */
// �������
extern int start_address_arrays;
//
extern struct var_array_stack func_pop(void);
extern int DEBUG_COUNTER;
extern int total_reads;
extern int in_cycle;
extern std::map<std::string, int> var_values;

extern std::vector<TokenInfo> g_token_stream;
extern int g_token_pos;
extern bool g_use_source_directly;  // for eval_array_index_expression fallback

extern bool G_SIM_MODE;
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



