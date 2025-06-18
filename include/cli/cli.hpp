#pragma once

#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "common/config.hpp"
#include "mcu_simulator.grpc.pb.h"

namespace cli {

class CLI {
public:
    CLI();
    ~CLI();

    // Initialize the CLI
    bool initialize(const std::string& config_path);
    
    // Run the CLI
    void run();
    
    // Stop the CLI
    void stop();

private:
    // Create gRPC channel
    bool createChannel();
    
    // Process user commands
    void processCommand(const std::string& command);
    
    // Display help
    void showHelp();

    // RPC methods
    void getTemperature(const std::string& mcu_name, const std::string& sensor_id);
    void getMCUStatus(const std::string& mcu_name = "");
    void setSimulationParams(double start_temp, double end_temp, double step_size);
    void setMCUFault(const std::string& mcu_name, bool is_faulty);
    void setSensorFault(const std::string& mcu_name, const std::string& sensor_id, bool is_faulty);
    void setSensorNoise(const std::string& mcu_name, const std::string& sensor_id, bool is_noisy);

    std::shared_ptr<grpc::ChannelInterface> channel_;
    std::unique_ptr<mcu_simulator::MCUSimulatorService::Stub> stub_;
    bool running_{false};
};

} // namespace cli 