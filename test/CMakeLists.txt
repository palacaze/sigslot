add_custom_target(tests
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  COMMENT "Build and run all the unit tests.")

file(GLOB_RECURSE UNIT_TESTS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp")
foreach(ut IN LISTS UNIT_TESTS)
    string(REPLACE ".cpp" "" target ${ut})
    string(REGEX REPLACE "/" "." target ${target})
    add_executable(${target} EXCLUDE_FROM_ALL "${ut}")
    add_test(${target} ${target})
    add_dependencies(tests ${target})
    target_link_libraries(${target} Pal::Sigslot)
    pal_set_properties(${target} PRIVATE)
endforeach()

