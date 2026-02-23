#pragma once
int entry_interp(int argc, char* argv[]);
void interp_block(void);
int load_program(char* p, char* fname);
void prescan(void);
void* extract_array_decl(const char* name, char* pos, int vartype, char* token_temp, int* size, int* sizeofop);
void decl_global(void);
void decl_local(void);
void get_args(void);
void get_params(void);
void func_ret(void);
void local_push(struct var_type i);
void local_push_array(struct array_type a);
struct var_array_stack func_pop(void);
void func_push(int vars, int arrays);
void* find_array_addr(char* name);
void exec_if(void);
void exec_while(void);
void exec_do(void);
void find_eob(void);
void exec_for(void);




