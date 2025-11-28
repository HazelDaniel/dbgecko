cmake_minimum_required(VERSION 3.30)
set(CMAKE_C_STANDARD 99)
project(dbgecko_root C)

enable_testing()

# Build the real project as a library (exclude main.c)
file(GLOB SRC_FILES "src/*.c" "src/*/*.c")
list(FILTER SRC_FILES EXCLUDE REGEX ".*main\\.c")
set(CMAKE_BUILD_TYPE Debug)


add_library(dbgecko_core ${SRC_FILES})
find_package(PkgConfig REQUIRED)

# External libs
find_library(YAML_LIB yaml)
pkg_check_modules(PCRE2 REQUIRED libpcre2-8)

target_link_libraries(dbgecko_core PUBLIC ${YAML_LIB} m  pcre2-8)

target_include_directories(dbgecko_core PUBLIC include ${PCRE2_INCLUDE_DIRS})

target_compile_options(dbgecko_core PRIVATE ${PCRE2_CFLAGS_OTHER})

# Tests
add_subdirectory(cmake)
add_subdirectory(cmake/tests)
