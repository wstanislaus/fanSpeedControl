#include "mcu_simulator/mcu_simulator.hpp"
#include "mcu_simulator/mcu_simulator_server.hpp"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include "common/config.hpp"

namespace mcu_simulator {

/**
 * @brief Constructs a new MCU Simulator
 * 
 * Initializes the simulator with the path to its configuration file.
 * The simulator is not running at this point and needs to be initialized
 * and started separately.
 * 
 * @param config_file Path to the YAML configuration file
 */
MCUSimulator::MCUSimulator(const std::string& config_file)
    : config_file_(config_file)
    , running_(false)
{
    name_ = "MCUSimulator";
}

/**
 * @brief Destructor that ensures proper cleanup
 * 
 * Stops all MCUs and cleans up resources when the simulator is destroyed.
 */
MCUSimulator::~MCUSimulator() {
    stop();
}

/**
 * @brief Initializes the MCU Simulator
 * 
 * Performs the following initialization steps:
 * 1. Loads configuration from the YAML file
 * 2. Initializes MQTT client for simulator-level communication
 * 3. Sets up logger and alarm system
 * 4. Creates and initializes MCUs based on configuration
 * 
 * @return true if initialization was successful, false otherwise
 * @throws std::exception if any initialization step fails
 */
bool MCUSimulator::initialize() {
    try {
        // Initialize common components
        auto& config = common::Config::getInstance();
        if (!config.load(config_file_)) {
            std::cerr << "Failed to load config file" << std::endl;
            return false;
        }
        config_ = config.getConfig();

        // Get log level for MCUSimulator from config
        auto log_level = config_["AppLogLevel"][name_].as<std::string>();

        // Initialize MQTT client for simulator-level logging
        auto mqtt_settings = config.getMQTTSettings();
        mqtt_client_ = std::make_shared<common::MQTTClient>(name_, mqtt_settings);
        if (!mqtt_client_->initialize() || !mqtt_client_->connect()) {
            std::cerr << "Failed to initialize MQTT client for simulator" << std::endl;
            return false;
        }

        // Initialize logger and alarm for simulator
        logger_ = std::make_unique<common::Logger>(name_, log_level, mqtt_client_);
        alarm_ = std::make_unique<common::Alarm>(name_, mqtt_client_);
        logger_->info("MCU Simulator initializing...");

        // Global max supported MCUs and max sensors per MCU
        const int max_mcus = config_["MaxMCUsSupported"].as<int>();
        const int max_sensors_per_mcu = config_["MaxTempSensorsPerMCU"].as<int>();

        // Create MCUs based on configuration
        const auto& mcu_config = config_["MCUs"];
        // Validate and raise alarm if number of MCUs exceeds the maximum, however use first max number of MCUs
        if (mcu_config.size() > max_mcus) {
            logger_->error("Number of MCUs: " + std::to_string(mcu_config.size()) + " exceeds the maximum of " + std::to_string(max_mcus));
            alarm_->raise(common::AlarmSeverity::HIGH, "Number of MCUs: " + std::to_string(mcu_config.size()) + " exceeds the maximum of " + std::to_string(max_mcus));
        }
        size_t mcu_count = 0;
        // Create MCUs based on configuration
        for (const auto& mcu_node : mcu_config) {
            if (mcu_count++ >= max_mcus) {
                break;
            }
            const auto& mcu_name = mcu_node.first.as<std::string>();
            const auto& mcu_data = mcu_node.second;
            int num_sensors = mcu_data["NumberOfSensors"].as<int>();

            std::cout << "MCU " << mcu_name << " has " << num_sensors << " sensors" << std::endl;

            // Validate and raise alarm if number of sensors exceeds the maximum, however use first max number of sensors
            if (num_sensors > max_sensors_per_mcu) {
                logger_->error("MCU " + mcu_name + " has " + std::to_string(num_sensors) + " sensors, but the maximum is " + std::to_string(max_sensors_per_mcu) + " using first " + std::to_string(max_sensors_per_mcu) + " sensors");
                alarm_->raise(common::AlarmSeverity::HIGH, "MCU " + mcu_name + " has " + std::to_string(num_sensors) + " sensors, but the maximum is " + std::to_string(max_sensors_per_mcu) + " using first " + std::to_string(max_sensors_per_mcu) + " sensors");
            }
            num_sensors = std::min(num_sensors, max_sensors_per_mcu);

            logger_->info("Creating MCU: " + mcu_name + " with " + std::to_string(num_sensors) + " sensors");

            // Load temperature settings
            auto temp_settings = loadTemperatureSettings(config_);

            // Create MCU instance
            auto mcu = std::make_unique<MCU>(mcu_name, num_sensors, temp_settings, mqtt_settings, mcu_data, config_file_);
            if (!mcu->initialize()) {
                logger_->error("Failed to initialize MCU " + mcu_name);
                alarm_->raise(common::AlarmSeverity::HIGH, "MCU initialization failed: " + mcu_name);
                return false;
            }

            mcus_.push_back(std::move(mcu));
            logger_->info("MCU " + mcu_name + " initialized successfully");
        }

        // Initialize RPC server
        rpc_server_ = std::make_unique<MCUSimulatorServer>(*this);
        logger_->info("RPC server initialized");

        logger_->info("MCU Simulator initialized successfully with " + std::to_string(mcus_.size()) + " MCUs");
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error initializing MCU simulator: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief Starts all MCUs in the simulator
 * 
 * Starts the temperature monitoring and MQTT publishing for all MCUs.
 * Logs the start operation and its result.
 */
void MCUSimulator::start() {
    if (running_) return;
    running_ = true;
    logger_->info("Starting MCU Simulator...");

    // Start all MCUs
    for (const auto& mcu : mcus_) {
        mcu->start();
    }

    // Start RPC server
    if (rpc_server_) {
        if (rpc_server_->start()) {
            logger_->info("RPC server started successfully");
        } else {
            logger_->error("Failed to start RPC server");
            alarm_->raise(common::AlarmSeverity::HIGH, "Failed to start RPC server");
        }
    }

    logger_->info("MCU Simulator started successfully");
}

/**
 * @brief Stops all MCUs in the simulator
 * 
 * Stops the temperature monitoring and MQTT publishing for all MCUs.
 * Logs the stop operation and its result.
 */
void MCUSimulator::stop() {
    if (!running_) return;
    running_ = false;
    logger_->info("Stopping MCU Simulator...");

    // Stop RPC server
    if (rpc_server_) {
        rpc_server_->stop();
        logger_->info("RPC server stopped");
    }

    // Stop all MCUs
    for (const auto& mcu : mcus_) {
        mcu->stop();
    }

    logger_->info("MCU Simulator stopped");
}

/**
 * @brief Loads configuration from the YAML file
 * 
 * Attempts to load and parse the YAML configuration file specified
 * in the constructor.
 * 
 * @return true if configuration was loaded successfully, false otherwise
 * @throws std::exception if the file cannot be loaded or parsed
 */
bool MCUSimulator::loadConfig() {
    try {
        config_ = YAML::LoadFile(config_file_);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading configuration file: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief Loads temperature settings from the configuration
 * 
 * Extracts temperature-related settings from the configuration,
 * including thresholds and publish intervals for different
 * temperature ranges.
 * 
 * @param config The YAML configuration node
 * @return TemperatureSettings structure containing all temperature-related settings
 */
MCU::TemperatureSettings MCUSimulator::loadTemperatureSettings(const YAML::Node& config) {
    MCU::TemperatureSettings settings;
    
    const auto& temp_settings = config["TemperatureSettings"];
    settings.bad_threshold = temp_settings["BadThreshold"].as<float>();
    settings.erratic_threshold = temp_settings["ErraticThreshold"].as<float>();

    const auto& intervals = temp_settings["PublishIntervals"];
    for (const auto& interval : intervals) {
        MCU::TemperatureSettings::PublishInterval pub_interval;
        const auto& range = interval["Range"].as<std::vector<float>>();
        pub_interval.min_temp = range[0];
        pub_interval.max_temp = range[1];
        pub_interval.interval_seconds = interval["Interval"].as<int>();
        settings.publish_intervals.push_back(pub_interval);
    }
    return settings;
}
} // namespace mcu_simulator 