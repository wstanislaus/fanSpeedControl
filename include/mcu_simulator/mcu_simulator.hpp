#pragma once

#include <string>
#include <vector>
#include <memory>
#include <yaml-cpp/yaml.h>
#include "mcu_simulator/mcu.hpp"
#include "common/mqtt_client.hpp"
#include "common/logger.hpp"
#include "common/alarm.hpp"

namespace mcu_simulator {

// Forward declaration
class MCUSimulatorServer;

/**
 * @class MCUSimulator
 * @brief Simulates multiple MCUs (Microcontroller Units) with temperature sensors and MQTT communication
 */
class MCUSimulator {
public:
    /**
     * @brief Constructs a new MCU Simulator
     * @param config_file Path to the YAML configuration file
     */
    MCUSimulator(const std::string& config_file);

    /**
     * @brief Destructor that ensures proper cleanup by stopping all MCUs
     */
    ~MCUSimulator();

    /**
     * @brief Initializes the simulator by loading configuration and creating MCUs
     * @return true if initialization was successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Starts all MCUs in the simulator
     * @note This will start temperature monitoring and MQTT publishing for all MCUs
     */
    void start();

    /**
     * @brief Stops all MCUs in the simulator
     * @note This will stop temperature monitoring and MQTT publishing for all MCUs
     */
    void stop();

    /**
     * @brief Makes an MCU disappear from the system (for testing purposes)
     * @param name The name of the MCU to make disappear
     * @return true if the operation was successful, false otherwise
     */
    bool makeMCUDisappear(const std::string& name);

    /**
     * @brief Makes a previously disappeared MCU reappear in the system (for testing purposes)
     * @param name The name of the MCU to make reappear
     * @return true if the operation was successful, false otherwise
     */
    bool makeMCUReappear(const std::string& name);

    /**
     * @brief Gets all MCUs in the simulator
     * @return Vector of unique pointers to all MCUs
     */
    const std::vector<std::unique_ptr<MCU>>& getAllMCUs() const { return mcus_; }

private:
    /**
     * @brief Loads configuration from the YAML file
     * @return true if configuration was loaded successfully, false otherwise
     */
    bool loadConfig();

    /**
     * @brief Creates MCU instances based on the loaded configuration
     * @return true if all MCUs were created successfully, false otherwise
     */
    bool createMCUs();

    /**
     * @brief Loads temperature settings from the configuration
     * @param config The YAML configuration node
     * @return TemperatureSettings structure containing all temperature-related settings
     */
    MCU::TemperatureSettings loadTemperatureSettings(const YAML::Node& config);

    std::string config_file_;                                    ///< Path to the configuration file
    std::vector<std::unique_ptr<MCU>> mcus_;                    ///< Vector of managed MCU instances
    bool running_;                                              ///< Flag indicating if the simulator is running
    YAML::Node config_;                                         ///< Loaded configuration data

    // Common components for simulator-level logging and alarms
    std::shared_ptr<common::MQTTClient> mqtt_client_;           ///< MQTT client for simulator-level communication
    std::unique_ptr<common::Logger> logger_;                    ///< Logger for simulator-level logging
    std::unique_ptr<common::Alarm> alarm_;                      ///< Alarm system for simulator-level alerts
    std::string name_;

    // RPC server
    std::unique_ptr<MCUSimulatorServer> rpc_server_;            ///< RPC server for MCU simulator
};

} // namespace mcu_simulator 