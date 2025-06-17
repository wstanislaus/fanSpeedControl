#pragma once

#include <string>
#include <yaml-cpp/yaml.h>
#include "common/mqtt_client.hpp"

namespace common {

/**
 * @class Config
 * @brief Singleton class for managing application configuration
 * 
 * This class provides a centralized way to load and access configuration
 * settings from a YAML file. It uses the singleton pattern to ensure
 * only one instance exists throughout the application.
 */
// Temperature simulation configuration
struct TemperatureSimConfig {
    float start_temp;
    float end_temp;
    float step_size;
};

class Config {
public:
    /**
     * @brief Gets the singleton instance of the Config class
     * @return Reference to the Config instance
     */
    static Config& getInstance() {
        static Config instance;
        return instance;
    }

    /**
     * @brief Loads configuration from a YAML file
     * @param config_file Path to the YAML configuration file
     * @return true if configuration was loaded successfully, false otherwise
     */
    bool load(const std::string& config_file);

    /**
     * @brief Gets the MQTT settings from the configuration
     * @return MQTTClient::Settings object containing the MQTT configuration
     * @throws std::runtime_error if configuration is not loaded
     */
    MQTTClient::Settings getMQTTSettings() const;

    /**
     * @brief Gets the configuration
     * @return YAML::Node object containing the configuration
     */
    YAML::Node getConfig() const;

    /**
     * @brief Gets the temperature simulation configuration
     * @return TemperatureSimConfig object containing the temperature simulation configuration
     */
    TemperatureSimConfig getTemperatureSimConfig() const;

private:
    /**
     * @brief Private constructor to enforce singleton pattern
     */
    Config() = default;

    /**
     * @brief Private destructor
     */
    ~Config() = default;

    /**
     * @brief Deleted copy constructor to prevent copying
     */
    Config(const Config&) = delete;

    /**
     * @brief Deleted assignment operator to prevent assignment
     */
    Config& operator=(const Config&) = delete;

    YAML::Node config_;                                         ///< Loaded configuration data
    bool loaded_ = false;                                       ///< Whether configuration has been loaded
    TemperatureSimConfig sim_config_;                           ///< Temperature sensor simulation configuration
};

} // namespace common 