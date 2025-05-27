cmake_minimum_required(VERSION 3.31)
project(DataStructure)

# C++20标准
set(CMAKE_CXX_STANDARD 20)
# 可执行程序输出路径为output文件夹
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/output/)

add_compile_options(-std=c++20 -Wall -Wextra -Werror)

include_directories(include)

add_executable(DataStructure
        src/main.cpp)
