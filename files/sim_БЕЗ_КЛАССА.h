#pragma once
void map_init_sim(const char* str);
void init_cache();
void simulator(int64_t cur_adr_a, const char* str, int num_els_in_cache_line);
void trace_handler(int64_t cur_adr, const char* array_name, const char* access_mode, const char* func);
void sim_init();
void sim_destroy();
void print_results();
void map_init_sim(const char* str);
