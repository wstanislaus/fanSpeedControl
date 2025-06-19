cmake_minimum_required(VERSION 3.10)
project(common)

include(FindgRPC)

# Create shared library
add_library(common SHARED
    mqtt_client.cpp
    logger.cpp
    alarm.cpp
    config.cpp
    rpc_server.cpp
)

# Include directories
target_include_directories(common
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

# Link dependencies
target_link_libraries(common
    PUBLIC
        mosquitto
        yaml-cpp
        nlohmann_json::nlohmann_json
        ${gRPC_LIBRARIES}
)

# Set library properties
set_target_properties(common PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
    POSITION_INDEPENDENT_CODE ON
)

# Install targets
install(TARGETS common
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
) 