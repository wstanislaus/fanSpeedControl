# Add source files
set(SOURCES
    main.cpp
    cli.cpp
)

# Create executable
add_executable(debug_cli ${SOURCES})

# Include directories
target_include_directories(debug_cli PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${YAML_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/src/proto
)

# Link libraries
target_link_libraries(debug_cli PRIVATE
    ${YAML_LIBRARIES}
    nlohmann_json::nlohmann_json
    common
    mcu_simulator_proto
    fan_control_system_proto
)

# Install targets
install(TARGETS debug_cli DESTINATION bin) 