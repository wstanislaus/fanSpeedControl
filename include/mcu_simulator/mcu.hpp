#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <deque>
#include <mosquitto.h>
#include <yaml-cpp/yaml.h>
#include "common/config.hpp"

namespace common {
    class MQTTClient;
    class Logger;
    class Alarm;
}

namespace mcu_simulator {

class TemperatureSensor;

// Forward declarations
struct TemperatureSettings;

/**
 * @class MCU
 * @brief Represents a Microcontroller Unit with multiple temperature sensors and MQTT communication capabilities
 */
class MCU {
public:
    /**
     * @struct TemperatureSettings
     * @brief Configuration settings for temperature monitoring and publishing
     */
    struct TemperatureSettings {
        float bad_threshold;                                     ///< Threshold for bad temperature readings
        float erratic_threshold;                                 ///< Threshold for erratic temperature readings
        struct PublishInterval {
            float min_temp;                                      ///< Minimum temperature for this interval
            float max_temp;                                      ///< Maximum temperature for this interval
            int interval_seconds;                                ///< Publishing interval in seconds for this temperature range
        };
        std::vector<PublishInterval> publish_intervals;          ///< List of temperature ranges and their publish intervals
    };

    /**
     * @brief Constructs a new MCU instance
     * @param name Name of the MCU
     * @param num_sensors Number of temperature sensors to create
     * @param temp_settings Temperature monitoring settings
     * @param mqtt_settings MQTT communication settings
     * @param sensor_config YAML node containing sensor configuration
     */
    MCU(const std::string& name, int num_sensors, const TemperatureSettings& temp_settings,
        const common::MQTTClient::Settings& mqtt_settings, const YAML::Node& sensor_config,
        const std::string& config_file);

    /**
     * @brief Destructor that ensures proper cleanup
     */
    ~MCU();

    /**
     * @brief Initializes the MCU and its temperature sensors
     * @return true if initialization was successful, false otherwise
     */
    bool initialize();
    
    /**
     * @brief Starts the temperature reading and publishing loop
     * @note This will begin continuous temperature monitoring and MQTT publishing
     */
    void start();
    
    /**
     * @brief Stops the temperature reading and publishing loop
     * @note This will stop all temperature monitoring and MQTT publishing
     */
    void stop();

    /**
     * @brief Gets the name of the MCU
     * @return The MCU's name
     */
    const std::string& getName() const { return name_; }

    /**
     * @brief Gets the number of temperature sensors on this MCU
     * @return Number of sensors
     */
    int getNumSensors() const { return sensors_.size(); }

    /**
     * @brief Makes a specific sensor report bad readings (for testing)
     * @param sensor_id ID of the sensor to make bad
     * @param is_bad true to set the sensor to bad, false to set it to good
     * @return true if the operation was successful, false otherwise
     */
    bool makeSensorBad(int sensor_id, bool is_bad);

    /**
     * @brief Makes a specific sensor report noisy readings (for testing)
     * @param sensor_id ID of the sensor to make noisy
     * @param is_noisy true to set the sensor to noisy, false to set it to normal
     * @return true if the operation was successful, false otherwise
     */
    bool makeSensorNoisy(int sensor_id, bool is_noisy);

    /**
     * @brief Formats a timestamp to a human-readable string
     * @param tp The timestamp to format
     * @return Formatted timestamp string
     */
    std::string formatTimestamp(const std::chrono::system_clock::time_point& tp);

    /**
     * @brief Gets the temperature from a specific sensor
     * @param sensor_id ID of the sensor
     * @param temperature Output parameter for the temperature reading
     * @return true if the reading is valid, false otherwise
     */
    bool getSensorTemperature(const std::string& sensor_id, double& temperature);

    /**
     * @brief Checks if the MCU is online
     * @return true if the MCU is online, false otherwise
     */
    bool isOnline() const { return running_; }

    /**
     * @brief Gets the number of active sensors
     * @return Number of active sensors
     */
    int getActiveSensorCount() const;

    /**
     * @brief Gets all sensors in this MCU
     * @return Vector of temperature sensors
     */
    const std::vector<std::unique_ptr<TemperatureSensor>>& getSensors() const { return sensors_; }

    /**
     * @brief Sets the MCU's fault state
     * @param is_faulty true to set the MCU as faulty, false to set it as normal
     */
    void setFaulty(bool is_faulty);

    /**
     * @brief Gets the MCU's fault state
     * @return true if the MCU is faulty, false otherwise
     */
    bool isFaulty() const { return is_faulty_; }

    /**
     * @brief Sets the simulation parameters for a specific sensor
     * @param sensor_id ID of the sensor
     * @param start_temp Start temperature for simulation
     * @param end_temp End temperature for simulation
     * @param step_size Step size for simulation
     */
    bool setSimulationParams(int sensor_id, double start_temp, double end_temp, double step_size);

private:
    /**
     * @brief Reads temperatures from all sensors and publishes them
     * @note This is called periodically by the main loop
     */
    void readAndPublishTemperatures();

    /**
     * @brief Publishes temperature data via MQTT
     * @note This includes all sensor readings and metadata
     */
    void publishTemperatureData();

    /**
     * @brief Checks if a series of temperature readings shows erratic behavior
     * @param readings Deque containing the last 5 temperature readings
     * @return true if readings are erratic, false otherwise
     */
    bool checkErraticReadings(const std::deque<float>& readings);

    /**
     * @brief Calculates the next publish interval based on current temperature
     * @param temperature Current temperature reading
     * @return Time interval until next publish
     */
    std::chrono::seconds calculatePublishInterval(float temperature);

    /**
     * @brief Generates an alarm if the sensors are bad or noisy, if already raised, clear the alarm
     */
    void checkAlarm();

    std::string name_;                                          ///< Name of the MCU
    std::vector<std::unique_ptr<TemperatureSensor>> sensors_;   ///< Vector of temperature sensors
    bool running_;                                              ///< Flag indicating if the MCU is running
    std::chrono::system_clock::time_point last_read_time_;      ///< Timestamp of last temperature reading
    std::shared_ptr<common::MQTTClient> mqtt_client_;           ///< MQTT client for communication
    std::unique_ptr<common::Logger> logger_;                    ///< Logger for MCU-level logging
    std::unique_ptr<common::Alarm> alarm_;                      ///< Alarm system for MCU-level alerts
    bool alarm_raised_;                                         ///< Flag indicating if an alarm has been raised
    bool is_faulty_ = false;                                    ///< Flag indicating if the MCU is in a faulty state

    std::vector<std::deque<float>> sensor_readings_;           ///< Temperature reading history for each sensor (last 5 readings)
    
    TemperatureSettings temp_settings_;                         ///< Temperature monitoring settings
    common::MQTTClient::Settings mqtt_settings_;                               ///< MQTT communication settings
    std::string config_file_;                                   ///< Path to the configuration file
};

} // namespace mcu_simulator 