#include "fan_control_system/alarm_manager.hpp"
#include "common/utils.hpp"
#include <iostream>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include <iomanip>
#include <sstream>

namespace fan_control_system {

/**
 * @brief Constructs a new AlarmManager instance
 * 
 * Initializes the alarm manager with configuration and MQTT settings.
 * Loads alarm configurations and severity actions from the YAML config.
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
 * @param alarm_source Source of the alarm
 * @param severity Severity of the alarm
 * @param message Description of the alarm condition
 */
void AlarmManager::raise_alarm(const std::string& alarm_source, AlarmSeverity severity, const std::string& message) {
    process_alarm(alarm_source, severity, message);
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
    action_callbacks_[action_name] = callback;
}

/**
 * @brief Gets the configuration for the alarm system
 * 
 * @return Reference to the alarm configuration
 */
const AlarmConfig& AlarmManager::get_alarm_config() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return alarm_config_;
}

// CLI Debugging and Testing APIs Implementation

std::vector<AlarmEntry> AlarmManager::get_alarm_history(const std::string& alarm_name, int max_entries) const {
    std::lock_guard<std::mutex> lock(history_mutex_);
    std::vector<AlarmEntry> result;
    
    for (const auto& entry : alarm_history_) {
        if (alarm_name.empty() || entry.name == alarm_name) {
            result.push_back(entry);
            if (max_entries > 0 && result.size() >= static_cast<size_t>(max_entries)) {
                break;
            }
        }
    }
    
    return result;
}

int AlarmManager::clear_alarm_history(const std::string& alarm_name) {
    std::lock_guard<std::mutex> lock(history_mutex_);
    int cleared_count = 0;
    
    if (alarm_name.empty()) {
        cleared_count = alarm_history_.size();
        alarm_history_.clear();
    } else {
        auto it = alarm_history_.begin();
        while (it != alarm_history_.end()) {
            if (it->name == alarm_name) {
                it = alarm_history_.erase(it);
                cleared_count++;
            } else {
                ++it;
            }
        }
    }
    
    return cleared_count;
}

std::vector<AlarmStatistics> AlarmManager::get_alarm_statistics(const std::string& alarm_name, int time_window_hours) const {
    std::lock_guard<std::mutex> lock(history_mutex_);
    std::map<std::string, AlarmStatistics> stats_map;
    
    auto now = std::chrono::system_clock::now();
    auto window_duration = std::chrono::hours(time_window_hours);
    
    for (const auto& entry : alarm_history_) {
        // Parse timestamp and check if within time window
        std::tm tm = {};
        std::istringstream ss(entry.timestamp);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        auto entry_time = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        
        if (now - entry_time > window_duration) {
            continue; // Skip entries outside time window
        }
        
        if (!alarm_name.empty() && entry.name != alarm_name) {
            continue; // Skip if filtering by specific alarm
        }
        
        auto& stats = stats_map[entry.name];
        stats.alarm_name = entry.name;
        stats.total_count++;
        
        if (entry.is_active) {
            stats.active_count++;
        }
        
        if (entry.acknowledged) {
            stats.acknowledged_count++;
        }
        
        std::string severity_str = severity_to_string(entry.severity);
        stats.severity_counts[severity_str]++;
        
        if (stats.last_occurrence.empty() || entry.timestamp > stats.last_occurrence) {
            stats.last_occurrence = entry.timestamp;
        }
        
        if (stats.first_occurrence.empty() || entry.timestamp < stats.first_occurrence) {
            stats.first_occurrence = entry.timestamp;
        }
    }
    
    std::vector<AlarmStatistics> result;
    for (const auto& pair : stats_map) {
        result.push_back(pair.second);
    }
    
    return result;
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

    // Subscribe to alarm topics from other modules
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
    
    std::string topic(reinterpret_cast<const char*>(msg->topic));
    std::string payload(reinterpret_cast<const char*>(msg->payload), msg->payloadlen);
    
    manager->process_mqtt_alarm_message(topic, payload);
}

/**
 * @brief Processes incoming MQTT alarm messages
 * 
 * @param topic MQTT topic
 * @param payload Message payload
 */
