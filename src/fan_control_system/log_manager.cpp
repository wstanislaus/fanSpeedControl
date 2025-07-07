#include "fan_control_system/log_manager.hpp"
#include <iostream>
#include <stdexcept>
#include <experimental/filesystem>
#include <nlohmann/json.hpp>
#include <iomanip>
#include <sstream>

namespace fs = std::experimental::filesystem;

namespace fan_control_system {

/**
 * @brief Constructs a new LogManager instance
 * 
 * Initializes the log manager with configuration and MQTT settings.
 * Sets up log file path, base name, and size limits from the configuration.
 * 
 * @param config YAML configuration node containing logging settings
 * @param mqtt_settings MQTT client settings for publishing logs
 */
LogManager::LogManager(const YAML::Node& config, const common::MQTTClient::Settings& mqtt_settings)
    : config_(config), mqtt_settings_(mqtt_settings), current_log_size_(0) {
    log_file_path_ = config_["Logging"]["FilePath"].as<std::string>();
    log_file_base_name_ = config_["Logging"]["FileName"].as<std::string>();
    max_log_size_bytes_ = static_cast<size_t>(config_["Logging"]["MaxFileSizeMB"].as<double>() * 1024 * 1024);
    max_log_files_ = config_["Logging"]["MaxFiles"].as<size_t>();
    name_ = "LogManager";
}

/**
 * @brief Destructor for LogManager instance
 * 
 * Ensures the log manager is properly stopped and the log file is closed.
 */
LogManager::~LogManager() {
    stop();
    if (log_file_.is_open()) {
        log_file_.close();
    }
}

/**
 * @brief Starts the log manager
 * 
 * Initializes MQTT client and log file, starts the main processing thread.
 * 
 * @return true if startup was successful, false otherwise
 */
bool LogManager::start() {
    if (running_) {
        return true;
    }

    if (!initialize() || !initialize_log_file()) {
        return false;
    }

    running_ = true;
    main_thread_ = std::thread(&LogManager::main_thread_function, this);
    return true;
}

/**
 * @brief Stops the log manager
 * 
 * Stops the main processing thread and cleans up resources.
 */
void LogManager::stop() {
    if (!running_) {
        return;
    }

    running_ = false;
    queue_cv_.notify_all();
    if (main_thread_.joinable()) {
        main_thread_.join();
    }
}

/**
 * @brief Adds a log entry to the processing queue
 * 
 * @param entry Log entry to be processed and written to the log file
 */
void LogManager::add_log(const LogEntry& entry) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    log_queue_.push(entry);
    queue_cv_.notify_one();
}

/**
 * @brief Initializes the log manager
 * 
 * Sets up MQTT client, logger, and subscribes to log topics.
 * Configures the log level from the configuration.
 * 
 * @return true if initialization was successful, false otherwise
 */
bool LogManager::initialize() {
    mqtt_client_ = std::make_shared<common::MQTTClient>(name_, mqtt_settings_);
    if (!mqtt_client_->initialize() || !mqtt_client_->connect()) {
        std::cerr << "Failed to initialize MQTT client" << std::endl;
        return false;
    }

    // Initialize logger
    auto log_level = config_["AppLogLevel"]["FanControlSystem"][name_].as<std::string>();
    logger_ = std::make_unique<common::Logger>(name_, log_level, mqtt_client_);
    logger_->info("Log Manager initialized successfully");

    // Subscribe to log topics
    mqtt_client_->subscribe("logs/#", 0);
    mqtt_client_->set_message_callback(&LogManager::mqtt_message_callback, this);

    // Get log level from config
    std::string log_level_str = config_["Logging"]["Level"].as<std::string>();
    if (log_level_str == "DEBUG") {
        log_level_ = common::LogLevel::DEBUG;
    } else if (log_level_str == "INFO") {
        log_level_ = common::LogLevel::INFO;
    } else if (log_level_str == "WARNING") {
        log_level_ = common::LogLevel::WARNING;
    } else if (log_level_str == "ERROR") {
        log_level_ = common::LogLevel::ERROR;
    } else {
        log_level_ = common::LogLevel::INFO;
    }

    return true;
}

/**
 * @brief Initializes the log file
 * 
 * Creates the log directory if it doesn't exist and opens the log file.
 * Gets the current file size if the file exists.
 * 
 * @return true if initialization was successful, false otherwise
 */
