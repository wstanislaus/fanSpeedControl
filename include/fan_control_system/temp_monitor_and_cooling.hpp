#pragma once

#include <memory>
#include <thread>
#include <atomic>
#include <string>
#include <map>
#include <mutex>
#include <deque>
#include <chrono>
#include <yaml-cpp/yaml.h>
#include <mosquitto.h>
#include "fan_control_system/fan_simulator.hpp"
#include "common/mqtt_client.hpp"
#include "common/logger.hpp"

namespace fan_control_system {

/**
 * @struct TemperatureReading
 * @brief Structure representing a single temperature reading from a sensor
 */
struct TemperatureReading {
    std::string mcu_name;     ///< Name of the MCU providing the reading
    int sensor_id;            ///< ID of the sensor providing the reading
    double temperature;       ///< Temperature value in degrees Celsius
    std::string status;       ///< Status of the reading (e.g., "GOOD", "BAD", "NOISY")
    std::chrono::system_clock::time_point timestamp;  ///< Timestamp of the reading
};

/**
 * @struct MCUConfig
 * @brief Configuration structure for an MCU and its sensors
 */
struct MCUConfig {
    std::string name;                         ///< Name of the MCU
    int number_of_sensors;                    ///< Number of temperature sensors
    std::map<int, std::string> sensor_configs; ///< Configuration for each sensor
};

/**
 * @struct TemperatureHistory
 * @brief Structure for storing temperature reading history
 */
struct TemperatureHistory {
    std::deque<TemperatureReading> readings;  ///< Queue of temperature readings
    std::chrono::minutes history_duration;    ///< Duration to keep readings
};

/**
 * @class TempMonitorAndCooling
 * @brief Monitors system temperatures and controls fan speeds for cooling
 * 
 * This class manages temperature monitoring across multiple MCUs and their sensors,
 * maintains temperature history, and controls fan speeds based on temperature readings.
 * It uses MQTT for receiving temperature data and controlling fan speeds.
 */
class TempMonitorAndCooling {
public:
    /**
     * @brief Constructs a new Temperature Monitor and Cooling instance
     * @param config YAML configuration node containing MCU and sensor settings
     * @param mqtt_settings MQTT communication settings
     * @param fan_simulator Shared pointer to the fan simulator for speed control
     */
    explicit TempMonitorAndCooling(const YAML::Node& config, 
                                 const common::MQTTClient::Settings& mqtt_settings,
                                 std::shared_ptr<FanSimulator> fan_simulator);

    /**
     * @brief Destructor that ensures proper cleanup
     */
    ~TempMonitorAndCooling();

    /**
     * @brief Starts the temperature monitor
     * @return true if the monitor started successfully, false otherwise
     */
    bool start();

    /**
     * @brief Stops the temperature monitor
     * @note This will stop all temperature monitoring and fan control
     */
    void stop();

    /**
     * @brief Gets the current temperature for a specific MCU and sensor
     * @param mcu_name Name of the MCU
     * @param sensor_id ID of the sensor
     * @return Current temperature in degrees Celsius
     */
    double get_temperature(const std::string& mcu_name, int sensor_id) const;

    /**
     * @brief Gets temperature history for a specific MCU and sensor
     * @param mcu_name Name of the MCU
     * @param sensor_id ID of the sensor
     * @return Deque containing temperature readings with timestamps
     */
    std::deque<TemperatureReading> get_temperature_history(const std::string& mcu_name, int sensor_id) const;

private:
    /**
     * @brief Initializes MQTT connection and components
     * @return true if initialization was successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Loads MCU configurations from the YAML node
     * @return true if configurations were loaded successfully, false otherwise
     */
    bool load_mcu_configs();

    /**
     * @brief Processes a new temperature reading
     * @param mcu_name Name of the MCU providing the reading
     * @param sensor_id ID of the sensor providing the reading
     * @param temperature Temperature value in degrees Celsius
     * @param status Status of the reading (e.g., "GOOD", "BAD", "NOISY")
     */
    void process_temperature_reading(const std::string& mcu_name, int sensor_id, double temperature, const std::string& status);

    /**
     * @brief Calculates required fan speed based on current temperatures
     * @return Required fan speed as a percentage (0-100)
     */
    int calculate_fan_speed() const;

    /**
     * @brief MQTT message callback for receiving temperature data
     * @param mosq Pointer to the mosquitto instance
     * @param obj User data pointer
     * @param msg Pointer to the received message
     */
    static void mqtt_message_callback(struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg);

    /**
     * @brief Main thread function for temperature monitoring
     */
    void main_thread_function();

    // Configuration
    YAML::Node config_;                                    ///< Loaded configuration data
    
    // MCU configurations
    std::map<std::string, MCUConfig> mcu_configs_;        ///< Map of MCU configurations
    
    // Temperature history
    std::map<std::string, std::map<int, TemperatureHistory>> temperature_history_;  ///< Temperature history for each sensor
    mutable std::mutex history_mutex_;                     ///< Mutex for thread-safe history access

    // Fan simulator reference
    std::shared_ptr<FanSimulator> fan_simulator_;          ///< Fan simulator for speed control

    // MQTT settings and components
    common::MQTTClient::Settings mqtt_settings_;           ///< MQTT communication settings
    std::shared_ptr<common::MQTTClient> mqtt_client_;      ///< MQTT client for communication

    // Common components
    std::unique_ptr<common::Logger> logger_;               ///< Logger for temperature monitor
    
    // Thread control
    std::thread main_thread_;                             ///< Main monitoring thread
    std::atomic<bool> running_{false};                    ///< Flag indicating if monitor is running

    // Temperature thresholds
    double temp_threshold_low_{25.0};                     ///< Below this, fans run at 20%
    double temp_threshold_high_{75.0};                    ///< Above this, fans run at 100%
    int fan_speed_min_{20};                              ///< Minimum fan speed percentage
    int fan_speed_max_{100};                             ///< Maximum fan speed percentage
    int update_interval_ms_{1000};                       ///< Update interval in milliseconds

    // Name
    std::string name_;                                    ///< Name of the temperature monitor
};

} // namespace fan_control_system 