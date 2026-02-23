cmake_minimum_required(VERSION 3.30)
set(CMAKE_C_STANDARD 99)
set(CMAKE_BUILD_TYPE Debug)
project(dbgecko C)

# Set default compiler to GCC if not specified
if(NOT CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER gcc)
endif()

# Collect all C source files
file(GLOB SRC_FILES "../src/*.c" "../src/utils/*.c")

# include(FindPkgConfig)
find_package(PkgConfig REQUIRED)
find_package(libssh REQUIRED)
find_package(OpenSSL REQUIRED)

pkg_check_modules(PCRE2 REQUIRED libpcre2-8)
pkg_check_modules(YAML REQUIRED yaml-0.1)
pkg_check_modules(LIBSSH REQUIRED libssh)

# Create the executable
add_executable(${PROJECT_NAME} ${SRC_FILES})

target_link_libraries(${PROJECT_NAME}
	aws-c-s3
	aws-c-auth
	aws-c-http
	aws-c-io
	aws-c-cal
	aws-c-compression
	aws-c-sdkutils
	aws-c-common
)
target_link_libraries(${PROJECT_NAME} ${YAML_LIBRARIES} m pcre2-8 ${LIBSSH_LIBRARIES} OpenSSL::Crypto OpenSSL::SSL)

target_link_options(${PROJECT_NAME} PRIVATE -rdynamic -export-dynamic)

target_include_directories(${PROJECT_NAME} PRIVATE ../include  ${YAML_INCLUDE_DIRS} ${PCRE2_INCLUDE_DIRS} ${LIBSSH_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PUBLIC ${LIBSSH_LIBRARY_DIRS})
target_link_directories(${PROJECT_NAME} PUBLIC /usr/local/lib)

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

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/../bin)
