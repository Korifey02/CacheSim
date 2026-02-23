#include <cstring>
#include "globals.h"
#include "parser.h"
#include "cache_memory.h"

char oper_local[MAX_OPERATOR_LENGTH];
char name_local[ID_LEN + 1];
char size_local[ID_LEN + 1];
char* pos_local;
char* token_l;
char temp_c;
char token_local[ID_LEN + 1];
int temp_i;
int pos_int;

void eval_exp_sim_jit()
{	
#ifdef NUMBER_OPERATORS
	//get_token();
	prog++; // буква 'o'
	int index = std::strtol(prog, &pos_local, 10);
	prog = pos_local; 
	prog++; // символ ';'
	//printf("%d\n", index);
	while (*prog != ';')
	{
	//	printf("%c", *prog);
		prog++;
	}
	get_token();
#else
	// считываем оператор
	char* temp_oper = oper_local;
	while (*prog != ';')
		*temp_oper++ = *prog++;
	*temp_oper = '\0';
	get_token();   // считываем ;
	// ищем в массиве
	int index = 0;
	while (strcmp(oper_local, oper[index])) index++;
#endif
	// выполняем план под номером index	
	// сначала чтения	
#ifdef NEW_M
	token_l= oper_plan[index];
	for (int i = 0; i < tokens_read[index]; i++) // цикл по токенам чтения
	{
		temp_c = token_l[tokens_read_length[index][i]];
		token_l[tokens_read_length[index][i]] = '\0';
		pos_local = strchr(token_l, '[');
		extract_array_name_index(name_local, size_local, token_l, pos_local);
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
		pos_local = strchr(token_local, '[');		
		extract_array_name_index(name_local, size_local, token_local, pos_local);
		//printf("%s %s \n", name_local, size_local);
		// Верим, считаем, что все массивы объявлены
		find_array_sim(name_local, size_local);
	}
#endif	
	// теперь запись
#ifdef NEW_M
	pos_local = strchr(token_l, '[');
	extract_array_name_index(name_local, size_local, token_l, pos_local);
	assign_array_sim(name_local, size_local);
#else	
	pos_int++;
	token_local_ptr = token_local;
	while (oper_plan[index][pos_int] != '\0')
		*token_local_ptr++ = oper_plan[index][pos_int++];
	*token_local_ptr = '\0';
	pos_local = strchr(token_local, '[');
	extract_array_name_index(name_local, size_local, token_local, pos_local);
	assign_array_sim(name_local, size_local);
#endif
}

/// !!! УБРАТЬ ВЕЗДЕ VALUE
/* Вход в парсер.*/
void eval_exp_sim()
{
	get_token();
	if (!*token) {
		sntx_err(NO_EXP);
		return;
	}
	if (*token == ';') { /* пустое выражение (оператор) */
		return;
	}
	eval_exp0_sim();
	putback(); /* возвращает последний считанный токен в поток ввода */
}

