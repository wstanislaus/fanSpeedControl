#pragma once

#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "common/config.hpp"
#include "mcu_simulator.grpc.pb.h"
#include "fan_control_system.grpc.pb.h"

namespace cli {

/**
 * @enum ServiceType
 * @brief Defines the available services that can be connected to via CLI
 */
enum class ServiceType {
    MCU_SIMULATOR,        ///< MCU Simulator service for temperature sensor simulation
    FAN_CONTROL_SYSTEM,   ///< Fan Control System service for fan management
    EXIT                  ///< Exit the CLI application
};

/**
 * @class CLI
 * @brief Command Line Interface for interacting with MCU Simulator and Fan Control System services
 * 
 * This class provides an interactive command-line interface that allows users to:
 * - Connect to MCU Simulator and Fan Control System gRPC services
 * - Execute commands to control and monitor temperature sensors
 * - Manage fan speeds and configurations
 * - Handle alarms and system status
 * - View help and documentation for available commands
 */
class CLI {
public:
    /**
     * @brief Default constructor for CLI
     */
    CLI();

    /**
     * @brief Destructor that ensures proper cleanup of connections
     */
    ~CLI();

    /**
     * @brief Initializes the CLI with configuration
     * @param config_path Path to the configuration file
     * @return true if initialization was successful, false otherwise
     * @note This method loads configuration settings and prepares the CLI for operation
     */
    bool initialize(const std::string& config_path);
    
    /**
     * @brief Starts the main CLI loop
     * @note This method runs the interactive command loop until the user exits
     * @note The CLI will continuously prompt for commands and process user input
     */
    void run();
    
    /**
     * @brief Stops the CLI and cleans up resources
     * @note This method disconnects from all services and stops the main loop
     */
    void stop();

private:
    /**
     * @brief Displays service selection menu and gets user choice
     * @return Selected service type from the user
     * @note This method shows available services and handles user input for selection
     */
    ServiceType selectService();

    /**
     * @brief Establishes connection to the specified service
     * @param service Type of service to connect to
     * @return true if connection was successful, false otherwise
     * @note This method creates gRPC channels and stubs for the selected service
     */
    bool connectToService(ServiceType service);

    /**
     * @brief Disconnects from the currently connected service
     * @note This method cleans up gRPC connections and resets service state
     */
    void disconnectFromService();
    
    /**
     * @brief Processes a user command string
     * @param command The command string to process
     * @note This method parses the command and routes it to the appropriate handler
     */
    void processCommand(const std::string& command);

    /**
     * @brief Processes MCU Simulator specific commands
     * @param cmd The command type
     * @param iss Input string stream containing command parameters
     * @note This method handles all MCU-related commands like temperature reading and fault simulation
     */
    void processMCUCommand(const std::string& cmd, std::istringstream& iss);

    /**
     * @brief Processes Fan Control System specific commands
     * @param cmd The command type
     * @param iss Input string stream containing command parameters
     * @note This method handles all fan-related commands like speed control and status monitoring
     */
    void processFanCommand(const std::string& cmd, std::istringstream& iss);
    
    /**
     * @brief Displays the main help menu
     * @note Shows available services and basic navigation commands
     */
    void showMainMenu();

    /**
     * @brief Displays MCU Simulator specific help
     * @note Shows all available MCU commands with usage examples
     */
    void showMCUHelp();

    /**
     * @brief Displays Fan Control System specific help
     * @note Shows all available fan commands with usage examples
     */
    void showFanHelp();

    // MCU Simulator RPC methods
    /**
     * @brief Gets current temperature from a specific sensor
     * @param mcu_name Name of the MCU to query
     * @param sensor_id ID of the temperature sensor
     * @note This method calls the MCU Simulator service to retrieve temperature readings
     */
    void getTemperature(const std::string& mcu_name, const std::string& sensor_id);

    /**
     * @brief Gets the status of MCU(s)
     * @param mcu_name Optional MCU name filter, empty string for all MCUs
     * @note This method retrieves operational status and configuration of MCU(s)
     */
    void getMCUStatus(const std::string& mcu_name = "");

    /**
     * @brief Sets simulation parameters for temperature generation
     * @param mcu_name Name of the MCU to configure
     * @param sensor_id ID of the temperature sensor
     * @param start_temp Starting temperature for simulation
     * @param end_temp Ending temperature for simulation
     * @param step_size Temperature step size for simulation
     * @note This method configures how the temperature sensor generates simulated values
     */
    void setSimulationParams(const std::string& mcu_name, const std::string& sensor_id, double start_temp, double end_temp, double step_size);

    /**
     * @brief Sets fault condition for an MCU
     * @param mcu_name Name of the MCU to set fault for
     * @param is_faulty true to enable fault condition, false to disable
     * @note This method simulates MCU hardware faults for testing purposes
     */
    void setMCUFault(const std::string& mcu_name, bool is_faulty);

    /**
     * @brief Sets fault condition for a specific sensor
     * @param mcu_name Name of the MCU containing the sensor
     * @param sensor_id ID of the temperature sensor
     * @param is_faulty true to enable fault condition, false to disable
     * @note This method simulates sensor hardware faults for testing purposes
     */
    void setSensorFault(const std::string& mcu_name, const std::string& sensor_id, bool is_faulty);

