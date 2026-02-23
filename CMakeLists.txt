cmake_minimum_required(VERSION 3.20)
project(CacheSim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Автоматически собрать список всех .cpp файлов из папки files/
file(GLOB SOURCES "files/*.cpp")

add_executable(CacheSim ${SOURCES})

# Чтобы компилятор находил заголовочные .h файлы из папки files/
target_include_directories(CacheSim PRIVATE files)
