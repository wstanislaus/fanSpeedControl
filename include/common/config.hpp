#pragma once

#include <string>
#include <yaml-cpp/yaml.h>
#include "common/mqtt_client.hpp"
#include <cstdint>
#include <unordered_map>

namespace common {

/**
 * @class Config
 * @brief Singleton class for managing application configuration
 * 
 * This class provides a centralized way to load and access configuration
 * settings from a YAML file. It uses the singleton pattern to ensure
 * only one instance exists throughout the application.
 */

/**
 * @struct TemperatureSimConfig
 * @brief Configuration structure for temperature sensor simulation parameters
 */
struct TemperatureSimConfig {
    float start_temp;    ///< Starting temperature for simulation (°C)
    float end_temp;      ///< Ending temperature for simulation (°C)
    float step_size;     ///< Temperature increment step size (°C)
};

/**
 * @struct RPCServerConfig
 * @brief Configuration structure for individual RPC server settings
 */
struct RPCServerConfig {
    uint16_t port;           ///< Port number for the RPC server
    uint32_t max_connections; ///< Maximum number of concurrent connections
};

/**
 * @struct RPCServerSettings
 * @brief Container for multiple RPC server configurations
 */
struct RPCServerSettings {
    std::unordered_map<std::string, RPCServerConfig> servers;  ///< Map of server names to their configurations
};

class Config {
public:
    /**
     * @brief Gets the singleton instance of the Config class
     * @return Reference to the Config instance
     * @note This method ensures only one configuration instance exists throughout the application
     */
    static Config& getInstance() {
        static Config instance;
        return instance;
    }

    /**
     * @brief Loads configuration from a YAML file
     * @param config_file Path to the YAML configuration file
     * @return true if configuration was loaded successfully, false otherwise
     * @note This method parses the YAML file and stores the configuration data internally
     */
    bool load(const std::string& config_file);

    /**
     * @brief Gets the MQTT settings from the configuration
     * @return MQTTClient::Settings object containing the MQTT configuration
     * @throws std::runtime_error if configuration is not loaded
     * @note This method extracts MQTT connection parameters from the loaded configuration
     */
    MQTTClient::Settings getMQTTSettings() const;

    /**
     * @brief Gets the configuration
     * @return YAML::Node object containing the configuration
     * @note This method provides direct access to the raw configuration data
     */
    YAML::Node getConfig() const;

    /**
     * @brief Gets the temperature simulation configuration
     * @return TemperatureSimConfig object containing the temperature simulation configuration
     * @note This method returns the parsed temperature simulation parameters
     */
    TemperatureSimConfig getTemperatureSimConfig() const;

    /**
     * @brief Gets all RPC server settings
     * @return Reference to RPCServerSettings containing all server configurations
     * @note This method provides access to all configured RPC servers
     */
    const RPCServerSettings& getRPCServerSettings() const { return rpc_settings_; }
    
    /**
     * @brief Gets configuration for a specific RPC server
     * @param server_name Name of the server to get configuration for
     * @return Pointer to RPCServerConfig if found, nullptr otherwise
     * @note This method looks up configuration for a specific server by name
     */
    const RPCServerConfig* getRPCServerConfig(const std::string& server_name) const;

private:
    /**
     * @brief Private constructor to enforce singleton pattern
     * @note This constructor is private to prevent direct instantiation
     */
    Config() = default;

    /**
     * @brief Private destructor
     * @note This destructor is private as part of the singleton pattern
     */
    ~Config() = default;

    /**
     * @brief Deleted copy constructor to prevent copying
     * @note This prevents creating multiple instances of the singleton
     */
    Config(const Config&) = delete;

    /**
     * @brief Deleted assignment operator to prevent assignment
     * @note This prevents modifying the singleton instance
     */
    Config& operator=(const Config&) = delete;

    YAML::Node config_;                                         ///< Loaded configuration data
    bool loaded_ = false;                                       ///< Whether configuration has been loaded
    TemperatureSimConfig sim_config_;                           ///< Temperature sensor simulation configuration
    RPCServerSettings rpc_settings_;                            ///< RPC server configuration settings
};

} // namespace common 