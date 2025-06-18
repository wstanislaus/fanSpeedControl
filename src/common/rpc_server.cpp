#include "common/rpc_server.hpp"
#include <iostream>

namespace common {

RPCServer::RPCServer(const std::string& server_name, uint16_t port, uint32_t max_connections)
    : server_name_(server_name)
    , port_(port)
    , max_connections_(max_connections) {
}

RPCServer::~RPCServer() {
    stop();
}

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