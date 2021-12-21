cmake_minimum_required(VERSION 3.16)

project(huawei-riscv-rv32i-sim LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17 REQUIRED)

add_executable(huawei-riscv-rv32i-sim
    src/main.cpp
    src/isa.cpp
    src/cpu.cpp
    src/cpu_env.cpp
)

target_include_directories(huawei-riscv-rv32i-sim PRIVATE
    src
)