void AlarmManager::process_mqtt_alarm_message(const std::string& topic, const std::string& payload) {
    try {
        nlohmann::json alarm_json = nlohmann::json::parse(payload);
        
        // Extract fields from the actual alarm message format
        std::string source = alarm_json.value("source", "");
        std::string message = alarm_json.value("message", "");
        std::string state = alarm_json.value("state", "");
        int severity_int = alarm_json.value("severity", 0);
        std::string timestamp = alarm_json.value("timestamp", "");
        
        // Only process raised alarms (not cleared ones)
        if (state == "raised" && !source.empty() && !message.empty()) {
            AlarmSeverity severity = static_cast<AlarmSeverity>(severity_int);
            process_alarm(source, severity, message);
        }
    } catch (const std::exception& e) {
        logger_->error("Failed to process MQTT alarm message: " + std::string(e.what()));
    }
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
        
        // Load alarm history size
        alarm_config_.alarm_history_size = alarms["AlarmHistory"].as<int>();
        max_history_entries_ = alarm_config_.alarm_history_size;
        
        // Load severity actions
        const auto& severity_actions = alarms["SeverityActions"];
        for (const auto& severity : severity_actions) {
            std::string severity_name = severity.first.as<std::string>();
            std::vector<std::string> actions;
            
            for (const auto& action : severity.second) {
                actions.push_back(action.as<std::string>());
            }
            
            alarm_config_.severity_actions[severity_name] = actions;
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
void AlarmManager::process_alarm(const std::string& alarm_source, AlarmSeverity severity, const std::string& message) {
    std::lock_guard<std::mutex> lock(config_mutex_);

    // Execute severity-based actions
    execute_severity_actions(severity, alarm_source, message);

    // Create alarm entry for runtime database
    AlarmEntry entry;
    entry.name = alarm_source;
    entry.message = message;
    entry.severity = severity;
    entry.is_active = true;
    entry.acknowledged = false;
    
    // Generate timestamp
    entry.timestamp = common::utils::getCurrentTimestamp();

    // Add to runtime database
    add_alarm_entry(entry);

    // Log the alarm
    std::string log_message = "Alarm Processed: " + alarm_source + " - " + message;
    switch (severity) {
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
 * @brief Executes actions for a specific severity level
 * 
 * @param severity Severity level
 * @param alarm_source Source of the alarm
 * @param message Alarm message
 */
void AlarmManager::execute_severity_actions(AlarmSeverity severity, const std::string& alarm_source, const std::string& message) {
    std::string severity_str = severity_to_string(severity);
    auto it = alarm_config_.severity_actions.find(severity_str);
    
    if (it != alarm_config_.severity_actions.end()) {
        for (const auto& action_name : it->second) {
            auto action_it = action_callbacks_.find(action_name);
            if (action_it != action_callbacks_.end()) {
                action_it->second(alarm_source, message);
                logger_->info("Executed action: " + action_name + " for alarm: " + alarm_source);
            }
        }
    }
}

/**
 * @brief Adds an alarm entry to the runtime database
 * 
 * @param entry Alarm entry to add
 */
void AlarmManager::add_alarm_entry(const AlarmEntry& entry) {
    std::lock_guard<std::mutex> lock(history_mutex_);
    
    alarm_history_.push_back(entry);
    
    // Maintain maximum history size
    while (alarm_history_.size() > static_cast<size_t>(max_history_entries_)) {
        alarm_history_.pop_front();
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

/**
 * @brief Converts severity enum to string
 * 
 * @param severity Severity enum
 * @return String representation
 */
std::string AlarmManager::severity_to_string(AlarmSeverity severity) {
    switch (severity) {
        case AlarmSeverity::INFO: return "INFO";
        case AlarmSeverity::WARNING: return "WARNING";
        case AlarmSeverity::ERROR: return "ERROR";
        case AlarmSeverity::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Converts string to severity enum
 * 
 * @param severity_str String representation
 * @return Severity enum
 */
AlarmSeverity AlarmManager::string_to_severity(const std::string& severity_str) {
    if (severity_str == "INFO") return AlarmSeverity::INFO;
    if (severity_str == "WARNING") return AlarmSeverity::WARNING;
    if (severity_str == "ERROR") return AlarmSeverity::ERROR;
    if (severity_str == "CRITICAL") return AlarmSeverity::CRITICAL;
    return AlarmSeverity::INFO; // Default to INFO
}

std::map<std::string, std::vector<std::string>> AlarmManager::get_severity_actions() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return alarm_config_.severity_actions;
}

} // namespace fan_control_system 