#pragma once

#include "common/mqtt_client.hpp"
#include <string>
#include <memory>
#include <nlohmann/json.hpp>

namespace common {

/**
 * @enum AlarmSeverity
 * @brief Defines the severity levels for alarms
 */
enum class AlarmSeverity {
    LOW,        ///< Low severity alarm - minor issues that don't require immediate attention
    MEDIUM,     ///< Medium severity alarm - issues that should be addressed soon
    HIGH,       ///< High severity alarm - serious issues requiring prompt attention
    CRITICAL    ///< Critical severity alarm - system-threatening issues requiring immediate action
};

/**
 * @class Alarm
 * @brief Manages alarm states and notifications via MQTT
 * 
 * This class handles the raising and clearing of alarms, with different severity levels.
 * Alarms are published to MQTT topics for monitoring and notification purposes.
 */
class Alarm {
public:
    /**
     * @brief Constructs a new Alarm instance
     * @param name Name of the alarm system (used in MQTT topics)
     * @param mqtt_client Shared pointer to MQTT client for publishing alarms
     */
    Alarm(const std::string& name, std::shared_ptr<MQTTClient> mqtt_client);
    
    /**
     * @brief Raises an alarm with the specified severity and message
     * @param severity The severity level of the alarm
     * @param message Description of the alarm condition
     */
    void raise(AlarmSeverity severity, const std::string& message);

    /**
     * @brief Clears the current alarm
     * @param message Description of why the alarm is being cleared
     */
    void clear(const std::string& message);

    /**
     * @brief Checks if there is an active alarm
     * @return true if an alarm is active, false otherwise
     */
    bool isActive() const { return active_; }

    /**
     * @brief Gets the severity of the current alarm
     * @return The current alarm severity level
     */
    AlarmSeverity getCurrentSeverity() const { return current_severity_; }

private:
    /**
     * @brief Formats an alarm message for MQTT publishing
     * @param severity The severity level of the alarm
     * @param message The alarm message
     * @param is_clear Whether this is a clear message (true) or raise message (false)
     * @return Formatted JSON string containing the alarm message
     */
    std::string formatAlarmMessage(AlarmSeverity severity, const std::string& message, bool is_clear = false);

    /**
     * @brief Gets the current timestamp in ISO 8601 format
     * @return Timestamp string
     */
    std::string getTimestamp();
    
    std::string name_;                                          ///< Name of the alarm system
    std::shared_ptr<MQTTClient> mqtt_client_;                   ///< MQTT client for publishing alarms
    std::string topic_prefix_;                                  ///< MQTT topic prefix for alarm messages
    bool active_;                                               ///< Whether an alarm is currently active
    AlarmSeverity current_severity_;                           ///< Severity of the current alarm
};

} // namespace common 