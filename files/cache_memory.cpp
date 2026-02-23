#include <vector>
#include <string>
#include "cache_memory.h"
#include "sim.h"

Sim& CacheMemory::add_sim(const std::string& config_line)
{
    levels.push_back(std::make_unique<Sim>(config_line));
    Sim* cur = levels.back().get();

    if (levels.size() >= 2) {
        Sim* prev = levels[levels.size() - 2].get();
        prev->set_next(cur);
        cur->set_prev(prev);
    }
    // для первого элемента prev/next останутся nullptr

    return *cur;                               // возвращаем ссылку
}

Sim& CacheMemory::add_sim()
{
    levels.push_back(std::make_unique<Sim>()); // создаём Sim в куче
    return *levels.back();                                // возвращаем ссылку
}

void CacheMemory::print_results()
{
    for (auto& s : levels) {
        s->print_results();     // у нас unique_ptr, поэтому через ->
    }
}

void CacheMemory::map_init_sim(const char* str)
{
    for (auto& s : levels) {
        s->map_init_sim(str);     // у нас unique_ptr, поэтому через ->
    }
}

void CacheMemory::sim_init()
{
    for (auto& s : levels) {
        s->sim_init();     // у нас unique_ptr, поэтому через ->
    }
}

void CacheMemory::trace_handler(int64_t cur_adr, const char* array_name, const char* access_mode, const char* func)
{
    levels[0]->trace_handler(cur_adr, array_name, access_mode, func);
}


CacheMemory cache;