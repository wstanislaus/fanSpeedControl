#pragma once

#include "common/rpc_server.hpp"
#include "mcu_simulator/mcu_simulator.hpp"
#include "mcu_simulator.grpc.pb.h"
#include <memory>
#include <unordered_map>
#include <mutex>

namespace mcu_simulator {

class MCUSimulatorServiceImpl final : public MCUSimulatorService::Service {
public:
    explicit MCUSimulatorServiceImpl(MCUSimulator& simulator);

    // Service implementations
    grpc::Status GetTemperature(grpc::ServerContext* context, 
                              const TemperatureRequest* request,
                              TemperatureResponse* response) override;

    grpc::Status GetMCUStatus(grpc::ServerContext* context,
                            const StatusRequest* request,
                            StatusResponse* response) override;

    grpc::Status SetSimulationParams(grpc::ServerContext* context,
                                   const SimulationParams* request,
                                   SimulationResponse* response) override;

    grpc::Status SetMCUFault(grpc::ServerContext* context,
                            const MCUFaultRequest* request,
                            FaultResponse* response) override;

    grpc::Status SetSensorFault(grpc::ServerContext* context,
                               const SensorFaultRequest* request,
                               FaultResponse* response) override;

    grpc::Status SetSensorNoise(grpc::ServerContext* context,
                               const SensorNoiseRequest* request,
                               FaultResponse* response) override;

private:
    MCUSimulator& simulator_;
};

class MCUSimulatorServer : public common::RPCServer {
public:
    /**
     * @brief Constructs a new MCU Simulator Server
     * @param simulator Reference to the MCU simulator instance
     */
    explicit MCUSimulatorServer(MCUSimulator& simulator);
    ~MCUSimulatorServer() override = default;

protected:
    // Implement the virtual method from RPCServer
    void addServices(grpc::ServerBuilder& builder) override;

private:
    MCUSimulator& simulator_;
    std::unique_ptr<MCUSimulatorServiceImpl> service_;
};

} // namespace mcu_simulator 