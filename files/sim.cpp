#include<iostream>
#include<map>
#include <bitset>
#include <sstream>
#include <cstring>
#include <stdexcept>
#include "globals.h"
#include "sim.h"

Sim::Sim()
	: level_name(level_name = "L1")
	, cacheSize(32 * 1024)
	, cacheBlockSize(64)
	, way(8)
{
	init_fields();
}

Sim::Sim(const std::string& config_line)
{
	level_name = "L1";
	cacheSize = 32 * 1024;
	cacheBlockSize = 64;
	way = 8;
	std::istringstream iss(config_line);
	std::string token;
	while (iss >> token) {
		auto pos = token.find('=');
		if (pos == std::string::npos) {
			// нет '=', можно игнорировать или ругаться
			continue;
		}

		std::string key = token.substr(0, pos);
		std::string value = token.substr(pos + 1);

		if (key == level_name_string) {
			level_name = value;
		}
		else if (key == cacheSize_string) {
			cacheSize = std::stoull(value);
		}
		else if (key == cacheBlockSize_string) {
			cacheBlockSize = std::stoull(value);
		}
		else if (key == way_string) {
			way = std::stoull(value);
		}
	}
	init_fields();
}


void Sim::init_fields()
{
	indexSize = cacheSize / (cacheBlockSize * way);
	bitsOfIndex = (std::uint32_t)log2(cacheSize / (cacheBlockSize * way));
	numSets = cacheSize / (cacheBlockSize * way);
	numSets_minus_1 = numSets - 1;
	offSet = (std::uint32_t)log2(cacheBlockSize);
	bitsOfTag = 64 - bitsOfIndex - offSet;
	cache_line_size = cacheBlockSize; // зачем это дублирующее определение
	offsetBits = static_cast<int>(std::log2(cache_line_size));
	offsetMask = cache_line_size - 1;
}

Sim::~Sim()
{
	if (traces_to_file)
		trace.close();
}

void Sim::map_init_sim(const char* str)
{
#ifdef MAP_SIM
	arrays[str] = 0;  // Сначала по всем массивам ноль промахов
	arrays_write[str] = 0;  // в т.ч. по записи
#else
	// Сначала по всем массивам ноль промахов
	my_strcpy_s((char*)arrays[arrays_num].array_name, ID_LEN, str);
	arrays[arrays_num].num = 0;
	// в т.ч. по записи
	arrays_write[arrays_num] = 0;
	arrays_num++;
#endif

}

void Sim::trace_handler(int64_t cur_adr, const char* array_name, const char* access_mode, const char* func)
{
	int64_t cur_adr_to_sim = cur_adr & (~offsetMask);
	int write_memory = (strcmp(access_mode, "w") == 0);
	if (!simulator(cur_adr_to_sim, array_name, write_memory)) // Передаем по одному адресу в строке
	{
		//Кеш инклюзивный, поэтому передаем запрос наверх для того,
		//  чтобы в случае, если на верхнем уровне не будет этих
		// данных, они также были подгружены
		if (next_level_ != nullptr)
			next_level_->trace_handler(cur_adr, array_name, access_mode, func);
		// Если же данный уровень последний, обращаемся к памяти
		else
			memory_reads++; // Т.к. промах верхнего уровня, считываем из памяти
	}
	if (traces_to_file)
	{
		int64_t cur_adr_to_file = cur_adr;
		if (traces_to_file_mode == CACHE_LINES_ADDRESSES_TO_FILE)
			cur_adr_to_file = cur_adr_to_sim;
		trace << hex << cur_adr_to_file << " " << array_name << endl;
	}
	//total_reads++;#
}
void Sim::sim_init()
{
	init_cache();
	if (traces_to_file) {
		std::string filename = "trace_" + level_name;
		trace.open(filename.c_str());
	}
}
void Sim::print_results()
{
	std::ostringstream oss;
	oss << "Cache " + level_name << '\n'
		<< "Cache size " << cacheSize / 1024 << " kB " << way << "-way" << '\n'
		<< "Cache line size " << cacheBlockSize << '\n'
		<< "Cache access: " << hit + miss << '\n'
		<< "Cache hit: " << hit << " (write - " << hit_write << " , read - " << (hit - hit_write) << ")" << '\n'
		<< "Cache misses: " << miss << " (write - " << miss_write << " , read - " << (miss - miss_write) << ")" << '\n'
		<< "Missrate: " << (double)miss / (double)(hit + miss) * 100 << '\n';
#ifdef MAP_SIM
	oss	<< "Cache misses array a: " << arrays["a"] << " (write - " << arrays_write["a"] << " , read - " << (arrays["a"] - arrays_write["a"]) << ")" << '\n'
		<< "Cache misses array b: " << arrays["b"] << " (write - " << arrays_write["b"] << " , read - " << (arrays["b"] - arrays_write["b"]) << ")" << '\n'
		<< "Cache misses array c: " << arrays["c"] << " (write - " << arrays_write["c"] << " , read - " << (arrays["c"] - arrays_write["c"]) << ")" << '\n';
#else
	for (int i = 0; i < arrays_num; i++)
		oss << "Cache misses array " << arrays[i].array_name << ": " << arrays[i].num << " (write - " << arrays_write[i] << " , read - " << (arrays[i].num - arrays_write[i]) << ")" << '\n';
#endif
	if (next_level_ == nullptr) // Смотрим, сколько всего нужно обудет сохранить в память
		// из последнего уровня после завершения программы
	{	// хотя при реальном сохранении необходимо данные поднимать с l1
		// в соответствии с битом dirty
		// но просто для подсчета такой подход может применяться
		for (int i = 0; i < indexSize; i++)
		 	for (int j = 0; j < way; j++)
				if (validTable[i][j] == 1)
					memory_writes++;
		oss << '\n'
			<< "Memory reads: " << memory_reads << '\n'
			<< "Memory writes: " << memory_writes << '\n';
	}
	oss << '\n';
	cout << oss.str();
}