    /**
     * @brief Sets noise level for a specific sensor
     * @param mcu_name Name of the MCU containing the sensor
     * @param sensor_id ID of the temperature sensor
     * @param is_noisy true to enable noise simulation, false to disable
     * @note This method adds random noise to temperature readings for realistic simulation
     */
    void setSensorNoise(const std::string& mcu_name, const std::string& sensor_id, bool is_noisy);

    // Fan Control System RPC methods
    /**
     * @brief Gets the status of fan(s)
     * @param fan_name Optional fan name filter, empty string for all fans
     * @note This method retrieves operational status, speed, and health of fan(s)
     */
    void getFanStatus(const std::string& fan_name = "");

    /**
     * @brief Sets the speed of a specific fan
     * @param fan_name Name of the fan to control
     * @param duty_cycle Duty cycle percentage (0-100)
     * @note This method controls fan speed using PWM duty cycle
     */
    void setFanSpeed(const std::string& fan_name, int32_t duty_cycle);

    /**
     * @brief Sets the speed of all fans
     * @param duty_cycle Duty cycle percentage (0-100) for all fans
     * @note This method controls all fans simultaneously with the same speed
     */
    void setFanSpeedAll(int32_t duty_cycle);

    /**
     * @brief Sets the PWM count for a specific fan
     * @param fan_name Name of the fan to control
     * @param pwm_count Raw PWM count value
     * @note This method sets the raw PWM value directly, bypassing duty cycle calculation
     */
    void setFanPWM(const std::string& fan_name, int32_t pwm_count);

    /**
     * @brief Simulates a bad/faulty fan condition
     * @param fan_name Name of the fan to make faulty
     * @note This method simulates fan hardware faults for testing alarm systems
     */
    void makeFanBad(const std::string& fan_name);

    /**
     * @brief Restores a fan to good working condition
     * @param fan_name Name of the fan to restore
     * @note This method clears simulated fault conditions on a fan
     */
    void makeFanGood(const std::string& fan_name);

    /**
     * @brief Gets noise level information for a fan
     * @param fan_name Name of the fan to query
     * @note This method retrieves acoustic noise measurements from the fan
     */
    void getFanNoise(const std::string& fan_name);
    
    /**
     * @brief Gets temperature history from a sensor
     * @param mcu_name Name of the MCU containing the sensor
     * @param sensor_id ID of the temperature sensor
     * @param max_readings Maximum number of historical readings to retrieve
     * @note This method retrieves historical temperature data for analysis
     */
    void getTemperatureHistory(const std::string& mcu_name, const std::string& sensor_id, int32_t max_readings);

    /**
     * @brief Gets the current cooling system status
     * @note This method retrieves overall cooling system performance and status
     */
    void getCoolingStatus();

    /**
     * @brief Sets temperature thresholds for automatic fan control
     * @param temp_low Low temperature threshold
     * @param temp_high High temperature threshold
     * @param fan_speed_min Minimum fan speed percentage
     * @param fan_speed_max Maximum fan speed percentage
     * @note This method configures automatic temperature-based fan control parameters
     */
    void setTemperatureThresholds(double temp_low, double temp_high, int32_t fan_speed_min, int32_t fan_speed_max);

    /**
     * @brief Gets current temperature thresholds
     * @note This method retrieves the currently configured temperature control thresholds
     */
    void getTemperatureThresholds();
    
    /**
     * @brief Raises an alarm in the system
     * @param alarm_name Name of the alarm to raise
     * @param message Alarm message description
     * @param severity Severity level of the alarm
     * @note This method triggers alarms for testing alarm management systems
     */
    void raiseAlarm(const std::string& alarm_name, const std::string& message, const std::string& severity);

    /**
     * @brief Gets alarm history
     * @param max_entries Maximum number of alarm entries to retrieve
     * @note This method retrieves historical alarm data for analysis
     */
    void getAlarmHistory(int32_t max_entries);

    /**
     * @brief Clears alarm history
     * @param alarm_name Optional alarm name filter, empty string for all alarms
     * @note This method clears historical alarm data from the system
     */
    void clearAlarmHistory(const std::string& alarm_name = "");

    /**
     * @brief Gets alarm statistics
     * @param alarm_name Optional alarm name filter, empty string for all alarms
     * @param time_window_hours Time window in hours for statistics calculation
     * @note This method provides statistical analysis of alarm occurrences
     */
    void getAlarmStatistics(const std::string& alarm_name = "", int32_t time_window_hours = 24);

    // Service connections
    std::shared_ptr<grpc::ChannelInterface> mcu_channel_;      ///< gRPC channel for MCU Simulator service
    std::unique_ptr<mcu_simulator::MCUSimulatorService::Stub> mcu_stub_;  ///< gRPC stub for MCU Simulator
    
    std::shared_ptr<grpc::ChannelInterface> fan_channel_;      ///< gRPC channel for Fan Control System service
    std::unique_ptr<fan_control_system::FanControlSystemService::Stub> fan_stub_;  ///< gRPC stub for Fan Control System
    
    ServiceType current_service_;  ///< Currently connected service type
    bool running_{false};          ///< Flag indicating if CLI is running
};

} // namespace cli 