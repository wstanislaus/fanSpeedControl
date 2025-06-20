#include "common/mqtt_client.hpp"
#include <iostream>

namespace common {

/**
 * @brief Static callback function that forwards MQTT messages to instance callbacks
 * 
 * This function is registered with the mosquitto library and forwards received
 * MQTT messages to the appropriate instance callback function. It extracts the
 * MQTTClient instance from the user data and calls the registered message callback.
 * 
 * @param mosq Pointer to the mosquitto instance
 * @param obj User data pointer (contains the MQTTClient instance)
 * @param msg Pointer to the received MQTT message
 */
void on_message(mosquitto* mosq, void* obj, const mosquitto_message* msg) {
    auto* client = static_cast<MQTTClient*>(obj);
    if (client && client->message_callback_) {
        client->message_callback_(mosq, client->user_data_, msg);
    }
}

/**
 * @brief Constructs a new MQTT client
 * 
 * Initializes the MQTT client with the given client ID and settings.
 * The client is not connected at this point and needs to be initialized
 * and connected separately.
 * 
 * @param client_id Unique identifier for this MQTT client
 * @param settings MQTT connection and publishing settings
 */
MQTTClient::MQTTClient(const std::string& client_id, const Settings& settings)
    : client_id_(client_id)
    , settings_(settings)
    , client_(nullptr)
    , initialized_(false)
    , user_data_(nullptr)
{
}

/**
 * @brief Destructor that ensures proper cleanup of MQTT resources
 * 
 * Disconnects from the MQTT broker if connected and cleans up
 * the mosquitto client instance.
 */
MQTTClient::~MQTTClient() {
    disconnect();
    if (client_) {
        mosquitto_destroy(client_);
    }
}

/**
 * @brief Initializes the MQTT client
 * 
 * Initializes the mosquitto library and creates a new mosquitto client
 * instance. This must be called before attempting to connect to the broker.
 * 
 * @return true if initialization was successful, false otherwise
 */
bool MQTTClient::initialize() {
    if (initialized_) return true;

    mosquitto_lib_init();
    client_ = mosquitto_new(client_id_.c_str(), true, this);
    if (!client_) {
        std::cerr << "Failed to create MQTT client: " << client_id_ << std::endl;
        return false;
    }

    // Set the message callback
    mosquitto_message_callback_set(client_, on_message);

    initialized_ = true;
    return true;
}

/**
 * @brief Connects to the MQTT broker
 * 
 * Establishes a connection to the MQTT broker using the configured settings.
 * If the client is not initialized, it will be initialized first.
 * 
 * @return true if connection was successful, false otherwise
 */
bool MQTTClient::connect() {
    if (!initialized_ && !initialize()) {
        return false;
    }

    int rc = mosquitto_connect(client_,
                             settings_.broker.c_str(),
                             settings_.port,
                             settings_.keep_alive);
    if (rc != MOSQ_ERR_SUCCESS) {
        std::cerr << "Failed to connect to MQTT broker: " << client_id_ << std::endl;
        return false;
    }

    // Start the network loop
    rc = mosquitto_loop_start(client_);
    if (rc != MOSQ_ERR_SUCCESS) {
        std::cerr << "Failed to start MQTT network loop: " << client_id_ << std::endl;
        return false;
    }

    return true;
}

/**
 * @brief Publishes a message to an MQTT topic
 * 
 * Publishes the given payload to the specified topic using the configured
 * QoS and retain settings.
 * 
 * @param topic The MQTT topic to publish to
 * @param payload The message payload to publish
 * @return true if publishing was successful, false otherwise
 */
bool MQTTClient::publish(const std::string& topic, const std::string& payload) {
    if (!client_) return false;

    int rc = mosquitto_publish(client_,
                             nullptr,
                             topic.c_str(),
                             payload.length(),
                             payload.c_str(),
                             settings_.qos,
                             settings_.retain);
    return rc == MOSQ_ERR_SUCCESS;
}

/**
 * @brief Subscribes to an MQTT topic
 * 
 * Subscribes to the specified topic with the given QoS level.
 * Messages received on this topic will be delivered to the message callback.
 * 
 * @param topic The MQTT topic to subscribe to
 * @param qos The Quality of Service level (0, 1, or 2)
 * @return true if subscription was successful, false otherwise
 */
bool MQTTClient::subscribe(const std::string& topic, int qos) {
    if (!client_) return false;
    return mosquitto_subscribe(client_, nullptr, topic.c_str(), qos) == MOSQ_ERR_SUCCESS;
}

/**
 * @brief Disconnects from the MQTT broker
 * 
 * Gracefully disconnects from the MQTT broker if connected.
 * This is called automatically by the destructor.
 */
void MQTTClient::disconnect() {
    if (client_) {
        mosquitto_disconnect(client_);
    }
}

/**
 * @brief Sets the message callback function
 * 
 * Sets the callback function that will be called when a message is received
 * on any subscribed topic. The callback is called with the mosquitto instance,
 * user data, and the received message.
 * 
 * @param callback The callback function to be called when a message is received
 * @param user_data User data to be passed to the callback function
 */
void MQTTClient::set_message_callback(MessageCallback callback, void* user_data) {
    message_callback_ = callback;
    user_data_ = user_data;
}

} // namespace common 