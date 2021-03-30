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
