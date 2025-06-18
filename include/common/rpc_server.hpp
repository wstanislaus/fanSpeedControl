#pragma once

#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>
#include <thread>
#include <atomic>

namespace common {

class RPCServer {
public:
    RPCServer(const std::string& server_name, uint16_t port, uint32_t max_connections);
    virtual ~RPCServer();

    // Start the server
    bool start();
    
    // Stop the server
    void stop();
    
    // Check if server is running
    bool isRunning() const { return running_; }

protected:
    // Add service to the server - to be implemented by derived classes
    virtual void addServices(grpc::ServerBuilder& builder) = 0;

private:
    std::string server_name_;
    uint16_t port_;
    uint32_t max_connections_;
    std::unique_ptr<grpc::Server> server_;
    std::thread server_thread_;
    std::atomic<bool> running_{false};
};

} // namespace common 