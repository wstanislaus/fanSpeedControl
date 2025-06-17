#include "fan_control_system/alarm_manager.hpp"
#include <iostream>
#include <stdexcept>
#include <nlohmann/json.hpp>

namespace fan_control_system {

/**
 * @brief Constructs a new AlarmManager instance
 * 
 * Initializes the alarm manager with configuration and MQTT settings.
 * Loads alarm configurations from the YAML config.
 * 
 * @param config YAML configuration node containing alarm settings
 * @param mqtt_settings MQTT client settings for publishing alarms
 * @throw std::runtime_error if initialization fails
 */
AlarmManager::AlarmManager(const YAML::Node& config, const common::MQTTClient::Settings& mqtt_settings)
    : config_(config), mqtt_settings_(mqtt_settings) {
    name_ = "AlarmManager";
    if (!load_alarm_configs()) {
        throw std::runtime_error("Failed to initialize alarm manager");
    }
}

/**
 * @brief Destructor for AlarmManager instance
 * 
 * Ensures the alarm manager is properly stopped and resources are cleaned up.
 */
AlarmManager::~AlarmManager() {
    stop();
}

/**
 * @brief Starts the alarm manager
 * 
 * Initializes MQTT client and logger, starts the main monitoring thread.
 * 
 * @return true if startup was successful, false otherwise
 */
bool AlarmManager::start() {
    if (running_) {
        return true;
    }

    if (!initialize()) {
        return false;
    }

    running_ = true;
    main_thread_ = std::thread(&AlarmManager::main_thread_function, this);
    return true;
}

/**
 * @brief Stops the alarm manager
 * 
 * Stops the main monitoring thread and cleans up resources.
 */
void AlarmManager::stop() {
    if (!running_) {
        return;
    }

    running_ = false;
    if (main_thread_.joinable()) {
        main_thread_.join();
    }
}

/**
 * @brief Raises an alarm with the specified name and message
 * 
 * Processes the alarm according to its configuration, executes associated actions,
 * and publishes the alarm to MQTT.
 * 
 * @param alarm_name Name of the alarm to raise
 * @param message Description of the alarm condition
 */
void AlarmManager::raise_alarm(const std::string& alarm_name, const std::string& message) {
    process_alarm(alarm_name, message);
}

/**
 * @brief Registers an action to be executed when an alarm is raised
 * 
 * @param action_name Name of the action to register
 * @param callback Function to be called when the action is triggered
 */
void AlarmManager::register_action(
    const std::string& action_name,
    std::function<void(const std::string&, const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    alarm_actions_[action_name] = AlarmAction{action_name, callback};
}

/**
 * @brief Gets the configuration for a specific alarm
 * 
 * @param alarm_name Name of the alarm
 * @return Reference to the alarm configuration
 * @throw std::runtime_error if alarm configuration not found
 */
const AlarmConfig& AlarmManager::get_alarm_config(const std::string& alarm_name) const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    auto it = alarm_configs_.find(alarm_name);
    if (it == alarm_configs_.end()) {
        throw std::runtime_error("Alarm configuration not found: " + alarm_name);
    }
    return it->second;
}

/**
 * @brief Initializes the alarm manager
 * 
 * Sets up MQTT client, logger, and subscribes to alarm topics.
 * 
 * @return true if initialization was successful, false otherwise
 */
bool AlarmManager::initialize() {
    mqtt_client_ = std::make_shared<common::MQTTClient>(name_, mqtt_settings_);
    if (!mqtt_client_->initialize() || !mqtt_client_->connect()) {
        std::cerr << "Failed to initialize MQTT client" << std::endl;
        return false;
    }

    // Initialize logger
    auto log_level = config_["AppLogLevel"]["FanControlSystem"][name_].as<std::string>();
    logger_ = std::make_unique<common::Logger>(name_, log_level, mqtt_client_);
    logger_->info("Alarm Manager initialized successfully");

    // Subscribe to alarm topics
    mqtt_client_->subscribe("alarms/#", 0);

    // Register message callback
    mqtt_client_->set_message_callback(&AlarmManager::mqtt_message_callback, this);

    return true;
}

/**
 * @brief Callback function for MQTT messages
 * 
 * @param mosq Mosquitto instance
 * @param obj User data (AlarmManager instance)
 * @param msg Received MQTT message
 */
void AlarmManager::mqtt_message_callback(
    struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg) {
    auto* manager = static_cast<AlarmManager*>(obj);
    if (!manager) return;
}

/**
 * @brief Loads alarm configurations from YAML
 * 
 * Parses the YAML configuration to load alarm settings including severity,
 * description, enabled state, and associated actions.
 * 
 * @return true if all configurations were loaded successfully, false otherwise
 */
bool AlarmManager::load_alarm_configs() {
    try {

        const auto& alarms = config_["Alarms"];
        for (const auto& alarm : alarms) {
            AlarmConfig config;
            config.name = alarm.first.as<std::string>();
            
            std::string severity_str = alarm.second["Severity"].as<std::string>();
            if (severity_str == "INFO") config.severity = AlarmSeverity::INFO;
            else if (severity_str == "WARNING") config.severity = AlarmSeverity::WARNING;
            else if (severity_str == "ERROR") config.severity = AlarmSeverity::ERROR;
            else if (severity_str == "CRITICAL") config.severity = AlarmSeverity::CRITICAL;
            
            config.description = alarm.second["Description"].as<std::string>();
            config.enabled = alarm.second["Enabled"].as<bool>();

            // Load actions
            for (const auto& action : alarm.second["Actions"]) {
                config.actions.push_back(action.as<std::string>());
            }

            alarm_configs_[config.name] = config;
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading alarm configurations: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief Processes an alarm when it is raised
 * 
 * Executes configured actions, publishes the alarm to MQTT, and logs the event
 * according to the alarm's severity level.
 * 
 * @param alarm_name Name of the alarm to process
 * @param message Description of the alarm condition
 */
void AlarmManager::process_alarm(const std::string& alarm_name, const std::string& message) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    auto it = alarm_configs_.find(alarm_name);
    if (it == alarm_configs_.end() || !it->second.enabled) {
        return;
    }

    const auto& config = it->second;

    // Execute actions
    for (const auto& action_name : config.actions) {
        auto action_it = alarm_actions_.find(action_name);
        if (action_it != alarm_actions_.end()) {
            action_it->second.callback(alarm_name, message);
        }
    }

    // Publish alarm to MQTT
    nlohmann::json alarm_json = {
        {"alarm", alarm_name},
        {"severity", static_cast<int>(config.severity)},
        {"message", message},
        {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
    };

    std::string topic = "alarms/" + alarm_name;
    mqtt_client_->publish(topic, alarm_json.dump());

    // Log the alarm
    std::string log_message = "Alarm raised: " + alarm_name + " - " + message;
    switch (config.severity) {
        case AlarmSeverity::INFO:
            logger_->info(log_message);
            break;
        case AlarmSeverity::WARNING:
            logger_->warning(log_message);
            break;
        case AlarmSeverity::ERROR:
            logger_->error(log_message);
            break;
        case AlarmSeverity::CRITICAL:
            logger_->error("CRITICAL: " + log_message);
            break;
    }
}

/**
 * @brief Main thread function for the alarm manager
 * 
 * Runs in a loop while the alarm manager is active, handling any background tasks.
 */
void AlarmManager::main_thread_function() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

} // namespace fan_control_system 