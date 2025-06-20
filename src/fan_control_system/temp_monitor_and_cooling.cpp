#include "fan_control_system/temp_monitor_and_cooling.hpp"
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <cmath>
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
    alarm_ = std::make_unique<common::Alarm>(name_, mqtt_client_);

    if (!load_mcu_configs()) {
        throw std::runtime_error("Failed to initialize temperature monitor");
    }
    cooling_status_.average_temperature = 0.0;
    cooling_status_.current_fan_speed = 0;
    cooling_status_.cooling_mode = "MANUAL";
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
 * @param max_readings Maximum number of readings to return
 * @return Deque of temperature readings, empty if no history available
 */
std::deque<TemperatureReading> TempMonitorAndCooling::get_temperature_history(
    const std::string& mcu_name, int sensor_id, int max_readings) const {
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

    // Create a list for the max readings specified and return it
    std::deque<TemperatureReading> history; 
    // Check if the size of the history is greater than the max readings, if so, return the until max readings, otherwise, return until available
    if (sensor_it->second.readings.size() <= max_readings) {
        return sensor_it->second.readings;
    }
    for (const auto& reading : sensor_it->second.readings) {
        history.push_back(reading);
        if (history.size() >= max_readings) {
            break;
        }
    }
    logger_->debug("Retrieved temperature history for MCU: " + mcu_name + ", Sensor: " + std::to_string(sensor_id) + 
                  ", Readings: " + std::to_string(history.size()));
    return history;
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

        // Load temperature history duration
        int history_duration_minutes = config_["TemperatureHistoryDurationMinutes"].as<int>();
        history_duration_ = std::chrono::minutes(history_duration_minutes);

        // Load standard deviation threshold from TemperatureSettings
        const auto& temp_settings = config_["TemperatureSettings"];
        std_dev_threshold_ = temp_settings["ErraticThreshold"].as<double>();

        logger_->info("Loaded temperature thresholds: " + std::to_string(temp_threshold_low_) + 
                     "°C - " + std::to_string(temp_threshold_high_) + "°C");
        logger_->info("Loaded fan speed range: " + std::to_string(fan_speed_min_) + 
                     "% - " + std::to_string(fan_speed_max_) + "%");
        logger_->info("Loaded temperature history duration: " + std::to_string(history_duration_minutes) + " minutes");
        logger_->info("Loaded standard deviation threshold: " + std::to_string(std_dev_threshold_) + "°C");

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
            {"history_duration_minutes", history_duration_minutes},
            {"std_dev_threshold", std_dev_threshold_},
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
        
        // Initialize history duration if this is the first reading for this sensor
        if (history.readings.empty()) {
            history.history_duration = history_duration_;
        }
        
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
}

/**
 * @brief Calculates the required fan speed based on temperature
 * 
 * Uses a linear interpolation between minimum and maximum fan speeds
 * based on the current temperature relative to the configured thresholds.
 * 
 * @param temperature Current temperature reading
 * @return Cooling status
 */
