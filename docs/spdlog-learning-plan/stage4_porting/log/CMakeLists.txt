add_library(robo_log STATIC
    src/robo_log.cpp
)
target_link_libraries(robo_log PUBLIC spdlog::spdlog Threads::Threads)
target_include_directories(robo_log PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
