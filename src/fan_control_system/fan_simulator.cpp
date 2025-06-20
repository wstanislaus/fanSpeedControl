#include "fan_control_system/fan_simulator.hpp"
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace fan_control_system {

/**
 * @brief Constructs a new FanSimulator instance
 * 
 * Initializes the fan simulator with configuration and MQTT settings.
 * Sets up MQTT client, logger, and alarm system. Loads fan models and creates
 * fan instances based on the configuration.
 * 
 * @param config YAML configuration node containing fan models and settings
 * @param mqtt_settings MQTT client settings for publishing fan status
 * @throw std::runtime_error if initialization fails
 */
FanSimulator::FanSimulator(const YAML::Node& config, const common::MQTTClient::Settings& mqtt_settings)
    : config_(config), mqtt_settings_(mqtt_settings), running_(false), is_it_loud_(false) {
    name_ = "FanSimulator";
    // Initialize MQTT client and logger first
    mqtt_client_ = std::make_shared<common::MQTTClient>(name_, mqtt_settings_);
    if (!mqtt_client_->initialize() || !mqtt_client_->connect()) {
        throw std::runtime_error("Failed to initialize MQTT client");
    }
    log_level_ = config_["AppLogLevel"]["FanControlSystem"][name_].as<std::string>();
    logger_ = std::make_unique<common::Logger>(name_, log_level_, mqtt_client_);
    logger_->info("Fan Simulator initializing...");

    // Initialize alarm
    alarm_ = std::make_unique<common::Alarm>(name_, mqtt_client_);

    if (!load_fan_models() || !create_fans()) {
        throw std::runtime_error("Failed to initialize fan simulator");
    }
}

/**
 * @brief Destructor for FanSimulator instance
 * 
 * Ensures the simulator is properly stopped and resources are cleaned up.
 */
FanSimulator::~FanSimulator() {
    stop();
}

/**
 * @brief Starts the fan simulator
 * 
 * Initializes all fans and starts the main monitoring thread.
 * 
 * @return true if startup was successful, false otherwise
 */
bool FanSimulator::start() {
    if (running_) {
        logger_->info("Fan Simulator already running");
        return true;
    }

    if (!initialize()) {
        logger_->error("Failed to initialize Fan Simulator");
        return false;
    }

    running_ = true;
    main_thread_ = std::thread(&FanSimulator::main_thread_function, this);
    logger_->info("Fan Simulator started successfully");
    return true;
}

/**
 * @brief Stops the fan simulator
 * 
 * Stops the main monitoring thread and all fan instances.
 */
void FanSimulator::stop() {
    if (!running_) {
        return;
    }

    logger_->info("Stopping Fan Simulator...");
    running_ = false;
    if (main_thread_.joinable()) {
        main_thread_.join();
    }

    // Stop all fans
    for (auto& fan : fans_) {
        fan.second->stop();
    }
    logger_->info("Fan Simulator stopped");
}

/**
 * @brief Gets the noise level for a fan model at a given duty cycle
 * 
 * @param model Name of the fan model
 * @param duty_cycle Current duty cycle percentage
 * @return Noise level in decibels, or 0 if model not found
 */
int FanSimulator::get_noise_level(const std::string& model, int duty_cycle) const {
    auto it = fan_models_.find(model);
    if (it == fan_models_.end()) {
        return 0;
    }
    const auto& fan_model = it->second;
    for (const auto& profile : fan_model.noise_profile) {
        if (duty_cycle >= profile.first) {
            return profile.second;
        }
    }
    return 0;
}

/**
 * @brief Sets the speed for all fans
 * 
 * Converts the duty cycle to PWM count and sets it for each fan.
 * 
 * @param duty_cycle Target duty cycle percentage
 * @return true if all fans were set successfully, false otherwise
 */
bool FanSimulator::set_fan_speed(int duty_cycle) {
    for (auto& fan : fans_) {
        logger_->debug("Setting fan speed to " + std::to_string(duty_cycle) + "%");
        //convert duty cycle to pwm
        int pwm_count = duty_cycle_to_pwm(fan.second->getModelName(), duty_cycle);
        bool result = fan.second->setPwmCount(duty_cycle, pwm_count);
        if (!result) {
            logger_->error("Failed to set fan speed for " + fan.first);
        }
    }
    return true;
}

bool FanSimulator::set_fan_speed(const std::string& fan_name, int duty_cycle) {
    auto it = fans_.find(fan_name);
    if (it == fans_.end()) {
        logger_->warning("Attempted to set speed for non-existent fan: " + fan_name);
        return false;
    }
    int pwm_count = duty_cycle_to_pwm(it->second->getModelName(), duty_cycle);
    return it->second->setPwmCount(duty_cycle, pwm_count);
}

/**
 * @brief Gets the current speed of a specific fan
 * 
 * @param controller_name Name of the fan controller
 * @return Current duty cycle percentage, or -1 if fan not found
 */
int FanSimulator::get_fan_speed(const std::string& controller_name) const {
    auto it = fans_.find(controller_name);
    if (it == fans_.end()) {
        logger_->warning("Attempted to get speed for non-existent fan: " + controller_name);
        return -1;
    }

    int speed = it->second->getDutyCycle();
    logger_->debug("Current fan speed for " + controller_name + ": " + std::to_string(speed) + "%");
    return speed;
}

/**
 * @brief Gets a fan instance by name
 * 
 * @param name Name of the fan
 * @return Shared pointer to the fan instance, or nullptr if not found
 */
std::shared_ptr<Fan> FanSimulator::get_fan(const std::string& name) {
    auto it = fans_.find(name);
    if (it == fans_.end()) {
        logger_->warning("Attempted to get non-existent fan: " + name);
        return nullptr;
    }
    return it->second;
}

/**
 * @brief Marks a fan as bad
 * 
 * @param name Name of the fan to mark as bad
 * @return true if the operation was successful, false otherwise
 */
bool FanSimulator::make_fan_bad(const std::string& name) {
    auto it = fans_.find(name);
    if (it == fans_.end()) {
        logger_->warning("Attempted to make non-existent fan bad: " + name);
        return false;
    }

    logger_->info("Making fan bad: " + name);
    bool result = it->second->makeBad();
    if (!result) {
        logger_->error("Failed to make fan bad: " + name);
    }
    return result;
}

/**
 * @brief Marks a fan as good
 * 
 * @param name Name of the fan to mark as good
 * @return true if the operation was successful, false otherwise
 */
bool FanSimulator::make_fan_good(const std::string& name) {
    auto it = fans_.find(name);
    if (it == fans_.end()) {
        logger_->warning("Attempted to make non-existent fan good: " + name);
        return false;
    }

    logger_->info("Making fan good: " + name);
    bool result = it->second->makeGood();
    if (!result) {
        logger_->error("Failed to make fan good: " + name);
    }
    return result;
}

/**
 * @brief Initializes all fans
 * 
 * Initializes and starts each fan instance.
 * 
 * @return true if all fans were initialized successfully, false otherwise
 */
bool FanSimulator::initialize() {
    // Start all fans
    for (auto& fan : fans_) {
        if (!fan.second->initialize()) {
            logger_->error("Failed to initialize fan: " + fan.first);
            return false;
        }
        fan.second->start();
        logger_->info("Fan initialized and started: " + fan.first);
    }

    logger_->info("Fan Simulator initialized successfully with " + std::to_string(fans_.size()) + " fans");
    return true;
}

/**
 * @brief Loads fan models from configuration
 * 
 * Parses the YAML configuration to load fan models with their properties
 * including PWM ranges, duty cycle ranges, and noise profiles.
 * 
 * @return true if all models were loaded successfully, false otherwise
 */
bool FanSimulator::load_fan_models() {
    try {
        const auto& models = config_["FanModels"];
        for (const auto& model : models) {
            FanModel fan_model;
            fan_model.name = model.first.as<std::string>();
            fan_model.number_of_fans = model.second["NumberOfFans"].as<int>();
            fan_model.pwm_min = model.second["PWMRange"]["Min"].as<int>();
            fan_model.pwm_max = model.second["PWMRange"]["Max"].as<int>();
            fan_model.duty_cycle_min = model.second["DutyCycleRange"]["Min"].as<int>();
            fan_model.duty_cycle_max = model.second["DutyCycleRange"]["Max"].as<int>();
            fan_model.interface = model.second["Interface"].as<std::string>();
            fan_model.pwm_reg = model.second["PWM_REG"].as<uint8_t>();

            // Load noise profile
            for (const auto& profile : model.second["NoiseProfile"]) {
                int duty_cycle = profile["DutyCycle"].as<int>();
                int noise_level = profile["NoiseLevel_dB"].as<int>();
                fan_model.noise_profile[duty_cycle] = noise_level;
            }

            fan_models_[fan_model.name] = fan_model;
            logger_->debug("Loaded fan model: " + fan_model.name);
        }
        logger_->info("Successfully loaded " + std::to_string(fan_models_.size()) + " fan models");
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading fan models: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief Creates fan instances based on configuration
 * 
 * Creates fan instances for each controller defined in the configuration.
 * 
 * @return true if all fans were created successfully, false otherwise
 */
bool FanSimulator::create_fans() {
    try {
        const auto& controllers = config_["FanControllers"];
        const auto max_fan_controllers = config_["MaxFanControllers"].as<int>();
        fans_too_loud_threshold_ = config_["FansTooLoudAlarm"].as<int>();
        if (controllers.size() > max_fan_controllers) {
            logger_->error("Max fan controllers exceeded: " + std::to_string(controllers.size()) + " > " + std::to_string(max_fan_controllers));
        }
        int fan_count = 0;
        for (const auto& controller : controllers) {
            const auto& name = controller.first.as<std::string>();
            const auto& model_name = controller.second["Model"].as<std::string>();
            const auto& i2c_address = controller.second["I2CAddress"].as<uint8_t>();

            auto model_it = fan_models_.find(model_name);
            if (model_it == fan_models_.end()) {
                logger_->error("Fan model not found: " + model_name);
                return false;
            }

            auto fan = std::make_shared<Fan>(name, model_name, i2c_address, 
                                           model_it->second.pwm_reg, mqtt_settings_, log_level_
                                           , model_it->second.pwm_min, model_it->second.pwm_max
                                           , model_it->second.duty_cycle_min, model_it->second.duty_cycle_max
                                           , model_it->second.noise_profile);
            fans_[name] = fan;
            logger_->debug("Created fan instance: " + name + " (Model: " + model_name + ")");
            if (fan_count >= max_fan_controllers) {
                break;
            }
            fan_count++;
        }
        logger_->info("Successfully created " + std::to_string(fan_count) + " fan instances");
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error creating fans: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief Converts duty cycle to PWM count for a fan model
 * 
 * @param model Name of the fan model
 * @param duty_cycle Duty cycle percentage
 * @return Corresponding PWM count value
 */
int FanSimulator::duty_cycle_to_pwm(const std::string& model, int duty_cycle) const {
    auto it = fan_models_.find(model);
    if (it == fan_models_.end()) {
        return -1;
    }

    const auto& fan_model = it->second;
    duty_cycle = std::max(fan_model.duty_cycle_min, std::min(duty_cycle, fan_model.duty_cycle_max));
    
    // Linear interpolation
    double ratio = static_cast<double>(duty_cycle - fan_model.duty_cycle_min) /
                  (fan_model.duty_cycle_max - fan_model.duty_cycle_min);
    return fan_model.pwm_min + static_cast<int>(ratio * (fan_model.pwm_max - fan_model.pwm_min));
}

/**
 * @brief Converts PWM count to duty cycle for a fan model
 * 
 * @param model Name of the fan model
 * @param pwm PWM count value
 * @return Corresponding duty cycle percentage
 */
int FanSimulator::pwm_to_duty_cycle(const std::string& model, int pwm) const {
    auto it = fan_models_.find(model);
    if (it == fan_models_.end()) {
        return -1;
    }

    const auto& fan_model = it->second;
    pwm = std::max(fan_model.pwm_min, std::min(pwm, fan_model.pwm_max));
    
    // Linear interpolation
    double ratio = static_cast<double>(pwm - fan_model.pwm_min) /
                  (fan_model.pwm_max - fan_model.pwm_min);
    return fan_model.duty_cycle_min + static_cast<int>(ratio * (fan_model.duty_cycle_max - fan_model.duty_cycle_min));
}

void FanSimulator::check_noise_condition() {
    // Check for noise conditions
    bool noise_condition = false;
    for (auto& fan : fans_) {
        int noise_level = get_noise_level(fan.second->getModelName(), fan.second->getDutyCycle());
        if (noise_level > static_cast<int>(NoiseLevel::MODERATE)) {
            noise_condition = true;
        }
    }
    if (noise_condition && !is_it_loud_) {
        last_loud_noise_start_time_ = std::chrono::system_clock::now();
        is_it_loud_ = true;
    } else if (!noise_condition && is_it_loud_) {
        is_it_loud_ = false;
    }
    if (is_it_loud_ && std::chrono::system_clock::now() - last_loud_noise_start_time_ > std::chrono::minutes(fans_too_loud_threshold_)) {
        logger_->warning("Fans are too loud for " + std::to_string(fans_too_loud_threshold_) + " minutes");
        alarm_->raise(common::AlarmSeverity::HIGH, "Fans are too loud");
        is_it_loud_ = false; //reset the flag to avoid raising the alarm again for the same noise condition
    }
}

void FanSimulator::main_thread_function() {
    logger_->info("Fan Simulator main thread started");
    while (running_) {
        check_noise_condition();
        // Process fan speed updates and send MQTT messages
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    logger_->info("Fan Simulator main thread stopped");
}

bool FanSimulator::set_fan_pwm(const std::string& fan_name, int pwm_count) {
    auto it = fans_.find(fan_name);
    if (it == fans_.end()) {
        logger_->warning("Attempted to set PWM for non-existent fan: " + fan_name);
        return false;
    }
    int duty_cycle = pwm_to_duty_cycle(it->second->getModelName(), pwm_count);
    return it->second->setPwmCount(duty_cycle, pwm_count);
}

int FanSimulator::get_fan_noise_level(const std::string& fan_name) const {
    auto it = fans_.find(fan_name);
    if (it == fans_.end()) {
        logger_->warning("Attempted to get noise level for non-existent fan: " + fan_name);
        return -1;
    }
    return it->second->getNoiseLevel();
}

std::string FanSimulator::get_fan_noise_category(const std::string& fan_name) const {
    auto it = fans_.find(fan_name);
    if (it == fans_.end()) {
        logger_->warning("Attempted to get noise category for non-existent fan: " + fan_name);
        return "UNKNOWN";
    }
    auto noise_level = it->second->getNoiseLevel();
    if (noise_level <= static_cast<int>(NoiseLevel::QUIET)) {
        return "QUIET";
    } else if (noise_level <= static_cast<int>(NoiseLevel::MODERATE)) {
        return "MODERATE";
    } else if (noise_level <= static_cast<int>(NoiseLevel::LOUD)) {
        return "LOUD";
    } else if (noise_level <= static_cast<int>(NoiseLevel::VERY_LOUD)) {
        return "VERY_LOUD";
    } else if (noise_level <= static_cast<int>(NoiseLevel::EXTREMELY_LOUD)) {
        return "EXTREMELY_LOUD";
    } else if (noise_level <= static_cast<int>(NoiseLevel::PAINFULLY_LOUD)) {
        return "PAINFULLY_LOUD";
    } else {
        return "DANGEROUS";
    }
}

} // namespace fan_control_system 