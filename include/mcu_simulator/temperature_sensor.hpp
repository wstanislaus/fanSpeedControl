#pragma once

#include <string>
#include <chrono>
#include "common/config.hpp"

namespace mcu_simulator {

/**
 * @class TemperatureSensor
 * @brief Represents a temperature sensor with configurable interface and behavior
 * 
 * This class simulates a temperature sensor with support for different communication
 * interfaces (I2C/SPI), configurable behavior including bad sensor simulation,
 * noisy readings, and temperature simulation with configurable parameters.
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
     * @note This constructor initializes the sensor with default good status and normal operation
     */
    TemperatureSensor(int id, const std::string& name, const SensorConfig& config);

    /**
     * @brief Destructor that ensures proper cleanup
     * @note This destructor cleans up any resources used by the sensor
     */
    ~TemperatureSensor();

    /**
     * @brief Gets the sensor's unique identifier
     * @return The sensor ID
     * @note This ID is used to distinguish between multiple sensors on the same MCU
     */
    int getId() const { return id_; }

    /**
     * @brief Gets the sensor's name
     * @return The sensor name
     * @note This name is used for identification and logging purposes
     */
    const std::string& getName() const { return name_; }

    /**
     * @brief Reads the current temperature from the sensor
     * @return The temperature reading in degrees Celsius
     * @note May return invalid readings if the sensor is in a bad state
     * @note When in simulation mode, returns simulated temperature values
     */
    float readTemperature();

    /**
     * @brief Sets the status of the sensor
     * @param is_bad true to set the sensor to bad, false to set it to good
     * @note This will cause the sensor to report invalid temperatures when bad
     * @note Used for testing fault conditions and alarm systems
     */
    void setStatus(bool is_bad) { status_ = is_bad ? "Bad" : "Good"; }

    /**
     * @brief Sets whether the sensor should report noisy readings
     * @param noisy true to enable noisy readings, false to disable
     * @note When noisy is true, the sensor will add random variations to readings
     * @note Used for testing noise detection and filtering algorithms
     */
    void setNoisy(bool noisy);

    /**
     * @brief Gets the current status of the sensor
     * @return String describing the sensor's current status ("Good" or "Bad")
     * @note This status indicates the operational state of the sensor
     */
    const std::string& getStatus() const { return status_; }

    /**
     * @brief Gets the timestamp of the last temperature reading
     * @return Timestamp of the last successful temperature reading
     * @note This timestamp is used for tracking reading frequency and timing
     */
    const std::chrono::system_clock::time_point& getLastReadTime() const { return last_read_time_; }

    /**
     * @brief Raises an alarm for this sensor
     * @note This method sets the alarm flag to indicate a sensor fault condition
     * @note Used in conjunction with alarm management systems
     */
    void raiseAlarm();

    /**
     * @brief Clears an alarm for this sensor
     * @note This method clears the alarm flag when the sensor returns to normal operation
     * @note Used when sensor faults are resolved
     */
    void clearAlarm();

    /**
     * @brief Gets the alarm raised status
     * @return True if the alarm has been raised, false otherwise
     * @note This indicates whether the sensor is currently in a fault condition
     */
    bool getAlarmRaised();

    /**
     * @brief Gets the interface type as a string
     * @return String representation of the interface ("I2C" or "SPI")
     * @note This method provides human-readable interface information
     */
    std::string getInterface() const { return (config_.interface == Interface::I2C) ? "I2C" : "SPI"; }
    
    /**
     * @brief Gets the address information for the sensor
     * @return String containing the address (I2C address or SPI CS line)
     * @note For I2C sensors, returns the I2C address; for SPI sensors, returns the CS line number
     */
    std::string getAddress() const;

    /**
     * @brief Gets the noisy status of the sensor
     * @return True if the sensor is noisy, false otherwise
     * @note This indicates whether the sensor is currently adding noise to readings
     */
    bool getNoisy() const { return is_noisy_; }

    /**
     * @brief Sets the simulation parameters for temperature generation
     * @param start_temp Start temperature for simulation (°C)
     * @param end_temp End temperature for simulation (°C)
     * @param step_size Step size for temperature increments (°C)
     * @note This method configures how the sensor generates simulated temperature values
     * @note The sensor will cycle through temperatures from start_temp to end_temp in step_size increments
     */
    void setSimulationParams(const double start_temp, const double end_temp, const double step_size);

private:
    int id_;                                                    ///< Unique identifier for the sensor
    std::string name_;                                          ///< Name of the sensor
    SensorConfig config_;                                       ///< Sensor configuration
    std::string status_;                                        ///< Current status of the sensor
    bool is_noisy_;                                             ///< Whether the sensor is in noisy mode
    std::chrono::system_clock::time_point last_read_time_;      ///< Timestamp of last temperature reading
    bool alarm_raised_;                                         ///< Flag indicating if an alarm has been raised
    float previous_temperature_;                                ///< Previous temperature reading for trend analysis
    bool raising_;                                              ///< Flag indicating if the temperature is raising
    float start_temp_;                                          ///< Start temperature for simulation (°C)
    float end_temp_;                                            ///< End temperature for simulation (°C)
    float step_size_;                                           ///< Step size for simulation (°C)
};

} // namespace mcu_simulator 