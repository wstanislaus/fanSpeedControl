#pragma once

#include <string>
#include <chrono>
#include "common/config.hpp"

namespace mcu_simulator {

/**
 * @class TemperatureSensor
 * @brief Represents a temperature sensor with configurable interface and behavior
 */
class TemperatureSensor {
public:
    /**
     * @enum Interface
     * @brief Supported communication interfaces for the temperature sensor
     */
    enum class Interface {
        I2C,    ///< I2C communication interface
        SPI     ///< SPI communication interface
    };

    /**
     * @struct SensorConfig
     * @brief Configuration settings for the temperature sensor
     */
    struct SensorConfig {
        Interface interface;     ///< Communication interface type
        int i2c_address;         ///< I2C address (only used if interface is I2C)
        int cs_line;            ///< Chip select line (only used if interface is SPI)
    };

    /**
     * @brief Constructs a new temperature sensor
     * @param id Unique identifier for the sensor
     * @param name Name of the sensor
     * @param config Sensor configuration including interface settings
     */
    TemperatureSensor(int id, const std::string& name, const SensorConfig& config);

    /**
     * @brief Destructor that ensures proper cleanup
     */
    ~TemperatureSensor();

    /**
     * @brief Gets the sensor's unique identifier
     * @return The sensor ID
     */
    int getId() const { return id_; }

    /**
     * @brief Gets the sensor's name
     * @return The sensor name
     */
    const std::string& getName() const { return name_; }

    /**
     * @brief Reads the current temperature from the sensor
     * @return The temperature reading in degrees Celsius
     * @note May return invalid readings if the sensor is in a bad state
     */
    float readTemperature();

    /**
     * @brief Makes the sensor report bad readings (for testing)
     * @note This will cause the sensor to report invalid temperatures
     */
    void makeBad();

    /**
     * @brief Sets whether the sensor should report noisy readings
     * @param noisy true to enable noisy readings, false to disable
     * @note When noisy is true, the sensor will add random variations to readings
     */
    void setNoisy(bool noisy);

    /**
     * @brief Gets the current status of the sensor
     * @return String describing the sensor's current status
     */
    const std::string& getStatus() const { return status_; }

    /**
     * @brief Gets the timestamp of the last temperature reading
     * @return Timestamp of the last successful temperature reading
     */
    const std::chrono::system_clock::time_point& getLastReadTime() const { return last_read_time_; }

    /**
     * @brief Raises an alarm
     */
    void raiseAlarm();

    /**
     * @brief Clears an alarm
     */
    void clearAlarm();

    /**
     * @brief Gets the alarm raised status
     * @return True if the alarm has been raised, false otherwise
     */
    bool getAlarmRaised();

private:
    int id_;                                                    ///< Unique identifier for the sensor
    std::string name_;                                          ///< Name of the sensor
    SensorConfig config_;                                       ///< Sensor configuration
    std::string status_;                                        ///< Current status of the sensor
    bool is_noisy_;                                             ///< Whether the sensor is in noisy mode
    std::chrono::system_clock::time_point last_read_time_;      ///< Timestamp of last temperature reading
    bool alarm_raised_;                                         ///< Flag indicating if an alarm has been raised
    float previous_temperature_;                                ///< Previous temperature reading
    bool raising_;                                              ///< Flag inddicate if the temperature is raising
    float start_temp_;                                          ///< Start temperature for simulation
    float end_temp_;                                            ///< End temperature for simulation
    float step_size_;                                           ///< Step size for simulation
};

} // namespace mcu_simulator 