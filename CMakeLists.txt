cmake_minimum_required(VERSION 3.5)

project(pal_sigslot VERSION 1.2.0 LANGUAGES CXX)

### main project
set(SIGSLOT_MAIN_PROJECT OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(SIGSLOT_MAIN_PROJECT ON)
endif()

include(cmake/SigslotUtils.cmake)

### options
option(SIGSLOT_COMPILE_EXAMPLES "Compile optional examples" ${SIGSLOT_MAIN_PROJECT})
option(SIGSLOT_COMPILE_TESTS "Compile tests" ${SIGSLOT_MAIN_PROJECT})
option(SIGSLOT_REDUCE_COMPILE_TIME "Attempt at reducing code size and compilation time" OFF)
option(SIGSLOT_ENABLE_INSTALL "Create install target" ${SIGSLOT_MAIN_PROJECT})

find_package(Threads REQUIRED)

### sigslot library
add_library(sigslot INTERFACE)
add_library(Pal::Sigslot ALIAS sigslot)
target_include_directories(sigslot INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_definitions(sigslot INTERFACE
    $<$<BOOL:${SIGSLOT_REDUCE_COMPILE_TIME}>:SIGSLOT_REDUCE_COMPILE_TIME>
)
target_link_options(sigslot INTERFACE
    # We deactivate ICF on windows compilers because it interferes with signal
    # disconnection by making identical functions not unique.
    $<$<CXX_COMPILER_ID:MSVC>:/OPT:NOICF>
    $<$<BOOL:${SIGSLOT_COMPILER_CLANGCL}>:/OPT:NOICF>
)
target_link_libraries(sigslot INTERFACE Threads::Threads)
set_target_properties(sigslot PROPERTIES EXPORT_NAME Sigslot)

if(SIGSLOT_ENABLE_INSTALL)
    #installation
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PalSigslotConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/PalSigslotConfig.cmake
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/PalSigslot"
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/PalSigslotConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(
        TARGETS sigslot
        EXPORT PalSigslotTargets
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    install(
        DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    install(
        EXPORT PalSigslotTargets
        FILE PalSigslotTargets.cmake
        NAMESPACE Pal::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/PalSigslot"
    )
    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/PalSigslotConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/PalSigslotConfigVersion.cmake
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/PalSigslot"
    )
    export(TARGETS sigslot
        NAMESPACE Pal::
        FILE ${CMAKE_CURRENT_BINARY_DIR}/PalSigslotTargets.cmake
    )

    # Install the Config and ConfigVersion files
    install(
        FILES ${version_file}
              ${config_file}
        DESTINATION ${cmake_dir}
        COMPONENT ${projectname_target}Export
    )
endif()

# dependencies for examples and tests
if(SIGSLOT_COMPILE_TESTS OR SIGSLOT_COMPILE_EXAMPLES)
    find_package(Boost COMPONENTS system QUIET)  # test of boost bridge with smart pointers
    find_package(Qt5 COMPONENTS Core Widgets Gui QUIET)  # optional test of Qt bridge
endif()

if(SIGSLOT_COMPILE_EXAMPLES)
    add_subdirectory(example)
endif()

if(SIGSLOT_COMPILE_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
