cmake_minimum_required(VERSION 3.6)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

project(pal_sigslot VERSION 0.0.1 LANGUAGES CXX)

### compiler support
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

include(CheckCXXCompilerFlag)
macro(pal_test_compiler_flag flag)
    string(TOUPPER ${flag} testname)
    check_cxx_compiler_flag("-${flag}" ${testname})
    if (${testname})
        list(APPEND PAL_COMPILER_FLAGS "-${flag}")
    endif()
endmacro()

pal_test_compiler_flag("Wall")
pal_test_compiler_flag("Wextra")
pal_test_compiler_flag("Wno-misleading-indentation")
pal_test_compiler_flag("fdiagnostics-color=always")

### options
option(COMPILE_EXAMPLES "Compile optional exemples" ON)
option(COMPILE_TESTS "Compile tests" ON)

### common properties
function(pal_set_properties target mode)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS NO)
    target_compile_definitions(${target} ${mode} $<$<CONFIG:Release>:NDEBUG>)
    target_compile_options(${target} ${mode} ${PAL_COMPILER_FLAGS})
endfunction()

### sigslot library
add_library(sigslot INTERFACE)
target_include_directories(sigslot INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

set_target_properties (sigslot PROPERTIES EXPORT_NAME Sigslot)
add_library(Pal::Sigslot ALIAS sigslot)

include(GNUInstallDirs)

install(TARGETS sigslot EXPORT PalSigslotConfig DESTINATION ${CMAKE_INSTALL_LIBDIR})
export(TARGETS sigslot NAMESPACE Pal:: FILE PalSigslotConfig.cmake)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT PalSigslotConfig NAMESPACE Pal:: DESTINATION "${CMAKE_INSTALL_DATADIR}/PalSigSlot/cmake")

if(COMPILE_EXAMPLES)
    # add_subdirectory(example)
endif()

if(COMPILE_TESTS)
    # optional test of boost bridge with smart pointers
    set(CMAKE_AUTOMOC ON)
    find_package(Boost QUIET)

    # optional test of Qt bridge
    find_package(Qt5 COMPONENTS Core Widgets Gui QUIET)

    find_package(Threads)

    enable_testing()
    add_subdirectory(test EXCLUDE_FROM_ALL)
endif()

