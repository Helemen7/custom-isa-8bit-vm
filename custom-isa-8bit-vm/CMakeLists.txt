cmake_minimum_required(VERSION "3.19.2")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(custom-isa-8bit-vm)
add_executable(custom-isa-8bit-vm src/main.cpp src/VM.cpp src/Assembler.cpp)
target_compile_options(custom-isa-8bit-vm PRIVATE -g)
target_include_directories(custom-isa-8bit-vm  PUBLIC ${CMAKE_CURRENT_WORK_DIR}/include/)
