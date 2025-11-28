cmake_minimum_required(VERSION 3.30)
set(CMAKE_C_STANDARD 99)
project(dbgecko C)

# Set default compiler to GCC if not specified
if(NOT CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER gcc)
endif()

# Collect all C source files
file(GLOB SRC_FILES "../src/*.c" "../src/utils/*.c")

set(CMAKE_BUILD_TYPE Debug)

# include(FindPkgConfig)
find_package(PkgConfig REQUIRED)

pkg_check_modules(PCRE2 REQUIRED libpcre2-8)
pkg_check_modules(YAML REQUIRED yaml-0.1)

# Create the executable
add_executable(${PROJECT_NAME} ${SRC_FILES})

target_link_libraries(${PROJECT_NAME} ${YAML_LIBRARIES} m pcre2-8)

target_include_directories(${PROJECT_NAME} PRIVATE ../include ${PCRE2_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC ${YAML_INCLUDE_DIRS})

target_compile_options(${PROJECT_NAME} PRIVATE ${PCRE2_CFLAGS_OTHER})
# Compiler flags (applies to all targets)
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wconversion
    -Wformat=2
    -std=c11
)

enable_testing()

install(TARGETS ${PROJECT_NAME} DESTINATION /usr/bin)
