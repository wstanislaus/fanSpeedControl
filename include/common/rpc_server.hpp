#pragma once

#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>
#include <thread>
#include <atomic>

namespace common {

/**
 * @class RPCServer
 * @brief Base class for gRPC server implementations
 * 
 * This class provides a common interface for gRPC servers with basic
 * server management functionality including starting, stopping, and
 * connection management. Derived classes must implement the addServices
 * method to register their specific gRPC services.
 */
class RPCServer {
public:
    /**
     * @brief Constructs a new RPC server instance
     * @param server_name Name identifier for the server
     * @param port Port number for the server to listen on
     * @param max_connections Maximum number of concurrent connections allowed
     * @note This constructor initializes server parameters but does not start the server
     */
    RPCServer(const std::string& server_name, uint16_t port, uint32_t max_connections);

    /**
     * @brief Virtual destructor for proper cleanup
     * @note This destructor ensures the server is stopped and resources are cleaned up
     */
    virtual ~RPCServer();

    /**
     * @brief Starts the gRPC server
     * @return true if the server started successfully, false otherwise
     * @note This method creates the server, adds services, and starts listening for connections
     */
    bool start();
    
    /**
     * @brief Stops the gRPC server
     * @note This method gracefully shuts down the server and waits for all connections to close
     */
    void stop();
    
    /**
     * @brief Checks if the server is currently running
     * @return true if the server is running, false otherwise
     * @note This method provides thread-safe access to the server running state
     */
    bool isRunning() const { return running_; }

protected:
    /**
     * @brief Pure virtual method to add gRPC services to the server
     * @param builder gRPC ServerBuilder instance for registering services
     * @note Derived classes must implement this method to register their specific services
     */
    virtual void addServices(grpc::ServerBuilder& builder) = 0;

private:
    std::string server_name_;      ///< Name identifier for the server
    uint16_t port_;                ///< Port number for the server to listen on
    uint32_t max_connections_;     ///< Maximum number of concurrent connections
    std::unique_ptr<grpc::Server> server_;  ///< gRPC server instance
    std::thread server_thread_;    ///< Thread running the server
    std::atomic<bool> running_{false};  ///< Atomic flag indicating if server is running
};

} // namespace common 