#pragma once

#include "common/mqtt_client.hpp"
#include <string>
#include <memory>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace common {

/**
 * @enum LogLevel
 * @brief Defines the severity levels for log messages
 */
enum class LogLevel {
    DEBUG,      ///< Debug level - detailed information for debugging
    INFO,       ///< Info level - general operational information
    WARNING,    ///< Warning level - potentially harmful situations
    ERROR       ///< Error level - error events that might still allow the application to continue
};

/**
 * @class Logger
 * @brief Provides logging functionality with MQTT integration
 * 
 * This class handles logging of messages at different severity levels.
 * Log messages are published to MQTT topics for monitoring and debugging purposes.
 */
class Logger {
public:
    /**
     * @brief Constructs a new Logger instance
     * @param name Name of the logger (used in MQTT topics)
     * @param log_level Log level for the logger
     * @param mqtt_client Shared pointer to MQTT client for publishing logs
     */
    Logger(const std::string& name, const std::string& log_level, std::shared_ptr<MQTTClient> mqtt_client);
    
    /**
     * @brief Logs a debug level message
     * @param message The debug message to log
     */
    void debug(const std::string& message);

    /**
     * @brief Logs an info level message
     * @param message The info message to log
     */
    void info(const std::string& message);

    /**
     * @brief Logs a warning level message
     * @param message The warning message to log
     */
    void warning(const std::string& message);

    /**
     * @brief Logs an error level message
     * @param message The error message to log
     */
    void error(const std::string& message);

private:
    /**
     * @brief Formats a log message for MQTT publishing
     * @param level The log level
     * @param message The log message
     * @return Formatted string containing the log message
     */
    std::string formatMessage(LogLevel level, const std::string& message);

    /**
     * @brief Gets the current timestamp in ISO 8601 format
     * @return Timestamp string
     */
    std::string getTimestamp();
    
    std::string name_;                                          ///< Name of the logger
    std::shared_ptr<MQTTClient> mqtt_client_;                   ///< MQTT client for publishing logs
    std::string topic_prefix_;                                  ///< MQTT topic prefix for log messages
    LogLevel log_level_;                                         ///< Log level for the logger
};

} // namespace common 