#include "mcu_simulator/mcu.hpp"
#include "mcu_simulator/temperature_sensor.hpp"
#include "common/config.hpp"
#include "common/mqtt_client.hpp"
#include "common/logger.hpp"
#include "common/alarm.hpp"
#include <thread>
#include <iostream>
#include <nlohmann/json.hpp>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <algorithm>
#include <cmath>
#include <yaml-cpp/yaml.h>

using json = nlohmann::json;

namespace mcu_simulator {

/**
 * @brief Constructs a new MCU instance
 * 
 * Initializes the MCU with the given name and number of sensors. Creates
 * temperature sensors based on configuration from the YAML file. Each sensor
 * is configured with either I2C or SPI interface settings.
 * 
 * @param name Name of the MCU
 * @param num_sensors Number of temperature sensors to create
 * @param temp_settings Temperature monitoring settings
 * @param mqtt_settings MQTT communication settings
 * @param sensor_config YAML node containing sensor configuration
 */
MCU::MCU(const std::string& name, int num_sensors, 
    const TemperatureSettings& temp_settings, const common::MQTTClient::Settings& mqtt_settings, 
    const YAML::Node& sensor_config, const std::string& config_file)
    : name_(name)
    , running_(false)
    , last_read_time_(std::chrono::system_clock::now())
    , sensor_readings_(num_sensors)
    , temp_settings_(temp_settings)
    , mqtt_settings_(mqtt_settings)
    , config_file_(config_file)
    , alarm_raised_(false)
    , is_faulty_(false)
{
    // Create temperature sensors based on config
    for (int i = 0; i < num_sensors; ++i) {
        TemperatureSensor::SensorConfig sensor_cfg;
        std::string sensor_name = "Sensor" + std::to_string(i + 1);
        
        // Get sensor config from YAML
        const auto& mcu_config = sensor_config["Sensors"][sensor_name];
        
        if (mcu_config["Interface"].as<std::string>() == "I2C") {
            sensor_cfg.interface = TemperatureSensor::Interface::I2C;
            sensor_cfg.i2c_address = std::stoi(mcu_config["Address"].as<std::string>(), nullptr, 16);
            sensor_cfg.cs_line = 0;  // Not used for I2C
        } else if (mcu_config["Interface"].as<std::string>() == "SPI") {
            sensor_cfg.interface = TemperatureSensor::Interface::SPI;
            sensor_cfg.cs_line = mcu_config["CSLine"].as<int>();
            sensor_cfg.i2c_address = 0;  // Not used for SPI
        }

        sensors_.push_back(std::make_unique<TemperatureSensor>(
            i + 1,
            sensor_name,
            sensor_cfg
        ));
    }
}

/**
 * @brief Destructor that ensures proper cleanup
 * 
 * Stops the MCU and cleans up resources when the instance is destroyed.
 */
MCU::~MCU() {
    stop();
}

/**
 * @brief Initializes the MCU and its components
 * 
 * Performs the following initialization steps:
 * 1. Loads configuration from the YAML file
 * 2. Initializes MQTT client for communication
 * 3. Sets up logger and alarm system
 * 
 * @return true if initialization was successful, false otherwise
 */
bool MCU::initialize() {
    // Initialize common components
    auto& config = common::Config::getInstance();
    if (!config.load(config_file_)) {
        std::cerr << "Failed to load config file" << std::endl;
        return false;
    }

    // Get log level for MCU from config
    YAML::Node config_node = config.getConfig();
    auto log_level = config_node["AppLogLevel"]["MCUSimulator"].as<std::string>();

    // Initialize MQTT client
    mqtt_client_ = std::make_shared<common::MQTTClient>(name_, mqtt_settings_);
    if (!mqtt_client_->initialize() || !mqtt_client_->connect()) {
        std::cerr << "Failed to initialize MQTT client for MCU " << name_ << std::endl;
        return false;
    }

    // Initialize logger and alarm
    logger_ = std::make_unique<common::Logger>(name_, log_level, mqtt_client_);
    alarm_ = std::make_unique<common::Alarm>(name_, mqtt_client_);

    logger_->info("MCU initialized successfully");
    return true;
}

/**
 * @brief Starts the MCU's temperature monitoring
 * 
 * Starts a background thread that continuously reads temperatures
 * from all sensors and publishes them via MQTT at appropriate intervals.
 */
void MCU::start() {
    if (running_) return;
    running_ = true;
    logger_->info("MCU started");

    // Start temperature reading thread
    std::thread([this]() {
        while (running_) {
            readAndPublishTemperatures();
            checkAlarm();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }).detach();
}

/**
 * @brief Stops the MCU's temperature monitoring
 * 
 * Stops the background thread and cleans up resources.
 */
void MCU::stop() {
    if (!running_) return;
    running_ = false;
    logger_->info("MCU stopped");
}

void MCU::checkAlarm() {
    // Check for MCUs sensor status, if more than half of the sensors are bad per MCU, raise an alarm
    int bad_sensors = 0;
    for (const auto& sensor : sensors_) {
        if (sensor->getStatus() == "Bad") {
            bad_sensors++;
        }
    }
    if (bad_sensors > sensors_.size() / 2) {
        logger_->error("MCU " + name_ + " has more than half of its sensors bad");
        alarm_->raise(common::AlarmSeverity::CRITICAL, "MCU " + name_ + " has more than half of its sensors bad");
        alarm_raised_ = true;
    }
    // Check if the alarm has been raised and sensors are good, clear the alarm
    else if (alarm_raised_) {
        alarm_->clear("MCU " + name_ + " is back to normal");
        alarm_raised_ = false;
    }
}

/**
 * @brief Makes a specific sensor report bad readings
 * 
 * Marks the specified sensor as bad, which will cause it to report
 * invalid temperatures. Raises a medium severity alarm.
 * 
 * @param sensor_id ID of the sensor to make bad (1-based)
 * @return true if the operation was successful, false if sensor_id is invalid
 */
bool MCU::makeSensorBad(int sensor_id, bool is_bad) {
    if (sensor_id < 1 || sensor_id > sensors_.size()) {
        logger_->warning("Invalid sensor ID: " + std::to_string(sensor_id));
        return false;
    }
    sensors_[sensor_id - 1]->setStatus(is_bad);
    logger_->info("Sensor " + std::to_string(sensor_id) + " marked as bad");
    alarm_->raise(common::AlarmSeverity::MEDIUM, "MCU " + name_ + " Sensor " + std::to_string(sensor_id) + " marked as bad");
    return true;
}

/**
 * @brief Makes a specific sensor report noisy readings
 * 
 * Sets the specified sensor to noisy mode, which will add random
 * variations to its temperature readings. Raises a low severity alarm.
 * 
 * @param sensor_id ID of the sensor to make noisy (1-based)
 * @return true if the operation was successful, false if sensor_id is invalid
 */
bool MCU::makeSensorNoisy(int sensor_id, bool is_noisy) {
    if (sensor_id < 1 || sensor_id > sensors_.size()) {
        logger_->warning("Invalid sensor ID: " + std::to_string(sensor_id));
        return false;
    }
    sensors_[sensor_id - 1]->setNoisy(is_noisy);
    logger_->info("Sensor " + std::to_string(sensor_id) + " set to noisy mode");
    alarm_->raise(common::AlarmSeverity::LOW, "MCU " + name_ + " Sensor " + std::to_string(sensor_id) + " set to noisy mode");
    return true;
}

/**
 * @brief Formats a timestamp to a human-readable string
 * 
 * Converts a system clock time point to a string in the format
 * "YYYY-MM-DD HH:MM:SS".
 * 
 * @param tp The timestamp to format
 * @return Formatted timestamp string
 */
std::string MCU::formatTimestamp(const std::chrono::system_clock::time_point& tp) {
    auto time = std::chrono::system_clock::to_time_t(tp);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

/**
 * @brief Checks if a series of temperature readings shows erratic behavior
 * 
 * Calculates the mean and standard deviation of the readings. If the
 * standard deviation exceeds the erratic threshold, the readings are
 * considered erratic.
 * 
 * @param readings Deque containing the last 5 temperature readings
 * @return true if readings are erratic, false otherwise
 */
bool MCU::checkErraticReadings(const std::deque<float>& readings) {
    if (readings.size() < 5) return false;

    // Calculate mean and standard deviation
    float sum = 0.0f;
    for (float reading : readings) {
        sum += reading;
    }
    float mean = sum / readings.size();

    float sum_sq_diff = 0.0f;
    for (float reading : readings) {
        float diff = reading - mean;
        sum_sq_diff += diff * diff;
    }
    float std_dev = std::sqrt(sum_sq_diff / readings.size());

    // If standard deviation is too high, readings are erratic
    return std_dev > temp_settings_.erratic_threshold;
}

/**
 * @brief Calculates the next publish interval based on temperature
 * 
 * Determines how frequently temperature readings should be published
 * based on the current temperature and configured intervals.
 * 
 * @param temperature Current temperature reading
 * @return Time interval until next publish
 */
std::chrono::seconds MCU::calculatePublishInterval(float temperature) {
    if (temperature < temp_settings_.bad_threshold) {
        // Use the first interval for bad readings
        return std::chrono::seconds(temp_settings_.publish_intervals[0].interval_seconds);
    }

    // Find the appropriate interval based on temperature
    for (const auto& interval : temp_settings_.publish_intervals) {
        if (temperature >= interval.min_temp && temperature <= interval.max_temp) {
            return std::chrono::seconds(interval.interval_seconds);
        }
    }

    // Default to the first interval if no range matches
    return std::chrono::seconds(temp_settings_.publish_intervals[0].interval_seconds);
}

/**
 * @brief Reads temperatures from all sensors and publishes them
 * 
 * This function is called periodically by the background thread. It:
 * 1. Reads temperatures from all sensors
 * 2. Updates reading history
 * 3. Checks for erratic readings and bad temperatures
 * 4. Publishes temperature data via MQTT if conditions are met
 * 
 * The publish interval is determined by the highest temperature reading
 * and configured intervals. Data is also published immediately if any
 * sensor shows erratic readings or bad temperatures.
 */
void MCU::readAndPublishTemperatures() {
    json sensor_data = json::array();
    auto now = std::chrono::system_clock::now();
    bool should_publish = false;
    
    for (size_t i = 0; i < sensors_.size(); ++i) {
        float temp = sensors_[i]->readTemperature();
        
        // Update reading history
        auto& readings = sensor_readings_[i];
        readings.push_back(temp);
        if (readings.size() > 5) {
            readings.pop_front();
        }
                // Status can be Good, Bad, Noisy
        std::string status = sensors_[i]->getStatus();
        if (sensors_[i]->getNoisy()) {
            status = "Noisy";
        } 

        // Check for erratic readings
        if (readings.size() == 5 && checkErraticReadings(readings)) {
            //Check if not nosisy falg set
            if (!sensors_[i]->getNoisy()) {
                status = "Bad";
            }
            logger_->warning("Sensor " + std::to_string(i + 1) + " showing erratic readings");
            alarm_->raise(common::AlarmSeverity::HIGH, 
                "MCU " + name_ + " Sensor " + std::to_string(i + 1) + " showing erratic readings");
            should_publish = true;
            sensors_[i]->raiseAlarm();
        }

        // Check if temperature is below threshold
        if (temp < temp_settings_.bad_threshold) {
            if (!sensors_[i]->getNoisy()) {
                status = "Bad";
            }
            logger_->error("Sensor " + std::to_string(i + 1) + " temperature below threshold: " + 
                std::to_string(temp));
            alarm_->raise(common::AlarmSeverity::CRITICAL, 
                "MCU " + name_ + " Sensor " + std::to_string(i + 1) + " temperature below threshold: " + 
                std::to_string(temp));
            should_publish = true;
            sensors_[i]->raiseAlarm();
        }

        // Check if the alarm has been raised and sensors are good, clear the alarm
        if (sensors_[i]->getAlarmRaised() && sensors_[i]->getStatus() == "Good") {
            alarm_->clear("MCU " + name_ + " Sensor " + std::to_string(i + 1) + " is back to normal");
            sensors_[i]->clearAlarm();
        }

        // Log temperature reading
        logger_->debug("Sensor " + std::to_string(i + 1) + " temperature: " + 
            std::to_string(temp) + "°C");

        json sensor_json = {
            {"SensorID", sensors_[i]->getId()},
            {"ReadAt", formatTimestamp(sensors_[i]->getLastReadTime())},
            {"Value", std::round(temp * 100.0) / 100.0},  // Round to 2 decimal places
            {"Status", status}
        };
        sensor_data.push_back(sensor_json);
    }

    // Calculate next publish interval based on highest temperature
    float max_temp = -999.9f;
    for (const auto& readings : sensor_readings_) {
        if (!readings.empty()) {
            max_temp = std::max(max_temp, readings.back());
        }
    }

    // Determine if we should publish based on time interval
    auto time_since_last = std::chrono::duration_cast<std::chrono::seconds>(
        now - last_read_time_).count();
    auto next_interval = calculatePublishInterval(max_temp).count();
    
    if (should_publish || time_since_last >= next_interval) {
        json message = {
            {"MCU", name_},
            {"NoOfTempSensors", sensors_.size()},
            {"MsgTimestamp", formatTimestamp(now)},
            {"SensorData", sensor_data}
        };

        std::string topic = "sensors/" + name_ + "/temperature";
        std::string payload = message.dump();

        if (!mqtt_client_->publish(topic, payload)) {
            logger_->error("Failed to publish temperature data");
            alarm_->raise(common::AlarmSeverity::MEDIUM, "MCU " + name_ + " Failed to publish temperature data");
        } else {
            logger_->debug("Published temperature data for " + name_);
        }
        last_read_time_ = now;
    }
}

bool MCU::getSensorTemperature(const std::string& sensor_id, double& temperature) {
    try {
        int id = std::stoi(sensor_id) - 1;
        if (id < 0 || id >= sensors_.size() || sensors_[id]->getStatus() == "bad") {
            return false;
        }
        temperature = sensors_[id]->readTemperature();
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

int MCU::getActiveSensorCount() const {
    return std::count_if(sensors_.begin(), sensors_.end(),
                        [](const auto& sensor) { return sensor->getStatus() != "bad"; });
}

void MCU::setFaulty(bool is_faulty) {
    is_faulty_ = is_faulty;
    if (is_faulty_) {
        logger_->error("MCU " + name_ + " set to faulty state");
        alarm_->raise(common::AlarmSeverity::HIGH, "MCU " + name_ + " set to faulty state");
    } else {
        logger_->info("MCU " + name_ + " set to normal state");
        alarm_->clear("MCU " + name_ + " is back to normal");
    }
}

} // namespace mcu_simulator 