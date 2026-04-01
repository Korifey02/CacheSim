#pragma once
void eval_exp(int* value, int get_token_);
void eval_exp0(int* value);
void eval_exp1(int* value);
void eval_exp2(int* value);
void eval_exp3(int* value);
void eval_exp4(int* value);
void eval_exp5(int* value);
void atom(int* value);
int internal_func(char* s);
int is_var(char* s);
int is_array(char* s);
void assign_array(char* array_name, int value, int index_value);
void assign_var(char* var_name, int value);
void assign_var_array(char* var_name, int value, int is_array, int index_value);
int find_array(char* name, int index_value);
int find_var(char* s);
int find_var_array(char* name, int is_array, int index_value);
int find_func(char* name);
void call(void);
char get_token(void);
char get_token_from_source(void);
void tokenize_source(void);
char look_up(char* s);
int isdelim(char c);
int iswhite(char c);
static void str_replace(char* line, const char* search, const char* replace);
#if defined(_MSC_VER) && _MSC_VER >= 1200
__declspec(noreturn) void sntx_err(int error);
#elif __GNUC__
void sntx_err(int error) __attribute((noreturn));
#else
void sntx_err(int error);
#endif
void putback(void);
int is_var(char* s);
int is_array(char*);
