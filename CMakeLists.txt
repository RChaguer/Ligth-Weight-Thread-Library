cmake_minimum_required(VERSION 3.10)

project(thread VERSION 1.0.0 DESCRIPTION "User space Threads implementation")

add_library(thread SHARED
        src/thread.c
        src/queue.h
        )

target_include_directories(thread
        PUBLIC
        ${CMAKE_SOURCE_DIR}/include # for thread.h
        PRIVATE
        ${CMAKE_SOURCE_DIR}}/src # for queue.h
        )

target_compile_options(thread PRIVATE -Wall -Wextra)

install(TARGETS thread DESTINATION lib)

include(CTest)

add_subdirectory(test)
