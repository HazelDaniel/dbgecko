cmake_minimum_required(VERSION 3.30)
set(CMAKE_C_STANDARD 99)
project(dbgecko_root C)

enable_testing()

# Build the real project as a library (exclude main.c)
file(GLOB SRC_FILES "src/*.c" "src/*/*.c")
list(FILTER SRC_FILES EXCLUDE REGEX ".*main\\.c")

add_library(slre STATIC IMPORTED)
set_target_properties(slre PROPERTIES
	IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/./lib/libslre.a"
)

add_library(dbgecko_core ${SRC_FILES})

# External libs
find_library(YAML_LIB yaml)
target_link_libraries(dbgecko_core PUBLIC ${YAML_LIB} m)
target_link_libraries(dbgecko_core PUBLIC slre ${YAML_LIB} m)

target_include_directories(dbgecko_core PUBLIC include)

# Tests
add_subdirectory(cmake)
add_subdirectory(cmake/tests)

