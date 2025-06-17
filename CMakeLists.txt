cmake_minimum_required(VERSION 3.14)
project(temperature-fan-controller VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add cmake modules path
list(APPEND CMAKE_MODULE_PATH 
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake
)

# Include custom modules
include(gRPCGenerate)

# Find required packages
find_package(PkgConfig REQUIRED)

# Find Mosquitto using pkg-config
pkg_check_modules(MOSQUITTO REQUIRED libmosquitto)

# Find yaml-cpp
pkg_check_modules(YAML REQUIRED yaml-cpp)

# Find gRPC and Protobuf
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

# Add nlohmann/json
include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# Set installation paths
include(GNUInstallDirs)
set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Installation Directory")
set(CMAKE_INSTALL_BINDIR "${CMAKE_INSTALL_PREFIX}/bin" CACHE PATH "Binary Directory")

# Add subdirectories
add_subdirectory(src)

# Install config file and create system directory
install(CODE "
    if(NOT EXISTS /etc/fan_control_system)
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E make_directory /etc/fan_control_system
            RESULT_VARIABLE result
        )
        if(NOT result EQUAL 0)
            message(FATAL_ERROR \"Failed to create /etc/fan_control_system directory\")
        endif()
    endif()
")
install(FILES config/config.yaml DESTINATION /etc/fan_control_system)