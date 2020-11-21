cmake_minimum_required(VERSION 3.5)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

project(StackLib)
project(StackDemo)
project(StackStress)

add_library(StackLib SHARED "src/stack.c")
add_executable(StackDemo "src/demo_stack.c")
add_executable(StackStress "src/stress_stack.c")

target_include_directories(StackLib PUBLIC "${PROJECT_SOURCE_DIR}/include/")
#target_compile_definitions(StackLib PRIVATE USE_LOG USE_POISON USE_HASH_FULL USE_CANARY USE_DATA_CANARY)
#target_compile_definitions(StackLib PRIVATE USE_HASH_FAST USE_CANARY USE_DATA_CANARY)

target_link_libraries(StackDemo StackLib)
target_link_libraries(StackStress StackLib)
