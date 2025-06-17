#pragma once

#include <string>
#include <memory>
#include <mosquitto.h>
#include <functional>

namespace common {

// Forward declaration of the static callback function
static void on_message(mosquitto* mosq, void* obj, const mosquitto_message* msg);

/**
 * @class MQTTClient
 * @brief Provides MQTT client functionality for publishing messages
 * 
 * This class wraps the mosquitto MQTT client library to provide a simple
 * interface for connecting to an MQTT broker and publishing messages.
 */
class MQTTClient {
    friend void ::common::on_message(mosquitto* mosq, void* obj, const mosquitto_message* msg);

public:
    /**
     * @struct Settings
     * @brief Configuration settings for the MQTT client
     */
    struct Settings {
        std::string broker;      ///< MQTT broker address
        int port;               ///< MQTT broker port
        int keep_alive;         ///< Keep-alive interval in seconds
        int qos;                ///< Quality of Service level (0, 1, or 2)
        bool retain;            ///< Whether to retain messages
    };

    /**
     * @brief Message callback function type
     * @param mosq Pointer to the mosquitto instance
     * @param obj User data pointer
     * @param msg Pointer to the received message
     */
    using MessageCallback = std::function<void(mosquitto*, void*, const mosquitto_message*)>;

    /**
     * @brief Constructs a new MQTT client
     * @param client_id Unique identifier for this MQTT client
     * @param settings MQTT connection and publishing settings
     */
    MQTTClient(const std::string& client_id, const Settings& settings);

    /**
     * @brief Destructor that ensures proper cleanup of MQTT resources
     */
    ~MQTTClient();

    /**
     * @brief Initializes the MQTT client
     * @return true if initialization was successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Connects to the MQTT broker
     * @return true if connection was successful, false otherwise
     */
    bool connect();

    /**
     * @brief Starts the MQTT network loop
     * @return true if the loop was started successfully, false otherwise
     */
    bool start_loop();

    /**
     * @brief Publishes a message to an MQTT topic
     * @param topic The MQTT topic to publish to
     * @param payload The message payload to publish
     * @return true if publishing was successful, false otherwise
     */
    bool publish(const std::string& topic, const std::string& payload);

    /**
     * @brief Subscribes to an MQTT topic
     * @param topic The MQTT topic to subscribe to
     * @param qos The Quality of Service level (0, 1, or 2)
     * @return true if subscribing was successful, false otherwise
     */
    bool subscribe(const std::string& topic, int qos);

    /**
     * @brief Sets the message callback function
     * @param callback The callback function to be called when a message is received
     * @param user_data User data to be passed to the callback function
     */
    void set_message_callback(MessageCallback callback, void* user_data);

    /**
     * @brief Disconnects from the MQTT broker
     * @note This is called automatically by the destructor
     */
    void disconnect();

private:
    std::string client_id_;                                     ///< Unique identifier for this MQTT client
    Settings settings_;                                         ///< MQTT client settings
    mosquitto* client_;                                         ///< Pointer to mosquitto client instance
    bool initialized_;                                          ///< Whether the client has been initialized
    MessageCallback message_callback_;                          ///< Message callback function
    void* user_data_;                                          ///< User data for the callback
};

} // namespace common 