bool LogManager::initialize_log_file() {
    try {
        // Combine path and filename
        fs::path full_path = fs::path(log_file_path_) / log_file_base_name_;
        
        // Create log directory if it doesn't exist
        if (!fs::exists(log_file_path_)) {
            fs::create_directories(fs::path(log_file_path_));
        }
        
        std::cout << "Attempting to create log file at: " << full_path.string() << std::endl;

        // Open the log file
        log_file_.open(full_path, std::ios::app);
        if (!log_file_.is_open()) {
            std::cerr << "Failed to open log file at: " << full_path.string() << std::endl;
            return false;
        }

        // Get current file size
        if (fs::exists(full_path)) {
            current_log_size_ = fs::file_size(full_path);
        } else {
            current_log_size_ = 0;
        }
        
        std::cout << "Log file initialized successfully at: " << full_path.string() << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error initializing log file: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief Rotates the log files
 * 
 * Closes the current log file and rotates existing log files.
 * Creates a new log file for writing.
 * 
 * @return true if rotation was successful, false otherwise
 */
bool LogManager::rotate_log_file() {
    try {
        // Close current log file
        log_file_.close();

        fs::path full_path = fs::path(log_file_path_) / log_file_base_name_;

        // Rotate existing log files
        for (size_t i = max_log_files_ - 1; i > 0; --i) {
            fs::path old_path = fs::path(log_file_path_) / 
                              (log_file_base_name_ + "_" + std::to_string(i) + ".log");
            fs::path new_path = fs::path(log_file_path_) / 
                              (log_file_base_name_ + "_" + std::to_string(i + 1) + ".log");
            
            if (fs::exists(old_path)) {
                if (i == max_log_files_ - 1) {
                    fs::remove(old_path);
                } else {
                    fs::rename(old_path, new_path);
                }
            }
        }

        // Rename current log file
        fs::rename(full_path, 
                  fs::path(log_file_path_)/ 
                  (log_file_base_name_ + "_1.log"));

        // Open new log file
        log_file_.open(full_path, std::ios::app);
        if (!log_file_.is_open()) {
            return false;
        }

        current_log_size_ = 0;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error rotating log file: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief Writes a log entry to the log file
 * 
 * Formats the log entry as JSON and writes it to the file.
 * Rotates the log file if the size limit is reached.
 * 
 * @param entry Log entry to be written
 */
void LogManager::write_log_entry(const LogEntry& entry) {
    nlohmann::json log_json = {
        {"timestamp", entry.timestamp},
        {"level", entry.level},
        {"source", entry.source},
        {"message", entry.message}
    };

    std::string log_line = log_json.dump() + "\n";
    log_file_ << log_line;
    log_file_.flush();

    // Get the actual file size from the filesystem
    fs::path full_path = fs::path(log_file_path_) / log_file_base_name_;
    if (fs::exists(full_path)) {
        current_log_size_ = fs::file_size(full_path);
    } else {
        current_log_size_ = 0;
    }
    
    if (current_log_size_ >= max_log_size_bytes_) {
        rotate_log_file();
    }
}

/**
 * @brief Callback function for MQTT messages
 * 
 * Processes incoming log messages from MQTT and adds them to the log queue
 * if they meet the configured log level.
 * 
 * @param mosq Mosquitto instance
 * @param obj User data (LogManager instance)
 * @param msg Received MQTT message
 */
void LogManager::mqtt_message_callback(
    struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg) {
    auto* manager = static_cast<LogManager*>(obj);
    if (!manager) {
        std::cerr << "LogManager: Invalid manager pointer in callback" << std::endl;
        return;
    }

    try {
        auto json = nlohmann::json::parse(static_cast<const char*>(msg->payload));
        
        // Convert numeric level to string
        std::string level_str;
        int level_num = json["level"].get<int>();
        if (level_num < static_cast<int>(manager->log_level_)) {
            // If the level is less than the log level configured, don't process the message or log to log file.
            return;
        }

        switch (level_num) {
            case 0:
                level_str = "DEBUG";
                break;
            case 1:
                level_str = "INFO";
                break;
            case 2:
                level_str = "WARNING";
                break;
            case 3:
                level_str = "ERROR";
                break;
            default:
                level_str = "UNKNOWN";
                break;
        }

        LogEntry entry{
            json["timestamp"],
            level_str,  // Use the converted string level
            json["source"],
            json["message"],
            json  // Use entire JSON as metadata
        };
        manager->add_log(entry);
    } catch (const std::exception& e) {
        std::cerr << "Error processing MQTT message: " << e.what() << std::endl;
    }
}

/**
 * @brief Main thread function for the log manager
 * 
 * Processes log entries from the queue and writes them to the log file.
 * Handles log file rotation when size limits are reached.
 */
void LogManager::main_thread_function() {
    while (running_) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        queue_cv_.wait(lock, [this] { return !log_queue_.empty() || !running_; });

        while (!log_queue_.empty()) {
            LogEntry entry = log_queue_.front();
            log_queue_.pop();
            lock.unlock();

            write_log_entry(entry);

            lock.lock();
        }
    }
}

} // namespace fan_control_system 