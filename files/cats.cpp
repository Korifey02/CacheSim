#include <cstdio>
#include <iostream>
#include "globals.h"
#include "interp.h"
#include "cache_memory.h"
#include <chrono>

char* p_buf = nullptr;

int main(int argc, char* argv[])
{
	if (argc != 2) {
		printf("Usage: littlec <filename>\n");
		exit(1);
	}

	/* allocate memory for the program */
	if ((p_buf = (char*)malloc(SETTINGS_PROG_SIZE)) == NULL) {
		printf("Allocation Failure");
		exit(1);
	}

	auto start = std::chrono::high_resolution_clock::now();
	
	Sim& l1 = cache.add_sim("level_name=L1 cacheSize=32768 cacheBlockSize=64 way=8");
	Sim& l2 = cache.add_sim("level_name=L2 cacheSize=262144 cacheBlockSize=64 way=8");
	//Sim& l3 = cache.add_sim("level_name=L3 cacheSize=1048576 cacheBlockSize=64 way=16");
	
	cache.sim_init();
	entry_interp(argc, argv);	

	auto finish = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = finish - start;
	std::cout << "Time is " << elapsed.count() << endl<< endl;

	cache.print_results();
	
}
