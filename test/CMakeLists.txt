enable_testing()

add_custom_target(tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    COMMENT "Build and run all the unit tests.")

macro(pal_create_test target ut)
    add_executable(${target} EXCLUDE_FROM_ALL "${ut}")
    add_test(${target} ${target})
    add_dependencies(tests ${target})
    sigslot_set_properties(${target} PRIVATE)
    target_link_libraries(${target} PRIVATE Pal::Sigslot)
endmacro()

file(GLOB_RECURSE UNIT_TESTS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp")
foreach(ut IN LISTS UNIT_TESTS)
    string(REPLACE ".cpp" "" target ${ut})
    string(REGEX REPLACE "/" "." target ${target})

    if (target MATCHES "boost")
        if (Boost_FOUND)
            pal_create_test(${target} "${ut}")
        endif()
    elseif (target MATCHES "qt")
        if (Qt5_FOUND)
            pal_create_test(${target} "${ut}")
            target_link_libraries(${target} PRIVATE Qt5::Core)
        endif()
    else()
        pal_create_test(${target} "${ut}")
    endif()
endforeach()
