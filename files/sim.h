#pragma once
#include <map>
#include <string>
#include <fstream>
#include <cmath>
#include <cstdint>
#include "globals.h"

// НУЖНО ПОДСЧИТЫВАТЬ ПРОМАХИ ОТДЕЛЬНО ПО ЗАПИСИ И ЧТЕНИЮ
// В т.ч. П ОКАЖДОМУ МАССИВУ В ОТДЕЛЬНОСТИ

class Sim {
 public:
	 Sim();
	 Sim(const std::string& config_line);
	 ~Sim();
	 void map_init_sim(const char* str);
	 void trace_handler(int64_t cur_adr, const char* array_name, const char* access_mode, const char* func)	;
	 void sim_init();
	 void print_results();
	 // доступ к соседним уровням
	 Sim* prev() const { return prev_level_; }
	 Sim* next() const { return next_level_; }

	 void set_prev(Sim* p) { prev_level_ = p; }
	 void set_next(Sim* p) { next_level_ = p; }
 private:
	 Sim* prev_level_ = nullptr;
	 Sim* next_level_ = nullptr;
	 std::string level_name;
	 std::uint64_t cacheSize;;
	 std::uint64_t cacheBlockSize;
	 std::uint64_t way;
	 static constexpr char level_name_string[] = "level_name";
	 static constexpr char cacheSize_string[] = "cacheSize";
	 static constexpr char cacheBlockSize_string[] = "cacheBlockSize";
	 static constexpr char way_string[] = "way";
	 std::uint64_t indexSize;
	 std::uint64_t bitsOfIndex;
	 std::uint64_t numSets;
	 std::uint64_t numSets_minus_1;
	 std::uint64_t offSet;
	 std::uint64_t bitsOfTag;
	 std::uint64_t miss = 0;
	 std::uint64_t miss_write = 0;
	 std::uint64_t linenum = 0;
	 std::uint64_t cur_miss = 0;
	 std::uint64_t miss_flag = 0;
	 std::uint64_t index = 0;
	 std::uint64_t hit = 0;
	 std::uint64_t hit_write = 0;
	 std::uint64_t programTimer = 0;
	 std::uint64_t memory_writes = 0;
	 std::uint64_t memory_reads = 0;
	 int cache_line_size;
	 int offsetBits;
	 int offsetMask;
	 static constexpr int VALUES_ADDRESSES_TO_FILE = 0;
	 static constexpr int CACHE_LINES_ADDRESSES_TO_FILE = 1;
	 int traces_to_file_mode = CACHE_LINES_ADDRESSES_TO_FILE;
	 int traces_to_file = 0;
	 // первый индекс - номер набора по ассоциативности(прямое отображение) (например, 0 - 63)
 // второй индекс - строка в наборе(например 0 - 7 при 8 - way),
	 std::uint64_t** cacheTable;
	 std::uint64_t** validTable;
	 std::uint64_t* validTableValues; // Храним количество занятых строк
	 std::uint64_t** dirtyTable;		  // Биты "dirty" - совпадает ли с памятью / верхним уровнем кеша
									      // Для полностью инклюзивного кеша данный бит не нужен
	 std::uint64_t** timeTable;
#ifdef MAP_SIM
	 std::map<std::string, int64_t> arrays; // массив - число промахлв
	 std::map<std::string, int64_t> arrays_write; // массив - число промахлв по записи 
#else
	 std::int32_t arrays_num = 0;
	 struct array_type_sim arrays[NUM_LOCAL_ARRAYS];	// массив - число промахлв
	 std::int64_t arrays_write[NUM_LOCAL_ARRAYS]; // массив - число промахлв по записи 
#endif
	 std::ofstream trace;
	 void init_cache();
	 bool simulator(int64_t cur_adr_a, const char* str, int write_mode); //int num_els_in_cache_line)
	 void init_fields();
};


