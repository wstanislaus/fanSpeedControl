#pragma once

#include "common/rpc_server.hpp"
#include "mcu_simulator/mcu_simulator.hpp"
#include "mcu_simulator.grpc.pb.h"
#include <memory>
#include <unordered_map>
#include <mutex>

namespace mcu_simulator {

/**
 * @class MCUSimulatorServiceImpl
 * @brief Implementation of the MCU Simulator gRPC service
 * 
 * This class implements all the gRPC service methods defined in the protobuf
 * interface. It provides remote access to MCU and temperature sensor control
 * and monitoring functionality.
 */
class MCUSimulatorServiceImpl final : public MCUSimulatorService::Service {
public:
    /**
     * @brief Constructs a new MCU Simulator Service implementation
     * @param simulator Reference to the MCU simulator instance
     * @note This constructor establishes the connection between the service and the simulator
     */
    explicit MCUSimulatorServiceImpl(MCUSimulator& simulator);

    // Service implementations
    /**
     * @brief Gets current temperature from a specific sensor
     * @param context gRPC server context
     * @param request Request containing MCU name and sensor ID
     * @param response Response containing temperature reading and status
     * @return gRPC status indicating success or failure
     * @note This method retrieves the most recent temperature reading from the specified sensor
     */
    grpc::Status GetTemperature(grpc::ServerContext* context, 
                              const TemperatureRequest* request,
                              TemperatureResponse* response) override;

    /**
     * @brief Gets the status of MCU(s)
     * @param context gRPC server context
     * @param request Request containing optional MCU name filter
     * @param response Response containing MCU status information
     * @return gRPC status indicating success or failure
     * @note This method retrieves operational status and configuration of MCU(s)
     */
    grpc::Status GetMCUStatus(grpc::ServerContext* context,
                            const StatusRequest* request,
                            StatusResponse* response) override;

    /**
     * @brief Sets simulation parameters for temperature generation
     * @param context gRPC server context
     * @param request Request containing MCU name, sensor ID, and simulation parameters
     * @param response Response containing operation result
     * @return gRPC status indicating success or failure
     * @note This method configures how the temperature sensor generates simulated values
     */
    grpc::Status SetSimulationParams(grpc::ServerContext* context,
                                   const SimulationParams* request,
                                   SimulationResponse* response) override;

    /**
     * @brief Sets fault condition for an MCU
     * @param context gRPC server context
     * @param request Request containing MCU name and fault state
     * @param response Response containing operation result
     * @return gRPC status indicating success or failure
     * @note This method simulates MCU hardware faults for testing purposes
     */
    grpc::Status SetMCUFault(grpc::ServerContext* context,
                            const MCUFaultRequest* request,
                            FaultResponse* response) override;

    /**
     * @brief Sets fault condition for a specific sensor
     * @param context gRPC server context
     * @param request Request containing MCU name, sensor ID, and fault state
     * @param response Response containing operation result
     * @return gRPC status indicating success or failure
     * @note This method simulates sensor hardware faults for testing purposes
     */
    grpc::Status SetSensorFault(grpc::ServerContext* context,
                               const SensorFaultRequest* request,
                               FaultResponse* response) override;

    /**
     * @brief Sets noise level for a specific sensor
     * @param context gRPC server context
     * @param request Request containing MCU name, sensor ID, and noise state
     * @param response Response containing operation result
     * @return gRPC status indicating success or failure
     * @note This method adds random noise to temperature readings for realistic simulation
     */
    grpc::Status SetSensorNoise(grpc::ServerContext* context,
                               const SensorNoiseRequest* request,
                               FaultResponse* response) override;

private:
    MCUSimulator& simulator_;  ///< Reference to the MCU simulator instance
};

/**
 * @class MCUSimulatorServer
 * @brief gRPC server for the MCU Simulator
 * 
 * This class extends the base RPCServer to provide gRPC functionality
 * specifically for the MCU Simulator, managing the service implementation
 * and server lifecycle.
 */
class MCUSimulatorServer : public common::RPCServer {
public:
    /**
     * @brief Constructs a new MCU Simulator Server
     * @param simulator Reference to the MCU simulator instance
     * @note This constructor initializes the server with the simulator reference
     */
    explicit MCUSimulatorServer(MCUSimulator& simulator);

    /**
     * @brief Destructor for proper cleanup
     * @note This destructor ensures proper cleanup of server resources
     */
    ~MCUSimulatorServer() override = default;

protected:
    /**
     * @brief Adds the MCU Simulator service to the gRPC server
     * @param builder gRPC ServerBuilder instance for registering services
     * @note This method registers the MCUSimulatorServiceImpl with the server
     */
    void addServices(grpc::ServerBuilder& builder) override;

private:
    MCUSimulator& simulator_;                                    ///< Reference to the MCU simulator instance
    std::unique_ptr<MCUSimulatorServiceImpl> service_;           ///< Service implementation instance
};

} // namespace mcu_simulator 