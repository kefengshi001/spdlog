add_executable(test_robo_log test_robo_log.cpp)
target_link_libraries(test_robo_log PRIVATE robo_log)
target_compile_definitions(test_robo_log PRIVATE
    LOG_DIR="${CMAKE_BINARY_DIR}/logs"
)
add_test(NAME test_robo_log COMMAND test_robo_log)

add_executable(test_performance test_performance.cpp)
target_link_libraries(test_performance PRIVATE robo_log)
target_compile_definitions(test_performance PRIVATE
    LOG_DIR="${CMAKE_BINARY_DIR}/logs"
)
add_test(NAME test_performance COMMAND test_performance)
