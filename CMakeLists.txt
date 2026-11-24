cmake_minimum_required(VERSION 3.25)

set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)

project(AnyBox VERSION 0.1.0 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "AnyBox requires Windows")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "AnyBox requires an x64 target")
endif()

include(CheckCXXSourceCompiles)
check_cxx_source_compiles([=[
    #if defined(_M_ARM64EC) || (!defined(_M_X64) && !defined(__x86_64__))
    #error AnyBox requires an x64 target
    #endif
    int main() {}
]=] ANYBOX_TARGET_IS_X64)
if(NOT ANYBOX_TARGET_IS_X64)
    message(FATAL_ERROR "AnyBox requires an x64 target")
endif()

add_library(anybox_options INTERFACE)
target_compile_features(anybox_options INTERFACE cxx_std_20)
if(MSVC)
    target_compile_options(anybox_options INTERFACE /W4 /WX /permissive- /EHsc- /Zc:__cplusplus /utf-8)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(anybox_options INTERFACE -Wall -Wextra -Wpedantic -Werror -fexceptions)
endif()
set(CMAKE_CXX_EXTENSIONS OFF)

set(ANYBOX_RUNTIME_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

include(CTest)
include(cmake/AnyBoxLibrary.cmake)

add_subdirectory(core/common)
add_subdirectory(core/libs)