// ИЗМЕНИТЬ ДЕСЬ - ДОБАВИТЬ ОБРАБОТКУ МАССИВОВ
/* Обработка присваивания */
void eval_exp0_sim()
{
	char temp[ID_LEN];  /* holds name of var receiving
						   the assignment */
	char temp_tok;
	bool is_array_token = false;

	if (token_type == IDENTIFIER) {

		char name[ID_LEN + 1];
		char size[ID_LEN + 1];
		char* pos;
		char token_temp[ID_LEN + 1];

		

		my_strcpy_s(token_temp, ID_LEN, token);
		if (pos = strchr(token_temp, '['))
		{
			extract_array_name_index(name, size, token_temp, pos);
			is_array_token = ::is_array(name);
		}
		if (is_var(token) || is_array_token) {  /* if a var, see if assignment */
			my_strcpy_s(temp, ID_LEN, token);
			temp_tok = token_type;
			// сейчас temp - token (переменная, куда присываиваем), 
			// is_array_token - токен является смассивом
			// name - название массива, size - индекс			
			// temp_tok - тип токена, т.е. IDENTIFIER
			get_token();
			if (*token == '=') {  /* is an assignment */
				not_rekurs_eval_exp0_sim = 0;
				get_token();
				eval_exp0_sim();  /* вычисляем выражения в правой части */
				if (is_array_token) {
					// функции тут упростить
					assign_array_sim(name, size);  /* assign the value */
					if (first_iter)
					{
#ifdef NEW_M
						temp_i = strlen(temp);
						pos_local = &oper_plan[oper_num][index_in_oper_plan];
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
				//else  // Переменные пока не моделируем
					//assign_var_array(temp, a, 0, 0);  /* assign the value */
				not_rekurs_eval_exp0_sim = 1; 
				return;
			}
			else {  /* not an assignment */
				putback();  /* restore original token */
				my_strcpy_s(token, 80, temp);
				token_type = temp_tok;
			}
		}
	}
	eval_exp1_sim();
}

/* обработка условных операторов. */
void eval_exp1_sim()
{
	int partial_value;
	char op;
	char relops[7] = {
	  LT, LE, GT, GE, EQ, NE, 0
	};

	eval_exp2_sim();
	op = *token;
	if (strchr(relops, op)) {
		get_token();
		eval_exp2_sim();		
	}
}

/*  Сложение и вычитание. */
void eval_exp2_sim()
{
	char  op;
	int partial_value;

	eval_exp3_sim();
	while ((op = *token) == '+' || op == '-') {
		get_token();
		eval_exp3_sim();		
	}
}

/* Умножение и деление. */
void eval_exp3_sim()
{
	char  op;
	int partial_value, t;

	eval_exp4_sim();
	while ((op = *token) == '*' || op == '/' || op == '%') {
		get_token();
		eval_exp4_sim();		
	}
}

/* Унарный + или -. */
void eval_exp4_sim()
{
	char  op;

	op = '\0';
	if (*token == '+' || *token == '-') {
		op = *token;
		get_token();
	}
	eval_exp5_sim();	
}

/* Обработка выражений в скобках. */
void eval_exp5_sim()
{
	if (*token == '(') {
		get_token();
		eval_exp0_sim();   /* получить выражение в скобках */
		if (*token != ')') sntx_err(PAREN_EXPECTED);
		get_token();
	}
	else
		atom_sim();
}

/* определяется значение числа, перемноой, элемента массива или ызова функции. */
void atom_sim()
{
	int i;
	char name[ID_LEN + 1];
	char size[ID_LEN + 1];
	char token_temp[ID_LEN + 1];
	char* token_t = token;
	bool is_array_atom = 0;
	char* pos;

	switch (token_type) {
	case IDENTIFIER:
		//printf("%s ", token);	
		my_strcpy_s(token_temp, ID_LEN, token);
		is_array_atom = (pos = strchr(token_temp, '['));
		if (first_iter && is_array_atom)
		{
			while (*token_t != '\0')
				oper_plan[oper_num][index_in_oper_plan++] = *token_t++;
#ifndef NEW_M
			oper_plan[oper_num][index_in_oper_plan++] = DELIMITER_OPER_PLAN;
#endif
			tokens_read[oper_num]++;
			tokens_read_length[oper_num][token_read_num++] = strlen(token);
		}

		i = internal_func(token);
		if (i != -1) {  /* call "standard library" function */
			(*intern_func[i].p)();
		}
		else if (find_func(token)) { /* call user-defined function */
			call();
			ret_value;
		}
		else
		{						
			// Консутрукцию ниже можно упростить
			if (is_array_atom) {
				extract_array_name_index(name, size, token_temp, pos);
				// Верим, считаем, что все массивы объявлены
				find_array_sim(name, size);
			}
			//if (!is_var(token)) // Все делаем выше
			//	find_array_sim(name, size);				
			//else    // Переменные пока в моделирвоании не учитываем
			//	find_var_sim(token);				
		}
		get_token();
		return;
	case NUMBER: /* is numeric constant */
		get_token();
		return;
	case DELIMITER: /* see if character constant */
		if (*token == '\'') {
			prog++;
			if (*prog != '\'') sntx_err(QUOTE_EXPECTED);
			prog++;
			get_token();
			return;
		}
		if (*token == ')') return; /* process empty expression */
		else sntx_err(SYNTAX); /* syntax error */
	default:
		sntx_err(SYNTAX); /* syntax error */
	}
}

void find_array_sim(char* name, char* index)
{
	for (int i = larraytos - 1; i >= call_stack[functos - 1].arrays; i--) {
		if (!strcmp(local_array_stack[i].array_name, name)) {
			int index_value, token_type_temp;
			char temp[ID_LEN + 1];
			my_strcpy_s(temp, ID_LEN, token);
			my_strcpy_s(token, ID_LEN, index);
			token_type_temp = token_type;
			token_type = IDENTIFIER;
			char* prog_temp = prog;
			prog = index;
			char* p_zero = strchr(prog, '\0');
			*p_zero++ = ';';
			*p_zero = '\0';
			eval_exp(&index_value, 1);
			prog = prog_temp;
			token_type = token_type_temp;
			my_strcpy_s(token, ID_LEN, temp);
			/* ЧТЕНИЕ*/
			cache.trace_handler((local_array_stack[i].start_address + index_value * local_array_stack[i].sizeofop), local_array_stack[i].array_name, "r", "");
		}
	}
}

/* Присваивает значение элемнету массива. */
void assign_array_sim(char* array_name, char* index)
{
	int com = strcmp(array_name, "c");

	for (int i = larraytos - 1; i >= call_stack[functos - 1].arrays; i--) {
		if (!strcmp(local_array_stack[i].array_name, array_name)) {
			//////////////   ДОБАВИТЬ ТУТ   int index = 5;
			int index_value, token_type_temp;
			char temp[ID_LEN + 1];
			my_strcpy_s(temp, ID_LEN, token);
			my_strcpy_s(token, ID_LEN, index);
			token_type_temp = token_type;
			token_type = IDENTIFIER;

			char* prog_temp = prog;
			prog = index;
			char* p_zero = strchr(prog, '\0');
			*p_zero++ = ';';
			*p_zero = '\0';
			eval_exp(&index_value, 1);
			prog = prog_temp;
			token_type = token_type_temp;
			my_strcpy_s(token, ID_LEN, temp);
			/* ЗАПИСЬ*/
			//printf("%d\n", index_value);
			cache.trace_handler((local_array_stack[i].start_address + index_value * local_array_stack[i].sizeofop), local_array_stack[i].array_name, "w", "");
			//cache.trace_handler((arrays[array_name]->start_address + index_value * arrays[array_name]->sizeofop), arrays[array_name]->array_name, "w", "");
			return;
		}
	}

	// ДОБАВИТЬ  - Я ТУТ НЕ ПОИСКАЛ В ГЛОБАЛЬНЫХ МАССИВАХ
}

