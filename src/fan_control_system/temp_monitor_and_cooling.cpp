#include "fan_control_system/temp_monitor_and_cooling.hpp"
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace fan_control_system {

/**
 * @brief Constructs a new TempMonitorAndCooling instance
 * 
 * Initializes the temperature monitor with configuration, MQTT settings, and fan simulator.
 * Sets up MQTT client and logger, loads MCU configurations.
 * 
 * @param config YAML configuration node containing temperature monitoring settings
 * @param mqtt_settings MQTT client settings for publishing temperature data
 * @param fan_simulator Shared pointer to the fan simulator for controlling fan speeds
 * @throw std::runtime_error if initialization fails
 */
TempMonitorAndCooling::TempMonitorAndCooling(const YAML::Node& config, 
                                           const common::MQTTClient::Settings& mqtt_settings,
                                           std::shared_ptr<FanSimulator> fan_simulator)
    : config_(config), mqtt_settings_(mqtt_settings), fan_simulator_(fan_simulator) {
    name_ = "TempMonitor";
    // Initialize MQTT client and logger first
    mqtt_client_ = std::make_shared<common::MQTTClient>(name_, mqtt_settings_);
    if (!mqtt_client_->initialize() || !mqtt_client_->connect()) {
        throw std::runtime_error("Failed to initialize MQTT client");
    }
    auto log_level = config_["AppLogLevel"]["FanControlSystem"][name_].as<std::string>();
    logger_ = std::make_unique<common::Logger>(name_, log_level, mqtt_client_);
    logger_->info("Temperature Monitor initializing...");

    if (!load_mcu_configs()) {
        throw std::runtime_error("Failed to initialize temperature monitor");
    }
}

/**
 * @brief Destructor for TempMonitorAndCooling instance
 * 
 * Ensures the temperature monitor is properly stopped and resources are cleaned up.
 */
TempMonitorAndCooling::~TempMonitorAndCooling() {
    stop();
}

/**
 * @brief Starts the temperature monitor
 * 
 * Initializes temperature thresholds and fan speed settings, starts the main monitoring thread.
 * 
 * @return true if startup was successful, false otherwise
 */
bool TempMonitorAndCooling::start() {
    if (running_) {
        logger_->info("Temperature Monitor already running");
        return true;
    }

    if (!initialize()) {
        logger_->error("Failed to initialize Temperature Monitor");
        return false;
    }

    running_ = true;
    main_thread_ = std::thread(&TempMonitorAndCooling::main_thread_function, this);
    logger_->info("Temperature Monitor started successfully");
    return true;
}

/**
 * @brief Stops the temperature monitor
 * 
 * Stops the main monitoring thread and cleans up resources.
 */
void TempMonitorAndCooling::stop() {
    if (!running_) {
        return;
    }

    logger_->info("Stopping Temperature Monitor...");
    running_ = false;
    if (main_thread_.joinable()) {
        main_thread_.join();
    }
    logger_->info("Temperature Monitor stopped");
}

/**
 * @brief Gets the current temperature for a specific MCU and sensor
 * 
 * @param mcu_name Name of the MCU
 * @param sensor_id ID of the temperature sensor
 * @return Current temperature in Celsius, or -1.0 if not available
 */
double TempMonitorAndCooling::get_temperature(const std::string& mcu_name, int sensor_id) const {
    std::lock_guard<std::mutex> lock(history_mutex_);
    auto mcu_it = temperature_history_.find(mcu_name);
    if (mcu_it == temperature_history_.end()) {
        logger_->warning("Attempted to get temperature for non-existent MCU: " + mcu_name);
        return -1.0;
    }

    auto sensor_it = mcu_it->second.find(sensor_id);
    if (sensor_it == mcu_it->second.end() || sensor_it->second.readings.empty()) {
        logger_->warning("No temperature readings available for MCU: " + mcu_name + ", Sensor: " + std::to_string(sensor_id));
        return -1.0;
    }

    double temp = sensor_it->second.readings.back().temperature;
    logger_->debug("Current temperature for MCU: " + mcu_name + ", Sensor: " + std::to_string(sensor_id) + 
                  ": " + std::to_string(temp) + "°C");
    return temp;
}

/**
 * @brief Gets the temperature history for a specific MCU and sensor
 * 
 * @param mcu_name Name of the MCU
 * @param sensor_id ID of the temperature sensor
 * @return Deque of temperature readings, empty if no history available
 */
std::deque<TemperatureReading> TempMonitorAndCooling::get_temperature_history(
    const std::string& mcu_name, int sensor_id) const {
    std::lock_guard<std::mutex> lock(history_mutex_);
    auto mcu_it = temperature_history_.find(mcu_name);
    if (mcu_it == temperature_history_.end()) {
        logger_->warning("Attempted to get history for non-existent MCU: " + mcu_name);
        return {};
    }

    auto sensor_it = mcu_it->second.find(sensor_id);
    if (sensor_it == mcu_it->second.end()) {
        logger_->warning("No history available for MCU: " + mcu_name + ", Sensor: " + std::to_string(sensor_id));
        return {};
    }

    logger_->debug("Retrieved temperature history for MCU: " + mcu_name + ", Sensor: " + std::to_string(sensor_id) + 
                  ", Readings: " + std::to_string(sensor_it->second.readings.size()));
    return sensor_it->second.readings;
}

/**
 * @brief Initializes the temperature monitor
 * 
 * Loads temperature thresholds and fan speed settings from configuration.
 * Sets up MQTT subscriptions and publishes initial configuration.
 * 
 * @return true if initialization was successful, false otherwise
 */
bool TempMonitorAndCooling::initialize() {
    try {
        // Load temperature thresholds and fan speed settings
        const auto& temp_monitor = config_["TemperatureMonitor"];
        temp_threshold_low_ = temp_monitor["MinTemp"].as<double>();
        temp_threshold_high_ = temp_monitor["MaxTemp"].as<double>();
        fan_speed_min_ = temp_monitor["MinDutyCycle"].as<int>();
        fan_speed_max_ = temp_monitor["MaxDutyCycle"].as<int>();
        update_interval_ms_ = temp_monitor["UpdateIntervalMs"].as<int>();

        logger_->info("Loaded temperature thresholds: " + std::to_string(temp_threshold_low_) + 
                     "°C - " + std::to_string(temp_threshold_high_) + "°C");
        logger_->info("Loaded fan speed range: " + std::to_string(fan_speed_min_) + 
                     "% - " + std::to_string(fan_speed_max_) + "%");

        // Subscribe to temperature topics
        std::string topic = "sensors/+/temperature";
        if (!mqtt_client_->subscribe(topic, 0)) {  // QoS level 0
            logger_->error("Failed to subscribe to temperature topics");
            return false;
        }

        // Set message callback
        mqtt_client_->set_message_callback(&TempMonitorAndCooling::mqtt_message_callback, this);

        // Publish initial configuration
        json config_data = {
            {"status", "initialized"},
            {"mcu_count", mcu_configs_.size()},
            {"temp_threshold_low", temp_threshold_low_},
            {"temp_threshold_high", temp_threshold_high_},
            {"fan_speed_min", fan_speed_min_},
            {"fan_speed_max", fan_speed_max_},
            {"timestamp", std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())}
        };
        mqtt_client_->publish("temp_monitor/config", config_data.dump());

        logger_->info("Temperature Monitor initialized successfully with " + std::to_string(mcu_configs_.size()) + " MCUs");
        return true;
    } catch (const std::exception& e) {
        logger_->error("Error initializing temperature monitor: " + std::string(e.what()));
        return false;
    }
}