CoolingStatus TempMonitorAndCooling::calculate_fan_speed() const {
    CoolingStatus status;
    status.current_fan_speed = 0;
    status.cooling_mode = "MANUAL";
    status.average_temperature = 0.0;
    std::lock_guard<std::mutex> lock(history_mutex_);
    // Find highest temperature across all sensors
    double max_temp = -1.0;
    for (const auto& mcu : temperature_history_) {
        // Calculate mean and standard deviations of all the sensors for a given MCU
        // If the standard deviation is too high, raise an alarm and skip this MCU as bad readings
        std::vector<double> temperatures;
        int num_readings = 0;
        
        // First pass: collect all valid temperature readings
        for (const auto& sensor : mcu.second) {
            if (sensor.second.readings.empty()) {
                logger_->debug("MCU " + mcu.first + " Sensor " + std::to_string(sensor.first) + " has no readings");
                continue;
            }
            const auto& latest_reading = sensor.second.readings.back();
            if (latest_reading.status != "Good") {
                logger_->debug("MCU " + mcu.first + " Sensor " + std::to_string(sensor.first) + " is not good, skipping");
                continue;
            }
            temperatures.push_back(latest_reading.temperature);
            num_readings++;
            logger_->debug("MCU " + mcu.first + " Sensor " + std::to_string(sensor.first) + " temperature: " + std::to_string(latest_reading.temperature) + "°C");
        }
        
        // Check if we have enough readings
        if (num_readings < 2) {
            logger_->debug("MCU " + mcu.first + " has insufficient readings (" + std::to_string(num_readings) + "), skipping");
            continue;
        }
        
        // Calculate mean
        double mean = 0.0;
        for (double temp : temperatures) {
            mean += temp;
        }
        mean /= num_readings;
        
        // Calculate standard deviation
        double variance = 0.0;
        for (double temp : temperatures) {
            variance += std::pow(temp - mean, 2);
        }
        double std_dev = std::sqrt(variance / num_readings);
        
        // Additional safety check for NaN or infinite values
        if (std::isnan(std_dev) || std::isinf(std_dev)) {
            logger_->warning("MCU " + mcu.first + " has invalid standard deviation (NaN or inf), skipping");
            continue;
        }
        
        // Debug logging to show the readings being used
        std::string temp_list = "Temperatures: ";
        for (size_t i = 0; i < temperatures.size(); ++i) {
            if (i > 0) temp_list += ", ";
            temp_list += std::to_string(temperatures[i]) + "°C";
        }
        logger_->debug("MCU " + mcu.first + " - " + temp_list + " | Mean: " + std::to_string(mean) + "°C | StdDev: " + std::to_string(std_dev) + "°C");
        
        if (std_dev > std_dev_threshold_) {
            logger_->debug("MCU " + mcu.first + " has high standard deviation: " + std::to_string(std_dev));
            alarm_->raise(common::AlarmSeverity::HIGH, "MCU " + mcu.first + " has high standard deviation: " + std::to_string(std_dev) + "°C, mean: " + std::to_string(mean) + "°C, hence skipping");
            continue;
        }
        max_temp = std::max(max_temp, mean);
    }

    // If no good readings, return minimum fan speed
    if (max_temp < 0.0) {
        logger_->debug("No temperature readings available, using minimum fan speed");
        status.current_fan_speed = fan_speed_min_;
        return status;
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
    status.current_fan_speed = speed;
    status.average_temperature = max_temp;
    return status;
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
 * @brief Sets the temperature thresholds
 * @param temp_threshold_low Low temperature threshold
 * @param temp_threshold_high High temperature threshold
 * @param fan_speed_min Minimum fan speed
 * @param fan_speed_max Maximum fan speed
 */
void TempMonitorAndCooling::set_thresholds(double temp_threshold_low, double temp_threshold_high, int fan_speed_min, int fan_speed_max) {
    temp_threshold_low_ = temp_threshold_low;
    temp_threshold_high_ = temp_threshold_high;
    fan_speed_min_ = fan_speed_min;
    fan_speed_max_ = fan_speed_max;
    logger_->info("Temperature thresholds set to: " + std::to_string(temp_threshold_low) + "°C - " + std::to_string(temp_threshold_high) + "°C");
    logger_->info("Fan speed range set to: " + std::to_string(fan_speed_min) + "% - " + std::to_string(fan_speed_max) + "%");
}

void TempMonitorAndCooling::update_fan_speed() {
    // Calculate and set new fan speed
    CoolingStatus new_status = calculate_fan_speed();
    // Check if current fan speed is different by 10% from the new fan speed, then only update the fan speed
    // or the temperature is different by 5°C, then only update the fan speed
    if (std::abs(cooling_status_.current_fan_speed - new_status.current_fan_speed) > 10 || 
        std::abs(cooling_status_.average_temperature - new_status.average_temperature) > 5.0) {
        cooling_status_.current_fan_speed = new_status.current_fan_speed;
        cooling_status_.average_temperature = new_status.average_temperature;
        cooling_status_.cooling_mode = "MANUAL";
    } else {
        logger_->debug("No need to update fan speed or temperature");
        return;
    }

    if (fan_simulator_) {
        if (fan_simulator_->set_fan_speed(new_status.current_fan_speed)) {
            logger_->info("Updated fan speed to " + std::to_string(new_status.current_fan_speed) + "%");
        } else {
            logger_->error("Failed to update fan speed");
        }
    } else {
        logger_->warning("Fan simulator not available for speed control");
    }

    // Publish temperature update
    json temp_data = {
        {"cooling_mode", new_status.cooling_mode},
        {"average_temperature", new_status.average_temperature},
        {"current_fan_speed", new_status.current_fan_speed},
        {"timestamp", std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())}
    };
    mqtt_client_->publish("temp_monitor/cooling_status", temp_data.dump());
}

/**
 * @brief Main thread function for the temperature monitor
 * 
 * Runs in a loop while the monitor is active, handling any background tasks.
 */
void TempMonitorAndCooling::main_thread_function() {
    logger_->info("Temperature Monitor main thread started");
    while (running_) {
        update_fan_speed();
        std::this_thread::sleep_for(std::chrono::milliseconds(update_interval_ms_));
    }
    logger_->info("Temperature Monitor main thread stopped");
}

} // namespace fan_control_system 