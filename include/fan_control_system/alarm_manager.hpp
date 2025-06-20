#pragma once

#include <memory>
#include <thread>
#include <atomic>
#include <string>
#include <map>
#include <mutex>
#include <vector>
#include <functional>
#include <deque>
#include <chrono>
#include <yaml-cpp/yaml.h>
#include "common/mqtt_client.hpp"
#include "common/logger.hpp"
#include "common/config.hpp"

namespace fan_control_system {

/**
 * @enum AlarmSeverity
 * @brief Defines the severity levels for system alarms
 */
enum class AlarmSeverity {
    INFO,       ///< Informational messages that don't require action
    WARNING,    ///< Warning conditions that may need attention
    ERROR,      ///< Error conditions that require attention
    CRITICAL    ///< Critical conditions that require immediate action
};

/**
 * @struct AlarmAction
 * @brief Defines an action to be taken when an alarm is triggered
 */
struct AlarmAction {
    std::string name;    ///< Name of the alarm action
    std::function<void(const std::string&, const std::string&)> callback;  ///< Callback function to execute
};

/**
 * @struct AlarmConfig
 * @brief Configuration structure for alarm system settings
 */
struct AlarmConfig {
    int alarm_history_size;                                  ///< Maximum number of alarms to keep in history
    std::map<std::string, std::vector<std::string>> severity_actions;  ///< Actions for each severity level
};

/**
 * @struct AlarmEntry
 * @brief Runtime alarm entry stored in the database
 */
struct AlarmEntry {
    std::string name;                    ///< Name of the alarm
    std::string message;                 ///< Alarm message
    AlarmSeverity severity;              ///< Severity level
    std::string timestamp;               ///< Timestamp when alarm was raised
    bool is_active;                      ///< Whether alarm is currently active
    bool acknowledged;                   ///< Whether alarm has been acknowledged
    std::vector<std::string> actions_taken; ///< Actions that were executed
};

/**
 * @struct AlarmStatistics
 * @brief Statistics for alarm analysis
 */
struct AlarmStatistics {
    std::string alarm_name;
    int32_t total_count;
    int32_t active_count;
    int32_t acknowledged_count;
    std::map<std::string, int32_t> severity_counts;
    std::string last_occurrence;
    std::string first_occurrence;
};

/**
 * @class AlarmManager
 * @brief Manages system-wide alarms with MQTT subscription, runtime database, and configurable actions
 * 
 * This class handles the raising and processing of alarms throughout the system.
 * It subscribes to MQTT alarm events from other modules, stores alarms in a runtime database,
 * executes configurable actions based on severity, and provides CLI debugging capabilities.
 */
class AlarmManager {
public:
    /**
     * @brief Constructs a new Alarm Manager instance
     * @param config YAML configuration node containing alarm configurations
     * @param mqtt_settings MQTT communication settings
     */
    explicit AlarmManager(const YAML::Node& config, const common::MQTTClient::Settings& mqtt_settings);

    /**
     * @brief Destructor that ensures proper cleanup
     */
    ~AlarmManager();

    /**
     * @brief Starts the alarm manager
     * @return true if the manager started successfully, false otherwise
     */
    bool start();

    /**
     * @brief Stops the alarm manager
     * @note This will stop all alarm processing and MQTT publishing
     */
    void stop();

    /**
     * @brief Raises an alarm with the specified name and message
     * @param alarm_name Name of the alarm to raise
     * @param severity Severity of the alarm
     * @param message Description of the alarm condition
     */
    void raise_alarm(const std::string& alarm_name, AlarmSeverity severity, const std::string& message);

    /**
     * @brief Registers a new alarm action
     * @param action_name Name of the action
     * @param callback Function to be called when the action is triggered
     */
    void register_action(const std::string& action_name, 
                        std::function<void(const std::string&, const std::string&)> callback);

    /**
     * @brief Gets the configuration for the alarm system
     * @return Reference to the alarm configuration
     */
    const AlarmConfig& get_alarm_config() const;

