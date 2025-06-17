cmake_minimum_required(VERSION 3.10)
project(common)

# Create library
add_library(common
    mqtt_client.cpp
    logger.cpp
    alarm.cpp
    config.cpp
)

# Include directories
target_include_directories(common
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# Link dependencies
target_link_libraries(common
    PUBLIC
        mosquitto
        yaml-cpp
        nlohmann_json::nlohmann_json
) 