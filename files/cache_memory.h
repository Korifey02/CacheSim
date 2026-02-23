#pragma once
#include <vector>
#include <string>
#include <memory> 
#include "sim.h"

class CacheMemory {
public:
    // добавить новый Sim по строке конфигурации
    Sim& add_sim(const std::string& config_line);

    Sim& add_sim();

    // доступ по индексу
    Sim& operator[](std::size_t i) { return *levels.at(i); }
    const Sim& operator[](std::size_t i) const { return *levels.at(i); }

    std::size_t size() const { return levels.size(); }
    bool empty() const { return levels.empty(); }

    void print_results();


    void map_init_sim(const char* str);
    void trace_handler(int64_t cur_adr, const char* array_name, const char* access_mode, const char* func);
    void sim_init();

private:
    std::vector<std::unique_ptr<Sim>> levels;
};

extern CacheMemory cache;
