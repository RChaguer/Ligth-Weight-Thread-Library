# list test files here
set(tests 01-main;02-switch;11-join;12-join-main;21-create-many;
        22-create-many-recursive;23-create-many-once;31-switch-many;
        32-switch-many-join;33-switch-many-cascade;51-fibonacci;
        61-mutex;62-mutex
        )

# function add binaries compiled with our thread implementation
function(register_test_thread target)
    add_executable(${target} ${target}.c)
    target_link_libraries(${target} PRIVATE thread)
    target_compile_options(${target} PRIVATE -Wall -Wextra)
endfunction(register_test_thread)

# function to add binaries compiled with pthread
function(register_test_pthread target)
    add_executable(${target}-pthread EXCLUDE_FROM_ALL ${target}.c)
    target_link_libraries(${target}-pthread PRIVATE pthread)
    target_compile_options(${target}-pthread PRIVATE -Wall -Wextra)
    target_compile_definitions(${target}-pthread PRIVATE USE_PTHREAD)
    target_include_directories(${target}-pthread PRIVATE ${CMAKE_SOURCE_DIR}/include)
endfunction(register_test_pthread)

# add custom target pthreads for excluded_from_all targets ${test}-pthread
add_custom_target(pthreads)

# add binaries
foreach(tst IN LISTS tests)
    register_test_pthread(${tst})
    add_dependencies(pthreads ${tst}-pthread)
    install(TARGETS ${tst}-pthread DESTINATION bin)
    # uncomment this when our thread implementation is done
    #register_test_thread(${tst})
    #install(TARGETS ${tst} DESTINATION bin)
endforeach()

# add custom target check to run tests
add_custom_target(check
        COMMAND ${CMAKE_BUILD_TOOL} test
        DEPENDS ${tests}
        )

# add custom target graphs
add_custom_target(graphs
        COMMAND python3 ${CMAKE_SOURCE_DIR}/graphs/draw_script.py
        DEPENDS ${tests} list(TRANSFORM ${tests} APPEND pthread)
        )

# add custom target valgrind
find_program(MEMORYCHECK_COMMAND valgrind)
set(MEMORYCHECK_COMMAND_OPTIONS "--leak-check=full --show-reachable=yes --track-origins=yes")
add_custom_target(valgrind
        COMMAND ${CMAKE_CTEST_COMMAND}
            --force-new-ctest-process --test-action memcheck
        COMMAND cat "${CMAKE_BINARY_DIR}/Testing/Temporary/MemoryChecker.*.log"
        DEPENDS ${tests}
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        )

# add tests here
# add_test(01-main-pthread 01-main-pthread)