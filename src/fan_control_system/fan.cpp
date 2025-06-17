#include "fan_control_system/fan.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace fan_control_system {

/**
 * @brief Constructs a new Fan instance
 * 
 * Initializes a fan with the specified name, model, I2C address, and PWM register.
 * Sets up MQTT settings and logging level for the fan.
 * 
 * @param name Name of the fan
 * @param model_name Model name of the fan
 * @param i2c_address I2C address of the fan controller
 * @param pwm_reg PWM register address for controlling fan speed
 * @param mqtt_settings MQTT client settings for publishing fan status
 * @param log_level Logging level for the fan
 */
Fan::Fan(const std::string& name, const std::string& model_name, uint8_t i2c_address, uint8_t pwm_reg,
         const common::MQTTClient::Settings& mqtt_settings, const std::string& log_level)
    : name_(name)
    , model_name_(model_name)
    , i2c_address_(i2c_address)
    , pwm_reg_(pwm_reg)
    , status_("Good")
    , current_pwm_count_(0)
    , current_duty_cycle_(0)
    , running_(false)
    , last_update_time_(std::chrono::system_clock::now())
    , mqtt_settings_(mqtt_settings)
    , log_level_(log_level)
{
}

/**
 * @brief Destructor for Fan instance
 * 
 * Ensures the fan is properly stopped and resources are cleaned up.
 */
Fan::~Fan() {
    stop();
}

/**
 * @brief Initializes the fan
 * 
 * Sets up MQTT client, logger, and alarm system. Publishes initial configuration
 * to MQTT broker.
 * 
 * @return true if initialization was successful, false otherwise
 */
bool Fan::initialize() {
    // Initialize MQTT client
    mqtt_client_ = std::make_shared<common::MQTTClient>(name_, mqtt_settings_);
    if (!mqtt_client_->initialize() || !mqtt_client_->connect()) {
        std::cerr << "Failed to initialize MQTT client for fan " << name_ << std::endl;
        return false;
    }

    // Initialize logger and alarm
    logger_ = std::make_unique<common::Logger>(name_, log_level_, mqtt_client_);
    alarm_ = std::make_unique<common::Alarm>(name_, mqtt_client_);

    // Publish initial configuration
    json config_data = {
        {"name", name_},
        {"model", model_name_},
        {"i2c_address", i2c_address_},
        {"pwm_reg", pwm_reg_},
        {"status", status_},
        {"timestamp", std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())}
    };
    mqtt_client_->publish("fan/" + name_ + "/config", config_data.dump());

    logger_->info("Fan initialized successfully");
    return true;
}

/**
 * @brief Starts the fan monitoring
 * 
 * Starts a background thread to monitor fan status and publish updates.
 * The thread reads the current PWM count from the I2C register and publishes
 * status changes to MQTT.
 */
