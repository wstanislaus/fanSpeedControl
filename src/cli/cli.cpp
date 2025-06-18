#include "cli/cli.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace cli {

CLI::CLI() = default;

CLI::~CLI() {
    stop();
}

bool CLI::initialize(const std::string& config_path) {
    // Load configuration
    if (!common::Config::getInstance().load(config_path)) {
        std::cerr << "Failed to load configuration from " << config_path << std::endl;
        return false;
    }

    // Create gRPC channel
    if (!createChannel()) {
        std::cerr << "Failed to create gRPC channel" << std::endl;
        return false;
    }

    return true;
}

bool CLI::createChannel() {
    const auto* server_config = common::Config::getInstance().getRPCServerConfig("MCUSimulator");
    if (!server_config) {
        std::cerr << "MCUSimulator RPC server configuration not found" << std::endl;
        return false;
    }

    std::string target = "localhost:" + std::to_string(server_config->port);
    channel_ = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
    if (!channel_) {
        return false;
    }

    stub_ = mcu_simulator::MCUSimulatorService::NewStub(channel_);
    return stub_ != nullptr;
}

void CLI::run() {
    if (running_) {
        return;
    }

    running_ = true;
    std::string command;

    std::cout << "Fan Control System CLI" << std::endl;
    std::cout << "Type 'help' for available commands" << std::endl;

    while (running_) {
        std::cout << "> ";
        std::getline(std::cin, command);
        
        if (command.empty()) {
            continue;
        }

        processCommand(command);
    }
}

void CLI::processCommand(const std::string& command) {
    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;

    if (cmd == "help") {
        showHelp();
    }
    else if (cmd == "exit" || cmd == "quit") {
        stop();
    }
    else if (cmd == "get_temp") {
        std::string mcu_name, sensor_id;
        if (iss >> mcu_name >> sensor_id) {
            getTemperature(mcu_name, sensor_id);
        } else {
            std::cout << "Usage: get_temp <mcu_name> <sensor_id>" << std::endl;
        }
    }
    else if (cmd == "get_mcu_status") {
        std::string mcu_name;
        if (iss >> mcu_name) {
            getMCUStatus(mcu_name);
        } else {
            getMCUStatus();  // Get status of all MCUs
        }
    }
    else if (cmd == "set_sim_params") {
        double start_temp, end_temp, step_size;
        if (iss >> start_temp >> end_temp >> step_size) {
            setSimulationParams(start_temp, end_temp, step_size);
        } else {
            std::cout << "Usage: set_sim_params <start_temp> <end_temp> <step_size>" << std::endl;
        }
    }
    else if (cmd == "set_mcu_fault") {
        std::string mcu_name;
        bool is_faulty;
        if (iss >> mcu_name >> is_faulty) {
            setMCUFault(mcu_name, is_faulty);
        } else {
            std::cout << "Usage: set_mcu_fault <mcu_name> <is_faulty>" << std::endl;
        }
    }
    else if (cmd == "set_sensor_fault") {
        std::string mcu_name, sensor_id;
        bool is_faulty;
        if (iss >> mcu_name >> sensor_id >> is_faulty) {
            setSensorFault(mcu_name, sensor_id, is_faulty);
        } else {
            std::cout << "Usage: set_sensor_fault <mcu_name> <sensor_id> <is_faulty>" << std::endl;
        }
    }
    else if (cmd == "set_sensor_noise") {
        std::string mcu_name, sensor_id;
        bool is_noisy;
        if (iss >> mcu_name >> sensor_id >> is_noisy) {
            setSensorNoise(mcu_name, sensor_id, is_noisy);
        } else {
            std::cout << "Usage: set_sensor_noise <mcu_name> <sensor_id> <is_noisy>" << std::endl;
        }
    }
    else {
        std::cout << "Unknown command. Type 'help' for available commands." << std::endl;
    }
}

void CLI::showHelp() {
    std::cout << "Available commands:" << std::endl;
    std::cout << "  help  - Show this help message" << std::endl;
    std::cout << "  exit  - Exit the application" << std::endl;
    std::cout << "  quit  - Exit the application" << std::endl;
    std::cout << "  get_temp <mcu_name> <sensor_id>  - Get temperature from a specific sensor" << std::endl;
    std::cout << "  get_mcu_status [mcu_name]  - Get status of all MCUs or a specific MCU" << std::endl;
    std::cout << "  set_sim_params <start_temp> <end_temp> <step_size>  - Set simulation parameters" << std::endl;
    std::cout << "  set_mcu_fault <mcu_name> <is_faulty>  - Set MCU fault state (0=normal, 1=faulty)" << std::endl;
    std::cout << "  set_sensor_fault <mcu_name> <sensor_id> <is_faulty>  - Set sensor fault state (0=normal, 1=faulty)" << std::endl;
    std::cout << "  set_sensor_noise <mcu_name> <sensor_id> <is_noisy>  - Set sensor noise state (0=normal, 1=noisy)" << std::endl;
}

