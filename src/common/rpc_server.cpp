#include "common/rpc_server.hpp"
#include <iostream>

namespace common {

/**
 * @brief Constructs a new RPC server instance
 * 
 * Initializes the RPC server with the specified name, port, and maximum
 * connection limit. The server is not started until the start() method is called.
 * 
 * @param server_name Name identifier for the RPC server
 * @param port Port number on which the server will listen
 * @param max_connections Maximum number of concurrent connections allowed
 */
RPCServer::RPCServer(const std::string& server_name, uint16_t port, uint32_t max_connections)
    : server_name_(server_name)
    , port_(port)
    , max_connections_(max_connections) {
}

/**
 * @brief Destructor that ensures proper cleanup
 * 
 * Stops the RPC server and cleans up resources when the instance is destroyed.
 */
RPCServer::~RPCServer() {
    stop();
}

/**
 * @brief Starts the RPC server
 * 
 * Initializes and starts the gRPC server on the configured port. The server
 * runs in a separate thread and listens for incoming RPC requests. Sets up
 * message size limits and adds the configured services.
 * 
 * @return true if the server started successfully, false otherwise
 */
bool RPCServer::start() {
    if (running_) {
        return true;
    }

    std::string server_address = "0.0.0.0:" + std::to_string(port_);
    grpc::ServerBuilder builder;
    
    // Add listening port
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    
    // Set max connections
    builder.SetMaxReceiveMessageSize(1024 * 1024); // 1MB
    builder.SetMaxSendMessageSize(1024 * 1024);   // 1MB
    
    // Add services
    addServices(builder);
    
    // Build and start server
    server_ = builder.BuildAndStart();
    if (!server_) {
        std::cerr << "Failed to start " << server_name_ << " server on " << server_address << std::endl;
        return false;
    }

    running_ = true;
    std::cout << server_name_ << " server listening on " << server_address << std::endl;
    
    // Start server in a separate thread
    server_thread_ = std::thread([this]() {
        server_->Wait();
    });

    return true;
}

/**
 * @brief Stops the RPC server
 * 
 * Gracefully shuts down the gRPC server, waits for the server thread to complete,
 * and cleans up resources. This method is safe to call multiple times.
 */
void RPCServer::stop() {
    if (!running_) {
        return;
    }

    running_ = false;
    if (server_) {
        server_->Shutdown();
    }
    
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
    
    server_.reset();
}

} // namespace common 