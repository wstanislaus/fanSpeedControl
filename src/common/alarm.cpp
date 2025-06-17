#include "common/alarm.hpp"
#include <sstream>
#include <iomanip>
#include <chrono>

using json = nlohmann::json;

namespace common {

/**
 * @brief Constructs a new Alarm instance
 * 
 * Initializes the alarm with the given name and MQTT client. Sets up the MQTT topic
 * prefix for alarm messages and initializes the alarm state as inactive.
 * 
 * @param name Name of the alarm system (used in MQTT topics)
 * @param mqtt_client Shared pointer to MQTT client for publishing alarms
 */
Alarm::Alarm(const std::string& name, std::shared_ptr<MQTTClient> mqtt_client)
    : name_(name)
    , mqtt_client_(mqtt_client)
    , topic_prefix_("alarms/" + name)
    , active_(false)
    , current_severity_(AlarmSeverity::LOW)
{
}

/**
 * @brief Raises an alarm with the specified severity and message
 * 
 * Sets the alarm as active and publishes the alarm message to MQTT.
 * The message is published to the topic "alarms/{name}/raise".
 * 
 * @param severity The severity level of the alarm
 * @param message Description of the alarm condition
 */
void Alarm::raise(AlarmSeverity severity, const std::string& message) {
    active_ = true;
    current_severity_ = severity;
    mqtt_client_->publish(topic_prefix_ + "/raise", formatAlarmMessage(severity, message));
}

/**
 * @brief Clears the current alarm
 * 
 * If an alarm is active, clears it and publishes a clear message to MQTT.
 * The message is published to the topic "alarms/{name}/clear".
 * 
 * @param message Description of why the alarm is being cleared
 */
void Alarm::clear(const std::string& message) {
    if (!active_) return;
    
    active_ = false;
    mqtt_client_->publish(topic_prefix_ + "/clear", formatAlarmMessage(current_severity_, message, true));
    current_severity_ = AlarmSeverity::LOW;
}

/**
 * @brief Formats an alarm message for MQTT publishing
 * 
 * Creates a JSON object containing the alarm details including timestamp,
 * severity, source, message, and state (raised/cleared).
 * 
 * @param severity The severity level of the alarm
 * @param message The alarm message
 * @param is_clear Whether this is a clear message (true) or raise message (false)
 * @return Formatted JSON string containing the alarm message
 */
std::string Alarm::formatAlarmMessage(AlarmSeverity severity, const std::string& message, bool is_clear) {
    json alarm_entry = {
        {"timestamp", getTimestamp()},
        {"severity", static_cast<int>(severity)},
        {"source", name_},
        {"message", message},
        {"state", is_clear ? "cleared" : "raised"}
    };
    return alarm_entry.dump();
}

/**
 * @brief Gets the current timestamp in human-readable format
 * 
 * Formats the current system time as "YYYY-MM-DD HH:MM:SS"
 * 
 * @return Timestamp string in the format "YYYY-MM-DD HH:MM:SS"
 */
std::string Alarm::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

} // namespace common 