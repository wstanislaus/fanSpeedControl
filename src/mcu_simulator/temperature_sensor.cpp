#include "mcu_simulator/temperature_sensor.hpp"
#include <random>
#include <chrono>

namespace mcu_simulator {

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

TemperatureSensor::~TemperatureSensor() {
}

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

void TemperatureSensor::setNoisy(bool noisy) {
    is_noisy_ = noisy;
}

void TemperatureSensor::setSimulationParams(const double start_temp, const double end_temp, const double step_size) {
    start_temp_ = start_temp;
    end_temp_ = end_temp;
    step_size_ = step_size;
}

void TemperatureSensor::raiseAlarm() {
    alarm_raised_ = true;
}

void TemperatureSensor::clearAlarm() {
    alarm_raised_ = false;
}

bool TemperatureSensor::getAlarmRaised() {
    return alarm_raised_;
}

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