    // CLI Debugging and Testing APIs
    /**
     * @brief Gets alarm history for CLI
     * @param alarm_name Optional alarm name filter
     * @param max_entries Maximum number of entries to return
     * @return Vector of alarm history entries
     */
    std::vector<AlarmEntry> get_alarm_history(const std::string& alarm_name = "", int max_entries = -1) const;

    /**
     * @brief Gets severity actions for CLI
     * @return Map of severity to actions
     */
    std::map<std::string, std::vector<std::string>> get_severity_actions() const;

    /**
     * @brief Clears alarm history for CLI
     * @param alarm_name Optional alarm name filter
     * @return Number of entries cleared
     */
    int clear_alarm_history(const std::string& alarm_name = "");

    /**
     * @brief Gets alarm statistics for CLI
     * @param alarm_name Optional alarm name filter
     * @param time_window_hours Time window for statistics
     * @return Vector of alarm statistics
     */
    std::vector<AlarmStatistics> get_alarm_statistics(const std::string& alarm_name = "", int time_window_hours = 24) const;

private:
    /**
     * @brief Initializes MQTT connection and components
     * @return true if initialization was successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Loads alarm configurations from the YAML node
     * @return true if configurations were loaded successfully, false otherwise
     */
    bool load_alarm_configs();

    /**
     * @brief Processes an alarm by executing its configured actions
     * @param alarm_source Source of the alarm
     * @param severity Severity of the alarm
     * @param message Description of the alarm condition
     */
    void process_alarm(const std::string& alarm_source, AlarmSeverity severity, const std::string& message);

    /**
     * @brief Executes actions for a specific severity level
     * @param severity Severity level
     * @param alarm_source Source of the alarm
     * @param message Alarm message
     */
    void execute_severity_actions(AlarmSeverity severity, const std::string& alarm_source, const std::string& message);

    /**
     * @brief Adds an alarm entry to the runtime database
     * @param entry Alarm entry to add
     */
    void add_alarm_entry(const AlarmEntry& entry);

    /**
     * @brief Main thread function for alarm processing
     */
    void main_thread_function();

    /**
     * @brief MQTT message callback for receiving alarm-related messages
     * @param mosq Pointer to the mosquitto instance
     * @param obj User data pointer
     * @param msg Pointer to the received message
     */
    static void mqtt_message_callback(struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg);

    /**
     * @brief Processes incoming MQTT alarm messages
     * @param topic MQTT topic
     * @param payload Message payload
     */
    void process_mqtt_alarm_message(const std::string& topic, const std::string& payload);

    /**
     * @brief Converts severity enum to string
     * @param severity Severity enum
     * @return String representation
     */
    static std::string severity_to_string(AlarmSeverity severity);

    /**
     * @brief Converts string to severity enum
     * @param severity_str String representation
     * @return Severity enum
     */
    static AlarmSeverity string_to_severity(const std::string& severity_str);

    // Configuration
    YAML::Node config_;                                    ///< Loaded configuration data

    // MQTT settings and components
    common::MQTTClient::Settings mqtt_settings_;           ///< MQTT communication settings
    std::shared_ptr<common::MQTTClient> mqtt_client_;      ///< MQTT client for communication
    
    // Alarm configurations and actions
    AlarmConfig alarm_config_;                              ///< Alarm configuration
    mutable std::mutex config_mutex_;                      ///< Mutex for thread-safe config access
    std::map<std::string, std::function<void(const std::string&, const std::string&)>> action_callbacks_; ///< Registered action callbacks

    // Runtime alarm database
    std::deque<AlarmEntry> alarm_history_;                ///< Runtime alarm history database
    mutable std::mutex history_mutex_;                     ///< Mutex for thread-safe history access
    int max_history_entries_;                              ///< Maximum number of history entries

    // Common components
    std::unique_ptr<common::Logger> logger_;               ///< Logger for alarm manager
    
    // Thread control
    std::thread main_thread_;                             ///< Main alarm processing thread
    std::atomic<bool> running_{false};                    ///< Flag indicating if manager is running

    // Name
    std::string name_;                                    ///< Name of the alarm manager
};

} // namespace fan_control_system 