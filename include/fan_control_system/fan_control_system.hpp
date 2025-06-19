#pragma once

#include <memory>
#include <thread>
#include <atomic>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>
#include "common/mqtt_client.hpp"

// Forward declarations
namespace fan_control_system {
class FanSimulator;
class TempMonitorAndCooling;
class LogManager;
class AlarmManager;
class FanControlSystemServer;
}

/**
 * @class FanControlSystem
 * @brief Main class that coordinates the fan control system components
 * 
 * This class manages the overall fan control system, including temperature monitoring,
 * fan control, logging, and alarm management. It coordinates between different components
 * to maintain optimal system temperature through fan speed control.
 */
class FanControlSystem {
public:
    /**
     * @brief Constructs a new Fan Control System instance
     * @param config_file Path to the YAML configuration file
     */
    FanControlSystem(const std::string& config_file);

    /**
     * @brief Destructor that ensures proper cleanup of all components
     */
    ~FanControlSystem();

    /**
     * @brief Starts the fan control system
     * @return true if the system started successfully, false otherwise
     */
    bool start();

    /**
     * @brief Stops the fan control system
     * @note This will stop all components and clean up resources
     */
    void stop();

    /**
     * @brief Checks if the system is currently running
     * @return true if the system is running, false otherwise
     */
    bool is_running() const { return running_; }

    /**
     * @brief Gets the fan simulator component
     * @return Shared pointer to the fan simulator
     */
    std::shared_ptr<fan_control_system::FanSimulator> get_fan_simulator() const { return fan_simulator_; }

private:
    /**
     * @brief Initializes all system components
     * @return true if all components initialized successfully, false otherwise
     */
    bool initialize_components();

    /**
     * @brief Loads system configuration from the YAML file
     * @return true if configuration was loaded successfully, false otherwise
     */
    bool load_configuration();

    /**
     * @brief Main thread function that coordinates system operations
     * @note This runs in a separate thread and manages the overall system state
     */
    void main_thread_function();

    // Component instances
    std::shared_ptr<fan_control_system::FanSimulator> fan_simulator_;      ///< Fan simulator for controlling fan speeds
    std::unique_ptr<fan_control_system::TempMonitorAndCooling> temp_monitor_; ///< Temperature monitoring and cooling control
    std::unique_ptr<fan_control_system::LogManager> log_manager_;          ///< System-wide logging management
    std::unique_ptr<fan_control_system::AlarmManager> alarm_manager_;      ///< System-wide alarm management

    // Threads
    std::thread main_thread_;                                              ///< Main system thread
    std::atomic<bool> running_{false};                                     ///< System running state

    // Configuration
    std::string config_file_;                                              ///< Path to configuration file
    YAML::Node config_;                                                    ///< Loaded configuration data

    // MQTT settings
    common::MQTTClient::Settings mqtt_settings_;                           ///< MQTT communication settings

    // RPC server
    std::unique_ptr<fan_control_system::FanControlSystemServer> rpc_server_; ///< RPC server for fan control system
}; 