# All test files inside tests/src
cmake_minimum_required(VERSION 3.30)
set(CMAKE_C_STANDARD 99)
set(CMAKE_BUILD_TYPE Debug)
project(dbgecko_test C)

file(GLOB TEST_A "src/test_config_loader.c")
file(GLOB TEST_B "src/test_config_arg_parser.c")
file(GLOB TEST_C "src/test_list_plugins.c")
file(GLOB TEST_D "src/test_plugin_loader.c")
file(GLOB TEST_E "src/test_pg_backup.c")

add_executable(test_config_loader ${TEST_A})
add_executable(test_config_arg_parser ${TEST_B})
add_executable(test_list_plugins ${TEST_C})
add_executable(test_plugin_loader ${TEST_D})
add_executable(test_pg_backup ${TEST_E})
# Link against the project library


target_link_libraries(test_config_loader PRIVATE dbgecko_core)

target_link_libraries(test_plugin_loader PUBLIC dbgecko_core)
target_link_options(test_plugin_loader PRIVATE -rdynamic -export-dynamic)

target_link_libraries(test_pg_backup PUBLIC dbgecko_core)
target_link_options(test_pg_backup PRIVATE -rdynamic -export-dynamic)

target_link_libraries(test_config_arg_parser PRIVATE dbgecko_core)
target_link_libraries(test_list_plugins PRIVATE dbgecko_core)

add_test(NAME test_config_loader COMMAND test_config_loader "${CMAKE_SOURCE_DIR}/cmake/tests/config.yml")
# add_test(NAME test_config_arg_parser COMMAND test_config_arg_parser "--config_path=${CMAKE_SOURCE_DIR}/cmake/tests/config.yml"  "--db_timeout_seconds=10" "--storage_compression=deflate2" "--runtime_log_level=8" "--storage_remote_target=https://cloudflare.com/connect")
add_test(NAME test_config_arg_parser COMMAND test_config_arg_parser "--config_path=${CMAKE_SOURCE_DIR}/cmake/tests/config.yml")
add_test(NAME test_list_plugins COMMAND test_list_plugins "--config_path=${CMAKE_SOURCE_DIR}/cmake/tests/config.yml")
add_test(NAME test_plugin_loader COMMAND test_plugin_loader "--config_path=${CMAKE_SOURCE_DIR}/cmake/tests/config.yml")
# add_test(NAME test_pg_backup COMMAND test_pg_backup "--config_path=${CMAKE_SOURCE_DIR}/cmake/tests/config.yml")