void CLI::stop() {
    running_ = false;
    channel_.reset();
    stub_.reset();
}

void CLI::getTemperature(const std::string& mcu_name, const std::string& sensor_id) {
    mcu_simulator::TemperatureRequest request;
    request.set_mcu_name(mcu_name);
    request.set_sensor_id(sensor_id);

    mcu_simulator::TemperatureResponse response;
    grpc::ClientContext context;

    grpc::Status status = stub_->GetTemperature(&context, request, &response);
    if (status.ok()) {
        if (response.is_valid()) {
            std::cout << "Temperature: " << response.temperature() << "°C" << std::endl;
        } else {
            std::cout << "Error: " << response.error_message() << std::endl;
        }
    } else {
        std::cout << "RPC failed: " << status.error_message() << std::endl;
    }
}

void CLI::getMCUStatus(const std::string& mcu_name) {
    mcu_simulator::StatusRequest request;
    request.set_mcu_name(mcu_name);

    mcu_simulator::StatusResponse response;
    grpc::ClientContext context;

    grpc::Status status = stub_->GetMCUStatus(&context, request, &response);
    if (status.ok()) {
        try {
            if (response.mcu_status_size() == 0) {
                std::cout << "No MCU status information available" << std::endl;
                return;
            }

            for (const auto& mcu_status : response.mcu_status()) {
                std::cout << "MCU: " << mcu_status.mcu_name() << std::endl;
                std::cout << "  Online: " << (mcu_status.is_online() ? "Yes" : "No") << std::endl;
                std::cout << "  Active Sensors: " << mcu_status.active_sensors() << std::endl;
                std::cout << "  Sensors:" << std::endl;
                for (const auto& sensor : mcu_status.sensors()) {
                    std::cout << "    ID: " << sensor.sensor_id() << std::endl;
                    std::cout << "    Active: " << (sensor.is_active() ? "Yes" : "No") << std::endl;
                    std::cout << "    Interface: " << sensor.interface() << std::endl;
                    std::cout << "    Address: " << sensor.address() << std::endl;
                    std::cout << "    Noisy: " << (sensor.is_noisy() ? "Yes" : "No") << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cout << "Error processing MCU status: " << e.what() << std::endl;
        }
    } else {
        std::cout << "RPC failed: " << status.error_message() << std::endl;
        std::cout << "Error code: " << status.error_code() << std::endl;
    }
}

void CLI::setSimulationParams(double start_temp, double end_temp, double step_size) {
    mcu_simulator::SimulationParams request;
    request.set_start_temp(start_temp);
    request.set_end_temp(end_temp);
    request.set_step_size(step_size);

    mcu_simulator::SimulationResponse response;
    grpc::ClientContext context;

    grpc::Status status = stub_->SetSimulationParams(&context, request, &response);
    if (status.ok()) {
        std::cout << response.message() << std::endl;
    } else {
        std::cout << "RPC failed: " << status.error_message() << std::endl;
    }
}

void CLI::setMCUFault(const std::string& mcu_name, bool is_faulty) {
    mcu_simulator::MCUFaultRequest request;
    request.set_mcu_name(mcu_name);
    request.set_is_faulty(is_faulty);

    mcu_simulator::FaultResponse response;
    grpc::ClientContext context;

    grpc::Status status = stub_->SetMCUFault(&context, request, &response);
    if (status.ok()) {
        std::cout << response.message() << std::endl;
        std::cout << "Current state: " << response.current_state() << std::endl;
    } else {
        std::cout << "RPC failed: " << status.error_message() << std::endl;
    }
}

void CLI::setSensorFault(const std::string& mcu_name, const std::string& sensor_id, bool is_faulty) {
    mcu_simulator::SensorFaultRequest request;
    request.set_mcu_name(mcu_name);
    request.set_sensor_id(sensor_id);
    request.set_is_faulty(is_faulty);

    mcu_simulator::FaultResponse response;
    grpc::ClientContext context;

    grpc::Status status = stub_->SetSensorFault(&context, request, &response);
    if (status.ok()) {
        std::cout << response.message() << std::endl;
        std::cout << "Current state: " << response.current_state() << std::endl;
    } else {
        std::cout << "RPC failed: " << status.error_message() << std::endl;
    }
}

void CLI::setSensorNoise(const std::string& mcu_name, const std::string& sensor_id, bool is_noisy) {
    mcu_simulator::SensorNoiseRequest request;
    request.set_mcu_name(mcu_name);
    request.set_sensor_id(sensor_id);
    request.set_is_noisy(is_noisy);

    mcu_simulator::FaultResponse response;
    grpc::ClientContext context;

    grpc::Status status = stub_->SetSensorNoise(&context, request, &response);
    if (status.ok()) {
        std::cout << response.message() << std::endl;
        std::cout << "Current state: " << response.current_state() << std::endl;
    } else {
        std::cout << "RPC failed: " << status.error_message() << std::endl;
    }
}

} // namespace cli 