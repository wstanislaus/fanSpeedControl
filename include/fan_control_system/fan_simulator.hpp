#pragma once

#include <memory>
#include <thread>
#include <atomic>
#include <string>
#include <map>
#include <mutex>
#include <yaml-cpp/yaml.h>
#include <mosquitto.h>
#include "common/mqtt_client.hpp"
#include "common/logger.hpp"
#include "fan_control_system/fan.hpp"
#include "common/alarm.hpp"

namespace fan_control_system {

/**
 * @struct FanModel
 * @brief Configuration structure for a fan model
 */
struct FanModel {
    std::string name;           ///< Name of the fan model
    int number_of_fans;         ///< Number of fans in this model
    int pwm_min;               ///< Minimum PWM value for the fan
    int pwm_max;               ///< Maximum PWM value for the fan
    int duty_cycle_min;        ///< Minimum duty cycle percentage
    int duty_cycle_max;        ///< Maximum duty cycle percentage
    std::string interface;     ///< Interface type (e.g., "I2C")
    uint8_t pwm_reg;          ///< PWM register address
    std::map<int, int> noise_profile; ///< Mapping of duty cycle to noise level in dB
};

/**
 * @struct FanController
 * @brief Configuration structure for a fan controller
 */
struct FanController {
    std::string name;          ///< Name of the fan controller
    std::string model;         ///< Model name of the fan
    std::string mode;          ///< Operating mode
    int set_duty_cycle;        ///< Current set duty cycle
    uint8_t i2c_address;       ///< I2C address of the controller
};

/**
 * @enum NoiseLevel
 * @brief Defines noise level thresholds in decibels
 */
enum class NoiseLevel {
    QUIET = 40,          ///< 30-40 dB - Normal background noise
    MODERATE = 50,       ///< 40-50 dB - Noticeable but acceptable
    LOUD = 60,           ///< 50-60 dB - Loud but manageable
    VERY_LOUD = 70,      ///< 60-70 dB - Very loud, may be uncomfortable
    EXTREMELY_LOUD = 80, ///< 70-80 dB - Extremely loud, hearing protection recommended
    PAINFULLY_LOUD = 90, ///< 80-90 dB - Painfully loud, immediate action required
    DANGEROUS = 91       ///< >90 dB - Dangerous levels, immediate shutdown required
};

/**
 * @class FanSimulator
 * @brief Simulates multiple fans with configurable models and noise profiles
 * 
 * This class manages a collection of fan instances, each with its own model configuration
 * and noise profile. It handles fan speed control, status monitoring, and noise level
 * management through MQTT communication.
 */
class FanSimulator {
public:
    /**
     * @brief Constructs a new Fan Simulator instance
     * @param config YAML configuration node containing fan models and controllers
     * @param mqtt_settings MQTT communication settings
     */
    explicit FanSimulator(const YAML::Node& config, const common::MQTTClient::Settings& mqtt_settings);

    /**
     * @brief Destructor that ensures proper cleanup of all fan instances
     */
    ~FanSimulator();

    /**
     * @brief Starts the fan simulator
     * @return true if the simulator started successfully, false otherwise
     */
    bool start();

    /**
     * @brief Stops the fan simulator
     * @note This will stop all fan monitoring and MQTT publishing
     */
    void stop();

    /**
     * @brief Sets the fan speed for all controllers
     * @param duty_cycle New duty cycle value (0-100)
     * @return true if the operation was successful, false otherwise
     */
    bool set_fan_speed(int duty_cycle);

    /**
     * @brief Gets the current fan speed for a specific controller
     * @param controller_name Name of the fan controller
     * @return Current duty cycle value (0-100)
     */
    int get_fan_speed(const std::string& controller_name) const;

    /**
     * @brief Gets a fan instance by name
     * @param name Name of the fan to retrieve
     * @return Shared pointer to the fan instance if found, nullptr otherwise
     */
    std::shared_ptr<Fan> get_fan(const std::string& name);

    /**
     * @brief Makes a fan report bad status (for testing)
     * @param name Name of the fan to make bad
     * @return true if the operation was successful, false otherwise
     */
    bool make_fan_bad(const std::string& name);

    /**
     * @brief Makes a fan report good status (for testing)
     * @param name Name of the fan to make good
     * @return true if the operation was successful, false otherwise
     */
    bool make_fan_good(const std::string& name);

private:
    /**
     * @brief Initializes MQTT connection and components
     * @return true if initialization was successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Loads fan models from configuration
     * @return true if models were loaded successfully, false otherwise
     */
    bool load_fan_models();

    /**
     * @brief Loads fan controllers from configuration
     * @return true if controllers were loaded successfully, false otherwise
     */
    bool load_fan_controllers();

    /**
     * @brief Creates fan instances based on configuration
     * @return true if all fans were created successfully, false otherwise
     */
    bool create_fans();

    /**
     * @brief Converts duty cycle to PWM counts for a specific model
     * @param model Name of the fan model
     * @param duty_cycle Duty cycle value (0-100)
     * @return Corresponding PWM count value
     */
    int duty_cycle_to_pwm(const std::string& model, int duty_cycle) const;

    /**
     * @brief Converts PWM counts to duty cycle for a specific model
     * @param model Name of the fan model
     * @param pwm PWM count value
     * @return Corresponding duty cycle value (0-100)
     */
    int pwm_to_duty_cycle(const std::string& model, int pwm) const;

    /**
     * @brief Gets noise level from duty cycle based on noise profile
     * @param model Name of the fan model
     * @param duty_cycle Current duty cycle value
     * @return Noise level in decibels
     */
    int get_noise_level(const std::string& model, int duty_cycle) const;

    /**
     * @brief Checks for noise conditions and raises alarms if necessary
     */
    void check_noise_condition();

    /**
     * @brief Main thread function for fan monitoring
     */
    void main_thread_function();

    // Configuration
    YAML::Node config_;                                    ///< Loaded configuration data
    
    // Fan models and controllers
    std::map<std::string, FanModel> fan_models_;          ///< Map of fan model configurations
    std::map<std::string, std::shared_ptr<Fan>> fans_;    ///< Map of fan instances
    
    // Current fan speeds
    std::map<std::string, int> current_speeds_;           ///< Current speed for each fan
    mutable std::mutex speeds_mutex_;                      ///< Mutex for thread-safe speed access

    // MQTT settings and components
    common::MQTTClient::Settings mqtt_settings_;           ///< MQTT communication settings
    std::shared_ptr<common::MQTTClient> mqtt_client_;      ///< MQTT client for communication
    std::unique_ptr<common::Logger> logger_;               ///< Logger for fan simulator

    // Thread control
    std::atomic<bool> running_;                           ///< Flag indicating if simulator is running
    std::thread main_thread_;                             ///< Main monitoring thread
    
    // Log Level
    std::string log_level_;                               ///< Log level for the simulator

    // Name
    std::string name_;                                    ///< Name of the fan simulator

    // Noise level threshold for raising an alarm
    int fans_too_loud_threshold_;                         ///< Threshold for noise level alarms

    // Alarm system
    std::unique_ptr<common::Alarm> alarm_;                ///< Alarm system for noise conditions

    // Noise monitoring
    std::chrono::system_clock::time_point last_loud_noise_start_time_;  ///< Start time of current loud noise period
    bool is_it_loud_;                                     ///< Flag indicating if noise is currently loud
};

} // namespace fan_control_system 