/**
 * @brief Loads MCU configurations from YAML
 * 
 * Parses the YAML configuration to load MCU settings including number of sensors
 * and sensor interface types.
 * 
 * @return true if all configurations were loaded successfully, false otherwise
 */
bool TempMonitorAndCooling::load_mcu_configs() {
    try {
        const auto& mcus = config_["MCUs"];
        for (const auto& mcu : mcus) {
            MCUConfig config;
            config.name = mcu.first.as<std::string>();
            config.number_of_sensors = mcu.second["NumberOfSensors"].as<int>();

            // Load sensor configurations
            for (const auto& sensor : mcu.second["Sensors"]) {
                // Extract sensor ID from the key (e.g., "Sensor1" -> 1)
                std::string sensor_key = sensor.first.as<std::string>();
                int sensor_id = std::stoi(sensor_key.substr(6)); // Remove "Sensor" prefix
                std::string sensor_type = sensor.second["Interface"].as<std::string>();
                config.sensor_configs[sensor_id] = sensor_type;
            }

            mcu_configs_[config.name] = config;
            logger_->debug("Loaded MCU configuration: " + config.name + 
                         " with " + std::to_string(config.number_of_sensors) + " sensors");
        }
        logger_->info("Successfully loaded " + std::to_string(mcu_configs_.size()) + " MCU configurations");
        return true;
    } catch (const std::exception& e) {
        logger_->error("Error loading MCU configurations: " + std::string(e.what()));
        return false;
    }
}

/**
 * @brief Processes a temperature reading
 * 
 * Updates the temperature history and calculates required fan speed based on
 * the current temperature.
 * 
 * @param mcu_name Name of the MCU
 * @param sensor_id ID of the temperature sensor
 * @param temperature Current temperature reading
 * @param status Status of the temperature reading
 */
