#include "mcu_simulator/temperature_sensor.hpp"
#include <random>
#include <chrono>

namespace mcu_simulator {

/**
 * @brief Constructs a new TemperatureSensor instance
 * 
 * Initializes a temperature sensor with the specified ID, name, and configuration.
 * Sets up the sensor with default simulation parameters from the configuration
 * and initializes the sensor state.
 * 
 * @param id Unique identifier for the sensor
 * @param name Human-readable name for the sensor
 * @param config Sensor configuration including interface type and address
 */
TemperatureSensor::TemperatureSensor(int id, const std::string& name, const SensorConfig& config)
    : id_(id)
    , name_(name)
    , config_(config)
    , status_("Good")
    , is_noisy_(false)
    , last_read_time_(std::chrono::system_clock::now())
    , alarm_raised_(false)
    , raising_(true)
{
    // Get temperature simulation configuration
    auto& config_obj = common::Config::getInstance();
    common::TemperatureSimConfig sim_config = config_obj.getTemperatureSimConfig();
    previous_temperature_ = sim_config.start_temp;
    start_temp_ = sim_config.start_temp;
    end_temp_ = sim_config.end_temp;
    step_size_ = sim_config.step_size;
}

/**
 * @brief Destructor for TemperatureSensor
 * 
 * Performs cleanup when the temperature sensor is destroyed.
 */
TemperatureSensor::~TemperatureSensor() {
}

/**
 * @brief Reads the current temperature from the sensor
 * 
 * Simulates temperature reading with configurable behavior based on sensor state.
 * If the sensor is marked as bad, returns a very low temperature (5°C).
 * Otherwise, simulates temperature oscillation between start and end temperatures
 * with optional noise addition for noisy sensors.
 * 
 * @return Current temperature reading in degrees Celsius
 */
float TemperatureSensor::readTemperature() {
    // Update last read time
    last_read_time_ = std::chrono::system_clock::now();

    // If sensor is bad, return a very low temperature
    if (status_ == "Bad") {
        return 5.0f;  // 5 degrees Celsius
    }

    if (raising_) {
        previous_temperature_ += step_size_;
    } else {
        previous_temperature_ -= step_size_;
    }

    if (previous_temperature_ < start_temp_) {
        raising_ = true;
        previous_temperature_ = start_temp_;
    } else if (previous_temperature_ > end_temp_) {
        raising_ = false;
        previous_temperature_ = end_temp_;
    }

    float base_temp = previous_temperature_;

    // If sensor is noisy, add some random variation
    if (is_noisy_) {
        static std::uniform_real_distribution<float> noise_dist(-50.0f, 50.0f);
        static std::random_device rd;
        static std::mt19937 gen(rd());
        base_temp += noise_dist(gen);
        // Range has to between 10 and 100
        if (base_temp < 10.0f) {
            base_temp = 10.0f;
        }
        if (base_temp > 100.0f) {
            base_temp = 100.0f;
        }
    }

    return base_temp;
}

/**
 * @brief Sets the noisy state of the sensor
 * 
 * When set to noisy, the sensor will add random variations to its temperature
 * readings to simulate sensor noise or interference.
 * 
 * @param noisy Whether the sensor should behave as noisy (true) or normal (false)
 */
void TemperatureSensor::setNoisy(bool noisy) {
    is_noisy_ = noisy;
}

/**
 * @brief Sets the temperature simulation parameters
 * 
 * Configures the temperature simulation behavior by setting the start temperature,
 * end temperature, and step size for the oscillating temperature pattern.
 * 
 * @param start_temp Starting temperature for the simulation cycle
 * @param end_temp Ending temperature for the simulation cycle
 * @param step_size Temperature change per simulation step
 */
void TemperatureSensor::setSimulationParams(const double start_temp, const double end_temp, const double step_size) {
    start_temp_ = start_temp;
    end_temp_ = end_temp;
    step_size_ = step_size;
}

/**
 * @brief Raises an alarm for this sensor
 * 
 * Marks the sensor as having an active alarm condition.
 */
void TemperatureSensor::raiseAlarm() {
    alarm_raised_ = true;
}

/**
 * @brief Clears the alarm for this sensor
 * 
 * Removes the alarm condition from the sensor.
 */
void TemperatureSensor::clearAlarm() {
    alarm_raised_ = false;
}

/**
 * @brief Checks if an alarm is currently raised for this sensor
 * 
 * @return true if an alarm is active, false otherwise
 */
bool TemperatureSensor::getAlarmRaised() {
    return alarm_raised_;
}

/**
 * @brief Gets the sensor address as a formatted string
 * 
 * Returns the sensor address in hexadecimal format. For I2C sensors,
 * returns the I2C address. For SPI sensors, returns the CS line number.
 * 
 * @return Formatted address string in hexadecimal format (e.g., "0x48")
 */
std::string TemperatureSensor::getAddress() const {
    if (config_.interface == Interface::I2C) {
        char hex[8];
        snprintf(hex, sizeof(hex), "0x%02X", config_.i2c_address);
        return hex;
    } else {
        char hex[8];
        snprintf(hex, sizeof(hex), "0x%02X", config_.cs_line);
        return hex;
    }
}

} // namespace mcu_simulator