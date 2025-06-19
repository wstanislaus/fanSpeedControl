# Add source files
set(SOURCES
    main.cpp
    fan_control_system.cpp
    fan.cpp
    fan_simulator.cpp
    temp_monitor_and_cooling.cpp
    log_manager.cpp
    alarm_manager.cpp
    fan_control_system_server.cpp
)

# Create executable
add_executable(fan_control_system ${SOURCES})

# Include directories
target_include_directories(fan_control_system PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${MOSQUITTO_INCLUDE_DIRS}
    ${YAML_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(fan_control_system PRIVATE
    ${MOSQUITTO_LIBRARIES}
    ${YAML_LIBRARIES}
    nlohmann_json::nlohmann_json
    pthread
    common
    stdc++fs
    fan_control_system_proto
)

# Install targets
install(TARGETS fan_control_system DESTINATION bin) 