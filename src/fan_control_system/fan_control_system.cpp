#include "fan_control_system/fan_control_system.hpp"
#include "fan_control_system/fan_simulator.hpp"
#include "fan_control_system/temp_monitor_and_cooling.hpp"
#include "fan_control_system/log_manager.hpp"
#include "fan_control_system/alarm_manager.hpp"
#include <iostream>
#include <stdexcept>
#include "common/config.hpp"

/**
 * @brief Constructs a new FanControlSystem instance
 * 
 * Initializes the fan control system with the specified configuration file.
 * Loads the configuration and sets up the system components.
 * 
 * @param config_file Path to the configuration file
 * @throw std::runtime_error if configuration loading fails
 */
FanControlSystem::FanControlSystem(const std::string& config_file)
    : config_file_(config_file) {
    std::cout << "Loading configuration from: " << config_file << std::endl;
    if (!load_configuration()) {
        throw std::runtime_error("Failed to load configuration");
    }
}

/**
 * @brief Destructor for FanControlSystem instance
 * 
 * Ensures the system is properly stopped and resources are cleaned up.
 */
FanControlSystem::~FanControlSystem() {
    stop();
}

/**
 * @brief Starts the fan control system
 * 
 * Initializes all components and starts the main monitoring thread.
 * 
 * @return true if startup was successful, false otherwise
 */
bool FanControlSystem::start() {
    if (running_) {
        std::cout << "Fan control system already running" << std::endl;
        return true;
    }

    if (!initialize_components()) {
        std::cerr << "Failed to initialize components" << std::endl;
        return false;
    }

    running_ = true;
    main_thread_ = std::thread(&FanControlSystem::main_thread_function, this);
    return true;
}

/**
 * @brief Stops the fan control system
 * 
 * Stops all sub-systems and the main monitoring thread.
 */
void FanControlSystem::stop() {
    if (!running_) {
        return;
    }

    // Stop all the sub-systems
    if (alarm_manager_) alarm_manager_->stop();
    if (log_manager_) log_manager_->stop();
    if (fan_simulator_) fan_simulator_->stop();
    if (temp_monitor_) temp_monitor_->stop();

    running_ = false;
    if (main_thread_.joinable()) {
        main_thread_.join();
    }
    std::cout << "Fan control system stopped" << std::endl;
}

/**
 * @brief Loads the system configuration
 * 
 * Loads configuration from the specified file using the Config singleton.
 * Sets up MQTT settings and system configuration.
 * 
 * @return true if configuration was loaded successfully, false otherwise
 */
bool FanControlSystem::load_configuration() {
    try {
        auto& config = common::Config::getInstance();
        if (!config.load(config_file_)) {
            std::cerr << "Failed to load configuration file: " << config_file_ << std::endl;
            return false;
        }
        config_ = config.getConfig();
        mqtt_settings_ = config.getMQTTSettings();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading configuration: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief Initializes all system components
 * 
 * Creates and initializes the fan simulator, temperature monitor,
 * log manager, and alarm manager components.
 * 
 * @return true if all components were initialized successfully, false otherwise
 */
bool FanControlSystem::initialize_components() {
    try {        
        // Initialize components with configuration
        fan_simulator_ = std::make_shared<fan_control_system::FanSimulator>(config_, mqtt_settings_);
        std::cout << "Fan simulator initialized" << std::endl;
        
        temp_monitor_ = std::make_unique<fan_control_system::TempMonitorAndCooling>(
            config_, mqtt_settings_, fan_simulator_);
        std::cout << "Temp monitor initialized" << std::endl;
        
        log_manager_ = std::make_unique<fan_control_system::LogManager>(config_, mqtt_settings_);
        std::cout << "Log manager initialized" << std::endl;
        
        alarm_manager_ = std::make_unique<fan_control_system::AlarmManager>(config_, mqtt_settings_);
        std::cout << "Alarm manager initialized" << std::endl;

        std::cout << "All components initialized successfully" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error initializing components: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief Main thread function for the fan control system
 * 
 * Starts all sub-systems and runs the main coordination loop.
 * Handles system-wide coordination and monitoring.
 */
void FanControlSystem::main_thread_function() {    
    // Start all the sub-systems
    if (alarm_manager_) alarm_manager_->start();
    if (log_manager_) log_manager_->start();
    if (fan_simulator_) fan_simulator_->start();
    if (temp_monitor_) temp_monitor_->start();

    std::cout << "All sub-systems started" << std::endl;

    while (running_) {
        // Main thread loop - can be used for system-wide coordination
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    
    std::cout << "Main thread stopped" << std::endl;
} 