void Sim::init_cache()
{
	cacheTable = (uint64_t**)malloc(indexSize * sizeof(uint64_t*));
	validTable = (uint64_t**)malloc(indexSize * sizeof(uint64_t*));
	validTableValues = (uint64_t*)malloc(indexSize * sizeof(uint64_t));
	timeTable = (uint64_t**)malloc(indexSize * sizeof(uint64_t*));
	dirtyTable = (uint64_t**)malloc(indexSize * sizeof(uint64_t*));
	for (int i = 0; i < indexSize; i++)
	{
		cacheTable[i] = (uint64_t*)malloc(way * sizeof(uint64_t));
		validTable[i] = (uint64_t*)malloc(way * sizeof(uint64_t));
		validTableValues[i] = 0;
		timeTable[i] = (uint64_t*)malloc(way * sizeof(uint64_t));
		dirtyTable[i] = (uint64_t*)malloc(way * sizeof(uint64_t));
		for (int j = 0; j < way; j++)
		{
			cacheTable[i][j] = 0;
			validTable[i][j] = 0;
			timeTable[i][j] = 0;
			dirtyTable[i][j] = 0;
		}
	}

	/*
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
	*/
}

bool Sim::simulator(int64_t cur_adr_a, const char* str, int write_mode) //int num_els_in_cache_line)
{
	linenum++;
	miss_flag = 1;
	int is_hit = 0;
	uint64_t tag = cur_adr_a >> (64 - bitsOfTag);
	if (bitsOfIndex > 0) // НУЖНО ЛИ ЭТО УСЛОВИЕ ? (Не?) Нужно тоько если будет полностью ассоциативный кеш
	{
		// Двигаем вправо на число разрядов смещения в строке
		// и выделяем биты номера множества
		index = (cur_adr_a >> offSet) & numSets_minus_1;
		// Здесь получили tag index
		// Hit часть
		for (int i = 0; i < way; i++)
			if ((validTable[index][i] == 1) and (cacheTable[index][i] == tag))
			{
				//hit += num_els_in_cache_line;
				hit++;
				if (write_mode)
					hit_write++;
				//if (policyNUmber == 0) // LRU
				timeTable[index][i] = programTimer; // LRU ТОЛЬКО
				if (!dirtyTable[index][i])   // Если еще не записывали
					dirtyTable[index][i] = write_mode; // Если операция - запись, то в бит записываем 1
				miss_flag = 0;
				is_hit = 1;
				break;
			}
		// Miss часть
		if (miss_flag == 1)
		{
			//cout << level_name << "- " << hex << tag << index << " " << str << endl;
#ifdef MAP_SIM
			arrays[str]++;   // фикструем промах в массиве
#else
			std::uint32_t index = find_index(arrays, (char*)str, arrays_num);
			arrays[index].num++;
#endif
			miss += 1;
			if (write_mode) {
				miss_write++;
#ifdef MAP_SIM
				arrays_write[str]++;
#else
 				arrays_write[index]++;
#endif
			}
			//hit += num_els_in_cache_line - 1;
			cur_miss += 1; // используется для схранения состояния кеша
			// LRU and FIFO
			// ЦИКЛЫ ВНИЗУ ДОЛГО ВЫПОЛНЯЮТСЯ ДЛЯ КАЖДОГО ПРОМАХА
			// МОЖНО ЗДЕСЬ КАКУЮ_ТО МАСКУ ПРИМЕНЯТЬ



			// Кеш инклюзивный, т.е. наверх не передаем
			// В первом цикле ищем свободное место
			if (validTableValues[index] < way)
				for (int i = 0; i < way; i++)
					if (validTable[index][i] == 0)
					{
						// Как будто обратились к вышестоящему уровню или
						// основной памятии поместили строку в свободное место
						// !!!!!!!!!!!!!!!!!!!!
						// НАДО ОБРАТИТЬСЯ !!!!!!!!!!!!!!!!!!
						// СЕЙЧАС ОБРАЩАЮСЬ ВО ВНЕШНЕЙ ФУНКЦИИ ВСЕГДА В СЛУЧАЕ ПРОМАХА
						// !!!!!!!!!!!!!!!!!!!!
						cacheTable[index][i] = tag;
						timeTable[index][i] = programTimer;
						validTable[index][i] = 1;
						//if (!dirtyTable[index][i])   // Если еще не записывали
						dirtyTable[index][i] = write_mode; // write-allocate Если операция - запись, то в бит записываем 1
						validTableValues[index]++;
						miss_flag = 0;
						break;
					}

			// В этом цикле уже ищем, что вытеснить
			// Надо проверить, если бит dirty=1, то надо передать
			// данные на уровень ввех или в память
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
				//if (!dirtyTable[index][min_k])   // Если еще не записывали
				dirtyTable[index][min_k] = write_mode; // write-allocate // Если операция - запись, то в бит записываем 1
				miss_flag = 0;
			}
			cur_miss = 0;
		}
	}
	programTimer++;
	return (is_hit == 1);
}