void TempMonitorAndCooling::process_temperature_reading(
    const std::string& mcu_name, int sensor_id, double temperature, const std::string& status) {

    {
        std::lock_guard<std::mutex> lock(history_mutex_);
        auto& history = temperature_history_[mcu_name][sensor_id];
        
        // Add new reading
        TemperatureReading reading{
            mcu_name,
            sensor_id,
            temperature,
            status,
            std::chrono::system_clock::now()
        };
        history.readings.push_back(reading);

        // Remove old readings
        auto cutoff_time = std::chrono::system_clock::now() - history.history_duration;
        while (!history.readings.empty() && history.readings.front().timestamp < cutoff_time) {
            history.readings.pop_front();
        }
    }

    // Calculate and set new fan speed
    int new_speed = calculate_fan_speed();
    logger_->debug("Calculated new fan speed: " + std::to_string(new_speed) + "% based on temperature: " + 
                  std::to_string(temperature) + "°C");

    if (fan_simulator_) {
        if (fan_simulator_->set_fan_speed(new_speed)) {
            logger_->info("Updated fan speed to " + std::to_string(new_speed) + "%");
        } else {
            logger_->error("Failed to update fan speed");
        }
    } else {
        logger_->warning("Fan simulator not available for speed control");
    }

    // Publish temperature update
    json temp_data = {
        {"mcu", mcu_name},
        {"sensor_id", sensor_id},
        {"temperature", temperature},
        {"status", status},
        {"fan_speed", new_speed},
        {"timestamp", std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())}
    };
    mqtt_client_->publish("temp_monitor/reading", temp_data.dump());
}

/**
 * @brief Calculates the required fan speed based on temperature
 * 
 * Uses a linear interpolation between minimum and maximum fan speeds
 * based on the current temperature relative to the configured thresholds.
 * 
 * @param temperature Current temperature reading
 * @return Required fan speed as a duty cycle percentage
 */
int TempMonitorAndCooling::calculate_fan_speed() const {
    std::lock_guard<std::mutex> lock(history_mutex_);
    // Find highest temperature across all sensors
    double max_temp = -1.0;
    for (const auto& mcu : temperature_history_) {
        for (const auto& sensor : mcu.second) {
            //avoid bad or noisysensors
            if (sensor.second.readings.back().status != "Good") {
                logger_->debug("Sensor " + std::to_string(sensor.first) + " is not good, skipping");
                continue;
            }
            max_temp = std::max(max_temp, sensor.second.readings.back().temperature);
   
        }
    }

    if (max_temp < 0.0) {
        logger_->debug("No temperature readings available, using minimum fan speed");
        return fan_speed_min_;
    }

    // Linear interpolation between thresholds
    int speed;
    if (max_temp <= temp_threshold_low_) {
        speed = fan_speed_min_;
    } else if (max_temp >= temp_threshold_high_) {
        speed = fan_speed_max_;
    } else {
        double ratio = (max_temp - temp_threshold_low_) / (temp_threshold_high_ - temp_threshold_low_);
        speed = fan_speed_min_ + static_cast<int>(ratio * (fan_speed_max_ - fan_speed_min_));
    }
    logger_->debug("Calculated fan speed: " + std::to_string(speed) + "% for max temperature: " + 
                  std::to_string(max_temp) + "°C");
    return speed;
}

/**
 * @brief Callback function for MQTT messages
 * 
 * Processes incoming temperature messages from MQTT and updates the temperature history.
 * 
 * @param mosq Mosquitto instance
 * @param obj User data (TempMonitorAndCooling instance)
 * @param msg Received MQTT message
 */
void TempMonitorAndCooling::mqtt_message_callback(
    struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg) {
    auto* monitor = static_cast<TempMonitorAndCooling*>(obj);
    if (!monitor) return;

    try {
        auto json = nlohmann::json::parse(static_cast<const char*>(msg->payload));
        std::string mcu_name = json["MCU"];
        for (const auto& sensor : json["SensorData"]) {
            int sensor_id = sensor["SensorID"];
            double temperature = sensor["Value"];
            std::string status = sensor["Status"];
            monitor->process_temperature_reading(mcu_name, sensor_id, temperature, status);
        }
    } catch (const std::exception& e) {
        monitor->logger_->error("Error processing MQTT message: " + std::string(e.what()));
    }
}

/**
 * @brief Main thread function for the temperature monitor
 * 
 * Runs in a loop while the monitor is active, handling any background tasks.
 */
void TempMonitorAndCooling::main_thread_function() {
    logger_->info("Temperature Monitor main thread started");
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    logger_->info("Temperature Monitor main thread stopped");
}

} // namespace fan_control_system 