cmake_minimum_required(VERSION 3.5)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

project(pal_sigslot VERSION 1.0.1 LANGUAGES CXX)
set(CMAKE_CXX_EXTENSIONS OFF)

include(SigslotUtils)

### options
option(SIGSLOT_COMPILE_EXAMPLES "Compile optional exemples" ON)
option(SIGSLOT_COMPILE_TESTS "Compile tests" ON)
option(SIGSLOT_DISABLE_CODE_SIZE_REDUCTION "Do not try to reduce code size and compilation time" OFF)

### sigslot library
add_library(sigslot INTERFACE)
target_include_directories(sigslot INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_definitions(sigslot INTERFACE
    $<$<BOOL:${SIGSLOT_DISABLE_CODE_SIZE_REDUCTION}>:SIGSLOT_DISABLE_CODE_SIZE_REDUCTION>)
set_target_properties(sigslot PROPERTIES EXPORT_NAME Sigslot)
sigslot_set_properties(sigslot INTERFACE)
add_library(Pal::Sigslot ALIAS sigslot)

#installation
include(GNUInstallDirs)

install(TARGETS sigslot EXPORT PalSigslotConfig DESTINATION ${CMAKE_INSTALL_LIBDIR})
export(TARGETS sigslot NAMESPACE Pal:: FILE PalSigslotConfig.cmake)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT PalSigslotConfig NAMESPACE Pal:: DESTINATION "${CMAKE_INSTALL_DATADIR}/PalSigSlot/cmake")

# dependencies for examples and tests
set(CMAKE_AUTOMOC ON)
find_package(Boost QUIET)  # test of boost bridge with smart pointers
find_package(Qt5 COMPONENTS Core Widgets Gui QUIET)  # optional test of Qt bridge
find_package(Threads REQUIRED)

if(SIGSLOT_COMPILE_EXAMPLES)
    add_subdirectory(example)
endif()

if(SIGSLOT_COMPILE_TESTS)
    add_subdirectory(test)
endif()
