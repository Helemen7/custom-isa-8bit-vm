cmake_minimum_required(VERSION "3.19.2")
project(custom-isa-8bit-vm)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(MINIFB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(APPLE)
    set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_PLATFORM_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

add_executable(custom-isa-8bit-vm src/main.cpp src/VM.cpp src/Assembler.cpp src/MemoryManager.cpp)
add_subdirectory(external/minifb)
target_link_libraries(custom-isa-8bit-vm PRIVATE minifb)
target_include_directories(custom-isa-8bit-vm PRIVATE external/minifb/include)
target_compile_options(custom-isa-8bit-vm PRIVATE -g)
target_include_directories(custom-isa-8bit-vm  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)
