# list test files here
set(tests 01-main;02-switch;11-join;12-join-main;21-create-many;
        22-create-many-recursive;23-create-many-once;31-switch-many;
        32-switch-many-join;33-switch-many-cascade;51-fibonacci;
        61-mutex;62-mutex
        )

function(register_test target)
    add_executable(${target} ${target}.c)
    target_link_libraries(${target} PRIVATE ${EXTRA_LIBS})
    target_include_directories(${target} PRIVATE ${EXTRA_INCLUDES})
    target_compile_options(${target} PRIVATE -Wall -Wextra)
endfunction(register_test)

foreach(tst IN LISTS tests)
    register_test(${tst})
endforeach()

install(TARGETS ${tests} DESTINATION bin)
