#include "common/logger.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace common {

/**
 * @brief Constructs a new Logger instance
 * 
 * Initializes the logger with the given name and MQTT client. Sets up the MQTT topic
 * prefix for log messages.
 * 
 * @param name Name of the logger (used in MQTT topics)
 * @param mqtt_client Shared pointer to MQTT client for publishing logs
 */
Logger::Logger(const std::string& name, const std::string& log_level, std::shared_ptr<MQTTClient> mqtt_client)
    : name_(name)
    , mqtt_client_(mqtt_client)
    , topic_prefix_("logs/" + name)
{
    if (log_level == "DEBUG") {
        log_level_ = LogLevel::DEBUG;
    } else if (log_level == "INFO") {
        log_level_ = LogLevel::INFO;
    } else if (log_level == "WARNING") {
        log_level_ = LogLevel::WARNING;
    } else if (log_level == "ERROR") {
        log_level_ = LogLevel::ERROR;
    } else {
        log_level_ = LogLevel::INFO;
    }
}

/**
 * @brief Logs a debug level message
 * 
 * Publishes a debug message to MQTT topic "logs/{name}/debug".
 * Debug messages contain detailed information for debugging purposes.
 * 
 * @param message The debug message to log
 */
void Logger::debug(const std::string& message) {
    if (log_level_ > LogLevel::DEBUG) {
        return;
    }
    mqtt_client_->publish(topic_prefix_ + "/debug", formatMessage(LogLevel::DEBUG, message));
}

/**
 * @brief Logs an info level message
 * 
 * Publishes an info message to MQTT topic "logs/{name}/info".
 * Info messages contain general operational information.
 * 
 * @param message The info message to log
 */
void Logger::info(const std::string& message) {
    if (log_level_ > LogLevel::INFO) {
        return;
    }
    mqtt_client_->publish(topic_prefix_ + "/info", formatMessage(LogLevel::INFO, message));
}

/**
 * @brief Logs a warning level message
 * 
 * Publishes a warning message to MQTT topic "logs/{name}/warning".
 * Warning messages indicate potentially harmful situations.
 * 
 * @param message The warning message to log
 */
void Logger::warning(const std::string& message) {
    if (log_level_ > LogLevel::WARNING) {
        return;
    }
    mqtt_client_->publish(topic_prefix_ + "/warning", formatMessage(LogLevel::WARNING, message));
}

/**
 * @brief Logs an error level message
 * 
 * Publishes an error message to MQTT topic "logs/{name}/error".
 * Error messages indicate error events that might still allow the application to continue.
 * 
 * @param message The error message to log
 */
void Logger::error(const std::string& message) {
    mqtt_client_->publish(topic_prefix_ + "/error", formatMessage(LogLevel::ERROR, message));
}

/**
 * @brief Formats a log message for MQTT publishing
 * 
 * Creates a JSON object containing the log details including timestamp,
 * log level, source, and message.
 * 
 * @param level The log level
 * @param message The log message
 * @return Formatted JSON string containing the log message
 */
std::string Logger::formatMessage(LogLevel level, const std::string& message) {
    json log_entry = {
        {"timestamp", getTimestamp()},
        {"level", static_cast<int>(level)},
        {"source", name_},
        {"message", message}
    };
    return log_entry.dump();
}

/**
 * @brief Gets the current timestamp in human-readable format
 * 
 * Formats the current system time as "YYYY-MM-DD HH:MM:SS"
 * 
 * @return Timestamp string in the format "YYYY-MM-DD HH:MM:SS"
 */
std::string Logger::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

} // namespace common 