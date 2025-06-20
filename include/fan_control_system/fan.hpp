#pragma once

#include <string>
#include <memory>
#include <chrono>
#include <yaml-cpp/yaml.h>
#include "common/mqtt_client.hpp"
#include "common/logger.hpp"
#include "common/alarm.hpp"

namespace fan_control_system {

/**
 * @class Fan
 * @brief Represents a single fan instance with I2C interface and status monitoring
 * 
 * This class manages a single fan instance, providing control over its speed through
 * PWM signals and monitoring its status. It supports I2C communication for speed
 * control and status reporting, with MQTT integration for remote monitoring and control.
 */
class Fan {
public:
    /**
     * @brief Constructs a new Fan instance
     * @param name Name of the fan
     * @param model_name Name of the fan model
     * @param i2c_address I2C address of the fan controller
     * @param pwm_reg PWM register address for speed control
     * @param mqtt_settings MQTT communication settings
     * @param log_level Logging level for the fan
     */
    Fan(const std::string& name, const std::string& model_name, uint8_t i2c_address, uint8_t pwm_reg,
        const common::MQTTClient::Settings& mqtt_settings, const std::string& log_level,
        int pwm_min, int pwm_max, int duty_cycle_min, int duty_cycle_max, const std::map<int, int>& noise_profile);

    /**
     * @brief Destructor that ensures proper cleanup
     * @note This will stop the fan and clean up MQTT resources
     */
    ~Fan();

    /**
     * @brief Initializes the fan and its components
     * @return true if initialization was successful, false otherwise
     * @note This includes setting up MQTT communication and initializing the I2C interface
     */
    bool initialize();

    /**
     * @brief Starts the fan monitoring
     * @note This will begin continuous status monitoring and MQTT publishing
     */
    void start();

    /**
     * @brief Stops the fan monitoring
     * @note This will stop all monitoring and MQTT publishing
     */
    void stop();

    /**
     * @brief Gets the name of the fan
     * @return The fan's name
     */
    const std::string& getName() const { return name_; }

    /**
     * @brief Gets the model name of the fan
     * @return The fan's model name
     */
    const std::string& getModelName() const { return model_name_; }

    /**
     * @brief Gets the interface of the fan
     * @return The fan's interface
     */
    const std::string& getInterface() const { return interface_; }

    /**
     * @brief Gets the current status of the fan
     * @return String describing the fan's current status
     * @note Status can be "GOOD", "BAD", or "NOISY"
     */
    const std::string& getStatus() const { return status_; }

    /**
     * @brief Gets the current duty cycle of the fan
     * @return Current duty cycle (0-100)
     */
    int getDutyCycle() const { return current_duty_cycle_; }

    /**
     * @brief Gets the current PWM count of the fan
     * @return Current PWM count
     */
    int getPWMCount() const { return current_pwm_count_; }

    /**
     * @brief Gets the current noise level of the fan
     * @return Current noise level (0-100)
     */
    int getNoiseLevel() const { return noise_level_; }

    /**
     * @brief Gets the I2C address of the fan
     * @return I2C address
     */
    uint8_t getI2CAddress() const { return i2c_address_; }

    /**
     * @brief Gets the PWM register address of the fan
     * @return PWM register address
     */
    uint8_t getPWMRegister() const { return pwm_reg_; }

    /**
     * @brief Gets the PWM minimum value of the fan
     * @return PWM minimum value
     */
    int getPWMMin() const { return pwm_min_; }

    /**
     * @brief Gets the PWM maximum value of the fan
     * @return PWM maximum value
     */
    int getPWMMax() const { return pwm_max_; }

    /**
     * @brief Gets the duty cycle minimum value of the fan
     * @return Duty cycle minimum value
     */
    int getDutyCycleMin() const { return duty_cycle_min_; }

    /**
    /**
     * @brief Gets the duty cycle maximum value of the fan
     * @return Duty cycle maximum value
     */
    int getDutyCycleMax() const { return duty_cycle_max_; }

    /**
     * @brief Sets the duty cycle of the fan
     * @param duty_cycle New duty cycle value (0-100)
     * @param pwm_count New pwm count value based on fan model
     * @return true if the operation was successful, false otherwise
     * @note The duty cycle is converted to PWM counts based on the fan model
     */
    bool setPwmCount(int duty_cycle, int pwm_count);

    /**
     * @brief Makes the fan report bad status (for testing)
     * @return true if the operation was successful, false otherwise
     * @note This is used for testing alarm conditions
     */
    bool makeBad();

    /**
     * @brief Makes the fan report good status (for testing)
     * @return true if the operation was successful, false otherwise
     * @note This is used for testing alarm conditions
     */
    bool makeGood();

private:
    /**
     * @brief Reads the current pwm count from the I2C register
     * @return Current pwm count value
     * @note This reads directly from the fan controller's PWM register
     */
    int readPwmCount();

    /**
     * @brief Writes the pwm count to the I2C register
     * @param pwm_count PWM count value to write
     * @return true if the operation was successful, false otherwise
     * @note This writes directly to the fan controller's PWM register
     */
    bool writePwmCount(int pwm_count);

    /**
     * @brief Publishes fan status via MQTT
     * @note This includes current speed, status, and any alarms
     */
    void publishStatus();

    std::string name_;                                          ///< Name of the fan
    std::string model_name_;                                    ///< Model name of the fan
    uint8_t i2c_address_;                                       ///< I2C address of the fan controller
    uint8_t pwm_reg_;                                          ///< PWM register address
    std::string status_;                                        ///< Current status of the fan
    int current_pwm_count_;                                    ///< Current pwm count based on the duty cycle
    int current_duty_cycle_;                                    ///< Current duty cycle based on the pwm count
    bool running_;                                              ///< Flag indicating if the fan is running
    std::chrono::system_clock::time_point last_update_time_;    ///< Timestamp of last status update

    // MQTT and logging components
    std::shared_ptr<common::MQTTClient> mqtt_client_;           ///< MQTT client for communication
    std::unique_ptr<common::Logger> logger_;                    ///< Logger for fan-level logging
    std::unique_ptr<common::Alarm> alarm_;                      ///< Alarm system for fan-level alerts
    common::MQTTClient::Settings mqtt_settings_;                ///< MQTT communication settings
    std::string log_level_;                                      ///< Log level for the fan
    int pwm_min_;                                                ///< PWM minimum value
    int pwm_max_;                                                ///< PWM maximum value
    int duty_cycle_min_;                                         ///< Duty cycle minimum value
    int duty_cycle_max_;                                         ///< Duty cycle maximum value
    int noise_level_;                                            ///< Noise level
    std::map<int, int> noise_profile_;                            ///< Noise profile
    std::string interface_;                                        ///< Interface of the fan
};

} // namespace fan_control_system 