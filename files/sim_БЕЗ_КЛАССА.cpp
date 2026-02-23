#include<iostream>
#include<map>
#include <bitset>
#include "globals.h"
#include "sim.h"

// ЛУЧШЕ СЧИТЫВАТЬ ИЗ КОНФИГА
uint64_t const cacheSize = 32 * 1024;
uint64_t const cacheBlockSize = 64;
uint64_t const way = 8;
// Cache initialize : start
uint64_t indexSize = cacheSize / (cacheBlockSize * way);
uint64_t bitsOfIndex = (uint32_t)log2(cacheSize / (cacheBlockSize * way));
uint64_t numSets = cacheSize / (cacheBlockSize * way);
uint64_t numSets_minus_1 = numSets - 1;
uint64_t offSet = (uint32_t)log2(cacheBlockSize);
uint64_t bitsOfTag = 64 - bitsOfIndex - offSet;
uint64_t miss = 0;
uint64_t linenum = 0;
uint64_t cur_miss = 0;
uint64_t miss_flag = 0;
uint64_t index = 0;
uint64_t hit = 0;
uint64_t programTimer = 0;
const int cache_line_size = 64;
int offsetBits = log2(cache_line_size);
int offsetMask = cache_line_size - 1;
#define VALUES_ADDRESSES_TO_FILE 0
#define CACHE_LIBES_ADDRESSES_TO_FILE 1
int traces_to_file_mode = CACHE_LIBES_ADDRESSES_TO_FILE;
int traces_to_file = 0;

// первый индекс - номер набора по ассоциативности(прямое отображение) (например, 0 - 63)
// второй индекс - строка в наборе(например 0 - 7 при 8 - way),
uint64_t** cacheTable;
uint64_t** validTable;
uint64_t* validTableValues; // Храним количество занятых строк
uint64_t** timeTable;

map<std::string, int64_t> arrays; // массив - число промахлв

ofstream trace;

void map_init_sim(const char* str)
{
	arrays[str] = 0;  // Сначала по всем массивам ноль промахов
}

void init_cache()
{
	cacheTable = (uint64_t**)malloc(indexSize * sizeof(uint64_t*));
	for (int i = 0; i < indexSize; i++)
	{
		cacheTable[i] = (uint64_t*)malloc(way * sizeof(uint64_t));
		for (int j = 0; j < way; j++)
			cacheTable[i][j] = 0;
	}
	validTable = (uint64_t**)malloc(indexSize * sizeof(uint64_t*));
	for (int i = 0; i < indexSize; i++)
	{
		validTable[i] = (uint64_t*)malloc(way * sizeof(uint64_t));
		for (int j = 0; j < way; j++)
			validTable[i][j] = 0;
	}
	validTableValues = (uint64_t*)malloc(indexSize * sizeof(uint64_t));
	for (int j = 0; j < indexSize; j++)
		validTableValues[j] = 0;
	timeTable = (uint64_t**)malloc(indexSize * sizeof(uint64_t*));
	for (int i = 0; i < indexSize; i++)
	{
		timeTable[i] = (uint64_t*)malloc(way * sizeof(uint64_t));
		for (int j = 0; j < way; j++)
			timeTable[i][j] = 0;
	}
}

void simulator(int64_t cur_adr_a, const char* str, int num_els_in_cache_line)
{
	linenum++;
	miss_flag = 1;
	uint64_t tag = cur_adr_a >> (64 - bitsOfTag);
	//std::string s_temp;
	//stringstream temp;
	//temp << bitset<64>(cur_adr_a);
	//cout << temp.str() << endl;
	if (bitsOfIndex > 0) // НУЖНО ЛИ ЭТО УСЛОВИЕ ? Нужно тоько если будет полностью ассоциативный кеш
	{
		// Двигаем вправо на число разрядов смещения в строке
		// и выделяем биты номера множества
		index = (cur_adr_a >> offSet) & numSets_minus_1;
		/*
		// Здесь длина всегда 64, пожтому нет "коротких" и "длинных"
		s_temp = temp.str().substr(64 - offSet - bitsOfIndex, bitsOfIndex);
		bitset<6> bs(s_temp);
		index = (uint32_t)bs.to_ulong();

		if (index != index1)
			cout << temp.str() << " " << index << " " << index1 << endl;
			*/
			// Здесь получили tag index
			// Hit часть
		for (int i = 0; i < way; i++)
			if ((validTable[index][i] == 1) and (cacheTable[index][i] == tag))
			{
				hit += num_els_in_cache_line;
				//if (policyNUmber == 0) // LRU
				timeTable[index][i] = programTimer; // LRU ТОЛЬКО
				miss_flag = 0;
				break;
			}
		// Miss часть
		if (miss_flag == 1)
		{
			//#define MISSES_FILE
#ifdef MISSES_FILE
			cout << tag << ", " << index << ", " << cur_adr_a << ", " << str << ", " << programTimer << endl;
#endif
			arrays[str]++;   // фикструем промах в массиве
			miss += 1;
			hit += num_els_in_cache_line - 1;
			cur_miss += 1; // используется для схранения состояния кеша
			// LRU and FIFO
			// ЦИКЛЫ ВНИЗУ ДОЛГО ВЫПОЛНЯЮТСЯ ДЛЯ КАЖДОГО ПРОМАХА

			// МОЖНО ЗДЕСЬ КАКУЮ_ТО МАСКУ ПРИМЕНЯТЬ

			if (validTableValues[index] < way)
				for (int i = 0; i < way; i++)
					if (validTable[index][i] == 0)
					{
						cacheTable[index][i] = tag;
						timeTable[index][i] = programTimer;
						validTable[index][i] = 1;
						validTableValues[index]++;
						miss_flag = 0;
						break;
					}
			if (miss_flag == 1)
			{
				uint32_t min = timeTable[index][0];                         // find the oldest one
				uint32_t min_k = 0;
				for (int k = 0; k < way; k++)
					if (timeTable[index][k] < min)
					{
						min = timeTable[index][k];
						min_k = k;
					}
				cacheTable[index][min_k] = tag;                    // replace the oldest one(for LRU)
				timeTable[index][min_k] = programTimer;

				miss_flag = 0;
			}
			cur_miss = 0;
		}
	}

	programTimer++;
}

void trace_handler(int64_t cur_adr, const char* array_name, const char* access_mode, const char* func)
{
	int64_t cur_adr_to_sim = cur_adr & (~offsetMask);
	simulator(cur_adr_to_sim, array_name, 1); // Передаем по одному адресу в строке
	if (traces_to_file)
	{
		int64_t cur_adr_to_file = cur_adr;
		if (traces_to_file_mode == CACHE_LIBES_ADDRESSES_TO_FILE)
			cur_adr_to_file = cur_adr_to_sim;
		trace << hex << cur_adr_to_file << " " << array_name << endl;
	}
	//total_reads++;#
}

void sim_init()
{
	init_cache();
	if (traces_to_file)
		trace.open("trace");
}

void sim_destroy()
{
	if (traces_to_file)
		trace.close();
}

void print_results()
{
	cout << "Cache access: " << hit + miss << endl;
	cout << "Cache hit: " << hit << endl;
	cout << "Cache misses: " << miss << endl;
	cout << "Missrate: " << (double)miss / (double)(hit + miss) * 100 << endl;
	cout << "Cache misses array a: " << arrays["a"] << endl;
	cout << "Cache misses array b: " << arrays["b"] << endl;
	cout << "Cache misses array c: " << arrays["c"] << endl;
}



