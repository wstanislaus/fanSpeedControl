#include "common/config.hpp"
#include <iostream>

namespace common {

/**
 * @brief Loads configuration from a YAML file
 * 
 * Attempts to load and parse the specified YAML configuration file.
 * The configuration is stored internally and can be accessed through
 * other methods.
 * 
 * @param config_file Path to the YAML configuration file
 * @return true if configuration was loaded successfully, false otherwise
 * @throws YAML::Exception if the file cannot be loaded or parsed
 */
bool Config::load(const std::string& config_file) {
    try {
        config_ = YAML::LoadFile(config_file);
        loaded_ = true;
        //read and populate all rpc server settings
        const auto& rpc_servers = config_["RPCServers"];
        for (const auto& server : rpc_servers) {
            std::string server_name = server.first.as<std::string>();
            RPCServerConfig server_config;
            server_config.port = server.second["Port"].as<uint16_t>();
            server_config.max_connections = server.second["MaxConnections"].as<uint32_t>();
            rpc_settings_.servers[server_name] = server_config;
        }
        return true;
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load config file: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief Gets the MQTT settings from the configuration
 * 
 * Extracts MQTT-related settings from the loaded configuration.
 * The configuration must be loaded before calling this method.
 * 
 * @return MQTTClient::Settings object containing the MQTT configuration
 * @throws YAML::Exception if the MQTT settings cannot be parsed
 */
MQTTClient::Settings Config::getMQTTSettings() const {
    MQTTClient::Settings settings;
    if (!loaded_) return settings;

    try {
        const auto& mqtt_config = config_["MQTTSettings"];
        settings.broker = mqtt_config["Broker"].as<std::string>();
        settings.port = mqtt_config["Port"].as<int>();
        settings.keep_alive = mqtt_config["KeepAlive"].as<int>();
        settings.qos = mqtt_config["QoS"].as<int>();
        settings.retain = mqtt_config["Retain"].as<bool>();
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to parse MQTT settings: " << e.what() << std::endl;
    }

    return settings;
}

/**
 * @brief Gets the entire configuration as a YAML node
 * 
 * Returns the complete configuration loaded from the YAML file.
 * If no configuration has been loaded, returns an empty YAML node.
 * 
 * @return YAML::Node containing the entire configuration
 */
YAML::Node Config::getConfig() const {
    if (!loaded_) return YAML::Node();
    return config_;
}

/**
 * @brief Gets the temperature simulation configuration
 * 
 * Extracts temperature simulation settings from the loaded configuration.
 * These settings control the behavior of temperature sensor simulation.
 * 
 * @return TemperatureSimConfig structure containing simulation settings
 * @throws YAML::Exception if the temperature simulation settings cannot be parsed
 */
TemperatureSimConfig Config::getTemperatureSimConfig() const {
    if (!loaded_) return TemperatureSimConfig();
    TemperatureSimConfig config;
    try {
        config.start_temp = config_["SimulatorStartTemp"].as<float>();
        config.end_temp = config_["SimulatorEndTemp"].as<float>();
        config.step_size = config_["SimulatorTempStepSize"].as<float>();
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to parse temperature simulation config: " << e.what() << std::endl;
    }
    return config;
}

const RPCServerConfig* Config::getRPCServerConfig(const std::string& server_name) const {
    auto it = rpc_settings_.servers.find(server_name);
    if (it != rpc_settings_.servers.end()) {
        return &(it->second);
    }
    return nullptr;
}

} // namespace common 