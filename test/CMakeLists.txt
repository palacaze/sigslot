
if(SIGSLOT_RUN_TESTS)
    add_custom_target(tests COMMENT "Build all the unit tests.")
    add_custom_command(
        TARGET tests
        POST_BUILD
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    )
endif()

macro(pal_create_test target ut)
    add_executable(${target} "${ut}")
    add_test(${target} ${target})
    if(SIGSLOT_RUN_TESTS)
        add_dependencies(tests ${target})
    endif()
    sigslot_set_properties(${target} PRIVATE)
    target_link_libraries(${target} PRIVATE Pal::Sigslot)
    target_compile_definitions(${target} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:_SCL_SECURE_NO_WARNINGS>)
endmacro()

file(
    GLOB_RECURSE UNIT_TESTS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "*.cpp"
)
foreach(ut IN LISTS UNIT_TESTS)
    string(REPLACE ".cpp" "" target ${ut})
    #XXX string(REGEX REPLACE "/" "." target ${target})

    if(target MATCHES "boost")
        if(TARGET Boost::system)
            message(STATUS "build ${target}")
            pal_create_test(${target} "${ut}")
            target_link_libraries(${target} PRIVATE Boost::system)
        endif()
    elseif(target MATCHES "qt")
        if(TARGET Qt5::Core)
            message(STATUS "build ${target}")
            pal_create_test(${target} "${ut}")
            target_link_libraries(${target} PRIVATE Qt5::Core)
            set_target_properties(${target} PROPERTIES AUTOMOC ON)
        endif()
    elseif(target MATCHES "bench")
        if(NOT SIGSLOT_SANITIZE_THREADS)
            message(STATUS "build ${target}")
            pal_create_test(${target} "${ut}")
        endif()
    else()
        message(STATUS "build ${target}")
        pal_create_test(${target} "${ut}")
    endif()
endforeach()
