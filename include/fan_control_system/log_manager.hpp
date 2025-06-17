#pragma once

#include <memory>
#include <thread>
#include <atomic>
#include <string>
#include <fstream>
#include <mutex>
#include <queue>
#include <chrono>
#include <yaml-cpp/yaml.h>
#include <mosquitto.h>
#include <nlohmann/json.hpp>
#include <condition_variable>
#include "common/mqtt_client.hpp"
#include "common/logger.hpp"

using json = nlohmann::json;

namespace fan_control_system {

/**
 * @struct LogEntry
 * @brief Structure representing a single log entry with metadata
 */
struct LogEntry {
    std::string timestamp;    ///< ISO 8601 formatted timestamp
    std::string level;        ///< Log level (DEBUG, INFO, WARNING, ERROR)
    std::string source;       ///< Source component of the log entry
    std::string message;      ///< Log message content
    nlohmann::json metadata;  ///< Additional metadata in JSON format
};

/**
 * @class LogManager
 * @brief Manages system-wide logging with file rotation and MQTT publishing
 * 
 * This class handles logging of system events with support for different log levels,
 * file-based storage with rotation, and MQTT publishing for real-time monitoring.
 * It maintains a queue of log entries and processes them asynchronously.
 */
class LogManager {
public:
    /**
     * @brief Constructs a new Log Manager instance
     * @param config YAML configuration node containing logging settings
     * @param mqtt_settings MQTT communication settings
     */
    explicit LogManager(const YAML::Node& config, const common::MQTTClient::Settings& mqtt_settings);

    /**
     * @brief Destructor that ensures proper cleanup
     */
    ~LogManager();

    /**
     * @brief Starts the log manager
     * @return true if the manager started successfully, false otherwise
     */
    bool start();

    /**
     * @brief Stops the log manager
     * @note This will stop all log processing and MQTT publishing
     */
    void stop();

    /**
     * @brief Adds a new log entry to the queue
     * @param entry The log entry to add
     */
    void add_log(const LogEntry& entry);

private:
    /**
     * @brief Initializes MQTT connection and components
     * @return true if initialization was successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Initializes the log file
     * @return true if initialization was successful, false otherwise
     */
    bool initialize_log_file();

    /**
     * @brief Rotates the log file if it exceeds size limits
     * @return true if rotation was successful, false otherwise
     */
    bool rotate_log_file();

    /**
     * @brief Writes a log entry to the current log file
     * @param entry The log entry to write
     */
    void write_log_entry(const LogEntry& entry);

    /**
     * @brief MQTT message callback for receiving log-related messages
     * @param mosq Pointer to the mosquitto instance
     * @param obj User data pointer
     * @param msg Pointer to the received message
     */
    static void mqtt_message_callback(struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg);

    /**
     * @brief Main thread function for log processing
     */
    void main_thread_function();

    // Configuration
    YAML::Node config_;                                    ///< Loaded configuration data

    // Name
    std::string name_;                                     ///< Name of the log manager
    
    // Log file settings
    std::string log_file_path_;                           ///< Path to the current log file
    std::string log_file_base_name_;                      ///< Base name for log files
    size_t max_log_size_bytes_;                           ///< Maximum size of a log file
    size_t max_log_files_;                                ///< Maximum number of log files to keep
    std::ofstream log_file_;                              ///< Current log file stream
    size_t current_log_size_;                             ///< Current size of the log file
    common::LogLevel log_level_;                          ///< Current log level
    
    // Log queue
    std::queue<LogEntry> log_queue_;                      ///< Queue of pending log entries
    mutable std::mutex queue_mutex_;                      ///< Mutex for thread-safe queue access
    std::condition_variable queue_cv_;                    ///< Condition variable for queue processing

    // MQTT settings and components
    common::MQTTClient::Settings mqtt_settings_;           ///< MQTT communication settings
    std::shared_ptr<common::MQTTClient> mqtt_client_;      ///< MQTT client for communication

    // Common components
    std::unique_ptr<common::Logger> logger_;               ///< Logger for log manager
    
    // Thread control
    std::thread main_thread_;                             ///< Main log processing thread
    std::atomic<bool> running_{false};                    ///< Flag indicating if manager is running
};

} // namespace fan_control_system 