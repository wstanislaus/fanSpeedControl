#pragma once

#include <memory>
#include <thread>
#include <atomic>
#include <string>
#include <map>
#include <mutex>
#include <vector>
#include <functional>
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
 * @brief Configuration structure for an alarm type
 */
struct AlarmConfig {
    std::string name;                    ///< Name of the alarm
    AlarmSeverity severity;              ///< Severity level of the alarm
    std::string description;             ///< Description of the alarm condition
    std::vector<std::string> actions;    ///< List of action names to execute
    bool enabled;                        ///< Whether this alarm is enabled
};

/**
 * @class AlarmManager
 * @brief Manages system-wide alarms with configurable actions and severity levels
 * 
 * This class handles the raising and processing of alarms throughout the system.
 * It supports different severity levels and can execute custom actions when alarms
 * are triggered. Alarms are published via MQTT for monitoring and notification.
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
     * @param message Description of the alarm condition
     */
    void raise_alarm(const std::string& alarm_name, const std::string& message);

    /**
     * @brief Registers a new alarm action
     * @param action_name Name of the action
     * @param callback Function to be called when the action is triggered
     */
    void register_action(const std::string& action_name, 
                        std::function<void(const std::string&, const std::string&)> callback);

    /**
     * @brief Gets the configuration for a specific alarm
     * @param alarm_name Name of the alarm
     * @return Reference to the alarm configuration
     */
    const AlarmConfig& get_alarm_config(const std::string& alarm_name) const;

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
     * @param alarm_name Name of the alarm to process
     * @param message Description of the alarm condition
     */
    void process_alarm(const std::string& alarm_name, const std::string& message);

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

    // Configuration
    YAML::Node config_;                                    ///< Loaded configuration data

    // MQTT settings and components
    common::MQTTClient::Settings mqtt_settings_;           ///< MQTT communication settings
    std::shared_ptr<common::MQTTClient> mqtt_client_;      ///< MQTT client for communication
    
    // Alarm configurations and actions
    std::map<std::string, AlarmConfig> alarm_configs_;    ///< Map of alarm configurations
    std::map<std::string, AlarmAction> alarm_actions_;    ///< Map of registered alarm actions
    mutable std::mutex config_mutex_;                      ///< Mutex for thread-safe config access

    // Common components
    std::unique_ptr<common::Logger> logger_;               ///< Logger for alarm manager
    
    // Thread control
    std::thread main_thread_;                             ///< Main alarm processing thread
    std::atomic<bool> running_{false};                    ///< Flag indicating if manager is running

    // Name
    std::string name_;                                    ///< Name of the alarm manager
};

} // namespace fan_control_system 