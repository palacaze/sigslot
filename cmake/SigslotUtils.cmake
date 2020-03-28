# Compiler name
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if (CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
        set(SIGSLOT_COMPILER_CLANGCL ON)
        set(SIGSLOT_COMPILER_NAME "clang-cl")
    else()
        set(SIGSLOT_COMPILER_CLANG ON)
        set(SIGSLOT_COMPILER_NAME "clang")
    endif()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(SIGSLOT_COMPILER_GCC ON)
    set(SIGSLOT_COMPILER_NAME "gcc")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(SIGSLOT_COMPILER_MSVC ON)
    set(SIGSLOT_COMPILER_NAME "msvc")
endif()

if (SIGSLOT_COMPILER_CLANG OR SIGSLOT_COMPILER_GCC)
    set(SIGSLOT_COMPILER_CLANG_OR_GCC ON)
endif()
if (SIGSLOT_COMPILER_CLANG_OR_GCC OR SIGSLOT_COMPILER_CLANGCL)
    set(SIGSLOT_COMPILER_CLANG_OR_CLANGCL_OR_GCC ON)
endif()

add_library(Sigslot_CommonWarnings INTERFACE)
target_compile_options(Sigslot_CommonWarnings INTERFACE
    $<$<BOOL:${SIGSLOT_COMPILER_CLANG_OR_CLANGCL_OR_GCC}>:-Wall;-Wextra>
    $<$<BOOL:${SIGSLOT_COMPILER_CLANG_OR_GCC}>:-fdiagnostics-color=always;-pipe>
    $<$<BOOL:${SIGSLOT_COMPILER_CLANGCL}>:
        -Wno-c++98-compat;-Wno-c++98-compat-pedantic;-Wno-documentation;-Wno-missing-prototypes>
    $<$<CXX_COMPILER_ID:MSVC>:/W3>
)

add_library(Sigslot_ManyWarnings INTERFACE)
target_compile_options(Sigslot_ManyWarnings INTERFACE
    $<$<BOOL:${SIGSLOT_COMPILER_CLANG_OR_CLANGCL_OR_GCC}>:
        -Weverything;-Wno-c++98-compat;-Wno-c++98-compat-pedantic;-Wno-unused-macros;
        -Wno-newline-eof;-Wno-exit-time-destructors;-Wno-global-constructors;
        -Wno-gnu-zero-variadic-macro-arguments;-Wno-documentation;-Wno-missing-prototypes;
        -Wno-padded;-Wno-weak-vtables;-Wno-c++17-extensions;-Wno-shadow-field-in-constructor;
        -Wno-return-std-move-in-c++11>
    $<$<BOOL:${SIGSLOT_COMPILER_CLANG_OR_GCC}>:-fdiagnostics-color=always;-pipe>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# RTTI
add_library(Sigslot_NoRTTI INTERFACE)
target_compile_options(Sigslot_NoRTTI INTERFACE
    $<$<BOOL:${SIGSLOT_COMPILER_CLANG_OR_CLANGCL_OR_GCC}>:-fno-rtti>
    $<$<CXX_COMPILER_ID:MSVC>:/GR->
)

# Profiling
add_library(Sigslot_Profiling INTERFACE)
target_compile_options(Sigslot_Profiling INTERFACE
    $<$<BOOL:${SIGSLOT_COMPILER_CLANG_OR_CLANGCL_OR_GCC}>:-g;-fno-omit-frame-pointer>
)

# sanitizers
add_library(Sigslot_AddressSanitizer INTERFACE)
target_compile_options(Sigslot_AddressSanitizer INTERFACE
    $<$<BOOL:${SIGSLOT_COMPILER_CLANG_OR_GCC}>:
        -g;-fno-omit-frame-pointer;-fsanitize=address;-fsanitize-address-use-after-scope>
)
target_link_libraries(Sigslot_AddressSanitizer INTERFACE
    $<$<BOOL:${SIGSLOT_COMPILER_CLANG_OR_GCC}>:-fsanitize=address>
)

add_library(Sigslot_ThreadSanitizer INTERFACE)
target_compile_options(Sigslot_ThreadSanitizer INTERFACE
    $<$<BOOL:${SIGSLOT_COMPILER_CLANG_OR_GCC}>:
        -g;-fno-omit-frame-pointer;-fsanitize=thread>
)
target_link_libraries(Sigslot_ThreadSanitizer INTERFACE
    $<$<BOOL:${SIGSLOT_COMPILER_CLANG_OR_GCC}>:-fsanitize=thread>
)

add_library(Sigslot_UndefinedSanitizer INTERFACE)
target_compile_options(Sigslot_UndefinedSanitizer INTERFACE
    $<$<BOOL:${SIGSLOT_COMPILER_CLANG_OR_GCC}>:
        -g;-fno-omit-frame-pointer;-fsanitize=undefined>
)
target_link_libraries(Sigslot_UndefinedSanitizer INTERFACE
    $<$<BOOL:${SIGSLOT_COMPILER_CLANG_OR_GCC}>:-fsanitize=undefined>
)

# Use Libcxx
add_library(Sigslot_Libcxx INTERFACE)
target_compile_options(Sigslot_Libcxx INTERFACE
    $<$<BOOL:${SIGSLOT_COMPILER_CLANG}>:-stdlib=libc++>
)
target_link_libraries(Sigslot_Libcxx INTERFACE
    $<$<BOOL:${SIGSLOT_COMPILER_CLANG}>:-stdlib=libc++;-rtlib=compiler-rt>
)

option(SIGSLOT_ENABLE_COMMON_WARNINGS "Enable common compiler flags" ON)
option(SIGSLOT_ENABLE_MANY_WARNINGS "Enable most compiler flags" OFF)
option(SIGSLOT_DISABLE_RTTI "Disable Runtime Type Information" OFF)
option(SIGSLOT_ENABLE_LTO "Enable link time optimization (release only)" OFF)
option(SIGSLOT_ENABLE_LIBCXX "Use libcxx with clang" OFF)
option(SIGSLOT_ENABLE_PROFILING "Add compile flags to help with profiling" OFF)
option(SIGSLOT_SANITIZE_ADDRESS "Compile with address sanitizer support" OFF)
option(SIGSLOT_SANITIZE_THREADS "Compile with thread sanitizer support" OFF)
option(SIGSLOT_SANITIZE_UNDEFINED "Compile with undefined sanitizer support" OFF)

# common properties
function(sigslot_set_properties target scope)
    target_compile_features(${target} ${scope} cxx_std_14)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)

    # account for options
    target_link_libraries(${target} ${scope}
        $<$<BOOL:${SIGSLOT_ENABLE_COMMON_WARNINGS}>:Sigslot_CommonWarnings>
        $<$<BOOL:${SIGSLOT_ENABLE_MANY_WARNINGS}>:Sigslot_ManyWarnings>
        $<$<BOOL:${SIGSLOT_DISABLE_RTTI}>:Sigslot_NoRTTI>
        $<$<BOOL:${SIGSLOT_ENABLE_LIBCXX}>:Sigslot_Libcxx>
        $<$<BOOL:${SIGSLOT_ENABLE_PROFILING}>:Sigslot_Profiling>
        $<$<BOOL:${SIGSLOT_SANITIZE_ADDRESS}>:Sigslot_AddressSanitizer>
        $<$<BOOL:${SIGSLOT_SANITIZE_THREADS}>:Sigslot_ThreadSanitizer>
        $<$<BOOL:${SIGSLOT_SANITIZE_UNDEFINED}>:Sigslot_UndefinedSanitizer>
    )

    if (SIGSLOT_ENABLE_LTO)
        set_target_properties(${target} PROPERTIES INTERFACE_INTERPROCEDURAL_OPTIMIZATION ON)
    endif()
endfunction()
