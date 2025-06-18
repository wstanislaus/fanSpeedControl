# Add source files
set(SOURCES
    main.cpp
    mcu_simulator.cpp
    mcu.cpp
    temperature_sensor.cpp
    mcu_simulator_server.cpp
)

# Create executable
add_executable(mcu_simulator ${SOURCES})

# Include directories
target_include_directories(mcu_simulator PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${MOSQUITTO_INCLUDE_DIRS}
    ${YAML_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/src/proto
)

# Link libraries
target_link_libraries(mcu_simulator PRIVATE
    ${MOSQUITTO_LIBRARIES}
    ${YAML_LIBRARIES}
    nlohmann_json::nlohmann_json
    pthread
    common
    proto
)

# Install targets
install(TARGETS mcu_simulator DESTINATION bin) 