cmake_minimum_required(VERSION 3.10)

project(thread VERSION 1.0.0 DESCRIPTION "User space Threads implementation")

option(USE_PTHREAD "Use pthread library" OFF)

# configure a header file to pass value of USE_PTHREAD
configure_file(config.h.in config.h)

# on install copy thread.h and config.h to include folder of installation dir
install(FILES ${PROJECT_BINARY_DIR}/config.h DESTINATION include)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/thread.h DESTINATION include)

# add pthread or the user-space threads implementation
if (USE_PTHREAD)
    list(APPEND EXTRA_LIBS pthread)
    list(APPEND EXTRA_INCLUDES
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${PROJECT_BINARY_DIR}
            )
else()
    list(APPEND EXTRA_LIBS thread)

    add_library(thread SHARED
            src/thread.c
            src/queue.h
            )

    target_include_directories(thread
            PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include # for thread.h
            ${PROJECT_BINARY_DIR} # for config.h
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}}/src # for queue.h
            )

    target_compile_options(thread PRIVATE -Wall -Wextra)

    install(TARGETS thread DESTINATION lib)
endif()

include(CTest)

add_subdirectory(test)