void Fan::start() {
    if (running_) {
        logger_->info("Fan already running");
        return;
    }
    running_ = true;
    logger_->info("Fan started");

    // Start status monitoring thread
    std::thread([this]() {
        logger_->debug("Fan monitoring thread started");
        while (running_) {
            // Read current duty cycle from I2C register
            int pwm_count = readPwmCount();
            if (pwm_count != current_pwm_count_) {
                logger_->debug("Duty cycle changed from " + std::to_string(current_pwm_count_) + 
                             "% to " + std::to_string(pwm_count) + "%");
                current_pwm_count_ = pwm_count;
                publishStatus();
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        logger_->debug("Fan monitoring thread stopped");
    }).detach();
}

/**
 * @brief Stops the fan monitoring
 * 
 * Stops the background monitoring thread and cleans up resources.
 */
void Fan::stop() {
    if (!running_) {
        return;
    }
    running_ = false;
    logger_->info("Fan stopped");
}

/**
 * @brief Sets the fan speed using PWM count
 * 
 * Sets the fan speed by writing the specified PWM count to the I2C register.
 * Also updates the duty cycle percentage and publishes the new status.
 * 
 * @param duty_cycle Duty cycle percentage (0-100)
 * @param pwm_count PWM count value to write to the register
 * @return true if the operation was successful, false otherwise
 */
bool Fan::setPwmCount(int duty_cycle, int pwm_count) {
    if (status_ == "Bad") {
        logger_->warning("Cannot set duty cycle - fan is in bad state");
        return false;
    }

    if (duty_cycle < 0 || duty_cycle > 100) {
        logger_->warning("Invalid duty cycle value: " + std::to_string(duty_cycle));
        return false;
    }

    logger_->debug("Setting duty cycle to " + std::to_string(duty_cycle) + "%");
    if (!writePwmCount(pwm_count)) {
        logger_->error("Failed to write pwm count to I2C register");
        alarm_->raise(common::AlarmSeverity::HIGH, "Failed to write pwm count to I2C register");
        return false;
    }

    current_pwm_count_ = pwm_count;
    current_duty_cycle_ = duty_cycle;
    publishStatus();
    logger_->info("Pwm count set to " + std::to_string(pwm_count) + " for duty cycle " + std::to_string(duty_cycle) + "%");
    return true;
}

/**
 * @brief Marks the fan as bad
 * 
 * Sets the fan status to "Bad" and raises a high severity alarm.
 * Also publishes the status change to MQTT.
 * 
 * @return true if the operation was successful, false otherwise
 */
bool Fan::makeBad() {
    if (status_ == "Bad") {
        logger_->debug("Fan already in bad state");
        return true;
    }
    
    status_ = "Bad";
    logger_->warning("Fan marked as bad");
    alarm_->raise(common::AlarmSeverity::HIGH, "Fan marked as bad");
    publishStatus();
    return true;
}

/**
 * @brief Marks the fan as good
 * 
 * Sets the fan status to "Good" and publishes the status change to MQTT.
 * 
 * @return true if the operation was successful, false otherwise
 */
bool Fan::makeGood() {
    if (status_ == "Good") {
        logger_->debug("Fan already in good state");
        return true;
    }
    
    status_ = "Good";
    logger_->info("Fan marked as good");
    publishStatus();
    return true;
}

/**
 * @brief Reads the current PWM count from the I2C register
 * 
 * In a real implementation, this would read from the actual I2C register.
 * Currently simulates the reading operation.
 * 
 * @return Current PWM count value
 */
int Fan::readPwmCount() {
    // Simulate reading from I2C register
    // In a real implementation, this would read from the actual I2C register
    logger_->debug("Reading pwm count from I2C register 0x" + 
                  std::to_string(static_cast<int>(pwm_reg_)) + 
                  " at address 0x" + std::to_string(static_cast<int>(i2c_address_)));
    return current_pwm_count_;
}

/**
 * @brief Writes a PWM count to the I2C register
 * 
 * In a real implementation, this would write to the actual I2C register.
 * Currently simulates the writing operation.
 * 
 * @param pwm_count PWM count value to write
 * @return true if the operation was successful, false otherwise
 */
bool Fan::writePwmCount(int pwm_count) {
    // Simulate writing to I2C register
    // In a real implementation, this would write to the actual I2C register
    if (status_ == "Bad") {
        logger_->warning("Cannot write to I2C register - fan is in bad state");
        return false;
    }

    logger_->debug("Writing pwm count " + std::to_string(pwm_count) + 
                  "% to I2C register 0x" + std::to_string(static_cast<int>(pwm_reg_)) + 
                  " at address 0x" + std::to_string(static_cast<int>(i2c_address_)));
    return true;
}

/**
 * @brief Publishes the current fan status to MQTT
 * 
 * Creates a JSON object with the current fan status and publishes it
 * to the MQTT topic "fan/{name}/status".
 */
void Fan::publishStatus() {
    if (!mqtt_client_) return;

    json status_data = {
        {"name", name_},
        {"model", model_name_},
        {"status", status_},
        {"pwm_count", current_pwm_count_},
        {"duty_cycle", current_duty_cycle_},
        {"i2c_address", i2c_address_},
        {"pwm_reg", pwm_reg_},
        {"timestamp", std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())}
    };

    mqtt_client_->publish("fan/" + name_ + "/status", status_data.dump());
    logger_->debug("Published status update");
}

} // namespace fan_control_system 