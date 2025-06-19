#include "cli/cli.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

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

    return true;
}

ServiceType CLI::selectService() {
    while (true) {
        showMainMenu();
        
        std::string choice;
        std::cout << "Please select a service (1-3): ";
        std::getline(std::cin, choice);
        
        if (choice == "1") {
            return ServiceType::MCU_SIMULATOR;
        } else if (choice == "2") {
            return ServiceType::FAN_CONTROL_SYSTEM;
        } else if (choice == "3" || choice == "quit") {
            return ServiceType::EXIT;
        } else {
            std::cout << "Invalid choice. Please enter 1, 2, 3, or 'quit'." << std::endl;
        }
    }
}

bool CLI::connectToService(ServiceType service) {
    disconnectFromService();
    
    const auto* server_config = common::Config::getInstance().getRPCServerConfig(
        service == ServiceType::MCU_SIMULATOR ? "MCUSimulator" : "FanControlSystem");
    
    if (!server_config) {
        std::cerr << "RPC server configuration not found" << std::endl;
        return false;
    }

    std::string target = "localhost:" + std::to_string(server_config->port);
    
    if (service == ServiceType::MCU_SIMULATOR) {
        mcu_channel_ = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
        if (!mcu_channel_) {
            return false;
        }
        mcu_stub_ = mcu_simulator::MCUSimulatorService::NewStub(mcu_channel_);
        if (!mcu_stub_) {
            return false;
        }
        std::cout << "Connected to MCU Simulator service (localhost:" << server_config->port << ")" << std::endl;
    } else {
        fan_channel_ = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
        if (!fan_channel_) {
            return false;
        }
        fan_stub_ = fan_control_system::FanControlSystemService::NewStub(fan_channel_);
        if (!fan_stub_) {
            return false;
        }
        std::cout << "Connected to Fan Control System service (localhost:" << server_config->port << ")" << std::endl;
    }
    
    current_service_ = service;
    return true;
}

void CLI::disconnectFromService() {
    if (current_service_ == ServiceType::MCU_SIMULATOR) {
        mcu_channel_.reset();
        mcu_stub_.reset();
        std::cout << "Disconnected from MCU Simulator service" << std::endl;
    } else if (current_service_ == ServiceType::FAN_CONTROL_SYSTEM) {
        fan_channel_.reset();
        fan_stub_.reset();
        std::cout << "Disconnected from Fan Control System service" << std::endl;
    }
}

void CLI::run() {
    if (running_) {
        return;
    }

    running_ = true;
    std::cout << "Fan Control System Debug CLI" << std::endl;
    std::cout << "============================" << std::endl;

    while (running_) {
        ServiceType service = selectService();
        
        if (!running_) {
            break;
        }
        
        // Check if user wants to exit
        if (service == ServiceType::EXIT) {
            running_ = false;
            break;
        }
        
        if (!connectToService(service)) {
            std::cerr << "Failed to connect to service" << std::endl;
            continue;
        }
        
        std::string command;
        std::string prompt = (service == ServiceType::MCU_SIMULATOR) ? "mcu> " : "fan> ";
        
        std::cout << "Type 'help' for available commands, 'exit' to return to main menu" << std::endl;
        
        while (running_) {
            std::cout << prompt;
            std::getline(std::cin, command);
            
            if (command.empty()) {
                continue;
            }

            if (command == "exit") {
                break;
            } else if (command == "quit") {
                running_ = false;
                break;
            }

            processCommand(command);
        }
        
        disconnectFromService();
    }
    
    std::cout << "Goodbye!" << std::endl;
}

void CLI::processCommand(const std::string& command) {
    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;

    if (cmd == "help") {
        if (current_service_ == ServiceType::MCU_SIMULATOR) {
            showMCUHelp();
        } else {
            showFanHelp();
        }
        return;
    }

    if (current_service_ == ServiceType::MCU_SIMULATOR) {
        processMCUCommand(cmd, iss);
    } else {
        processFanCommand(cmd, iss);
    }
}

void CLI::processMCUCommand(const std::string& cmd, std::istringstream& iss) {
    if (cmd == "get_temp") {
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
            getMCUStatus();
        }
    }
    else if (cmd == "set_sim_params") {
        std::string mcu_name, sensor_id;
        double start_temp, end_temp, step_size;
        if (iss >> mcu_name >> sensor_id >> start_temp >> end_temp >> step_size) {
            setSimulationParams(mcu_name, sensor_id, start_temp, end_temp, step_size);
        } else {
            std::cout << "Usage: set_sim_params <mcu_name> <sensor_id> <start_temp> <end_temp> <step_size>" << std::endl;
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

void CLI::processFanCommand(const std::string& cmd, std::istringstream& iss) {
    // Fan operations
    if (cmd == "get_fan_status") {
        std::string fan_name;
        if (iss >> fan_name) {
            getFanStatus(fan_name);
        } else {
            getFanStatus();
        }
    }
    else if (cmd == "set_fan_speed") {
        std::string fan_name;
        int32_t duty_cycle;
        if (iss >> fan_name >> duty_cycle) {
            setFanSpeed(fan_name, duty_cycle);
        } else {
            std::cout << "Usage: set_fan_speed <fan_name> <duty_cycle>" << std::endl;
        }
    }
    else if (cmd == "set_fan_speed_all") {
        int32_t duty_cycle;
        if (iss >> duty_cycle) {
            setFanSpeedAll(duty_cycle);
        } else {
            std::cout << "Usage: set_fan_speed_all <duty_cycle>" << std::endl;
        }
    }
    else if (cmd == "set_fan_pwm") {
        std::string fan_name;
        int32_t pwm_count;
        if (iss >> fan_name >> pwm_count) {
            setFanPWM(fan_name, pwm_count);
        } else {
            std::cout << "Usage: set_fan_pwm <fan_name> <pwm_count>" << std::endl;
        }
    }
    else if (cmd == "make_fan_bad") {
        std::string fan_name;
        if (iss >> fan_name) {
            makeFanBad(fan_name);
        } else {
            std::cout << "Usage: make_fan_bad <fan_name>" << std::endl;
        }
    }
    else if (cmd == "make_fan_good") {
        std::string fan_name;
        if (iss >> fan_name) {
            makeFanGood(fan_name);
        } else {
            std::cout << "Usage: make_fan_good <fan_name>" << std::endl;
        }
    }
    else if (cmd == "get_fan_noise") {
        std::string fan_name;
        if (iss >> fan_name) {
            getFanNoise(fan_name);
        } else {
            std::cout << "Usage: get_fan_noise <fan_name>" << std::endl;
        }
    }
    // Temperature operations
    else if (cmd == "get_temp_status") {
        getTemperatureStatus();
    }
    else if (cmd == "get_temp_history") {
        std::string mcu_name, sensor_id;
        int32_t max_readings;
        if (iss >> mcu_name >> sensor_id >> max_readings) {
            getTemperatureHistory(mcu_name, sensor_id, max_readings);
        } else {
            std::cout << "Usage: get_temp_history <mcu> <sensor> <count>" << std::endl;
        }
    }
    else if (cmd == "get_cooling_status") {
        getCoolingStatus();
    }
    else if (cmd == "set_temp_thresholds") {
        double temp_low, temp_high;
        int32_t fan_speed_min, fan_speed_max;
        if (iss >> temp_low >> temp_high >> fan_speed_min >> fan_speed_max) {
            setTemperatureThresholds(temp_low, temp_high, fan_speed_min, fan_speed_max);
        } else {
            std::cout << "Usage: set_temp_thresholds <low> <high> <min_speed> <max_speed>" << std::endl;
        }
    }
    else if (cmd == "get_temp_thresholds") {
        getTemperatureThresholds();
    }
    // Alarm operations
    else if (cmd == "get_alarm_status") {
        getAlarmStatus();
    }
    else if (cmd == "raise_alarm") {
        std::string alarm_name, message, severity;
        if (iss >> alarm_name >> message >> severity) {
            raiseAlarm(alarm_name, message, severity);
        } else {
            std::cout << "Usage: raise_alarm <name> <message> <severity>" << std::endl;
        }
    }
    else if (cmd == "get_alarm_history") {
        int32_t max_entries;
        if (iss >> max_entries) {
            getAlarmHistory(max_entries);
        } else {
            std::cout << "Usage: get_alarm_history <count>" << std::endl;
        }
    }
    else if (cmd == "enable_alarm") {
        std::string alarm_name;
        if (iss >> alarm_name) {
            enableAlarm(alarm_name);
        } else {
            std::cout << "Usage: enable_alarm <name>" << std::endl;
        }
    }
    else if (cmd == "disable_alarm") {
        std::string alarm_name;
        if (iss >> alarm_name) {
            disableAlarm(alarm_name);
        } else {
            std::cout << "Usage: disable_alarm <name>" << std::endl;
        }
    }
    // Log operations
    else if (cmd == "get_log_status") {
        getLogStatus();
    }
    else if (cmd == "get_recent_logs") {
        int32_t max_entries;
        std::string level;
        if (iss >> max_entries) {
            if (iss >> level) {
                getRecentLogs(max_entries, level);
            } else {
                getRecentLogs(max_entries);
            }
        } else {
            std::cout << "Usage: get_recent_logs <count> [level]" << std::endl;
        }
    }
    else if (cmd == "set_log_level") {
        std::string level;
        if (iss >> level) {
            setLogLevel(level);
        } else {
            std::cout << "Usage: set_log_level <level>" << std::endl;
        }
    }
    else if (cmd == "rotate_log") {
        rotateLog();
    }
    else {
        std::cout << "Unknown command. Type 'help' for available commands." << std::endl;
    }
}

void CLI::showMainMenu() {
    std::cout << std::endl;
    std::cout << "Fan Control System Debug CLI" << std::endl;
    std::cout << "============================" << std::endl;
    std::cout << std::endl;
    std::cout << "Available services:" << std::endl;
    std::cout << "1. MCU Simulator" << std::endl;
    std::cout << "2. Fan Control System" << std::endl;
    std::cout << "3. Exit" << std::endl;
    std::cout << std::endl;
}

void CLI::showMCUHelp() {
    std::cout << "Available commands:" << std::endl;
    std::cout << "  help  - Show this help message" << std::endl;
    std::cout << "  exit  - Return to main menu" << std::endl;
    std::cout << "  quit  - Exit the application" << std::endl;
    std::cout << "  get_temp <mcu_name> <sensor_id>  - Get temperature from a specific sensor" << std::endl;
    std::cout << "  get_mcu_status [mcu_name]  - Get status of all MCUs or a specific MCU" << std::endl;
    std::cout << "  set_sim_params <mcu_name> <sensor_id> <start_temp> <end_temp> <step_size>  - Set simulation parameters" << std::endl;
    std::cout << "  set_mcu_fault <mcu_name> <is_faulty>  - Set MCU fault state (0=normal, 1=faulty)" << std::endl;
    std::cout << "  set_sensor_fault <mcu_name> <sensor_id> <is_faulty>  - Set sensor fault state (0=normal, 1=faulty)" << std::endl;
    std::cout << "  set_sensor_noise <mcu_name> <sensor_id> <is_noisy>  - Set sensor noise state (0=normal, 1=noisy)" << std::endl;
}

void CLI::showFanHelp() {
    std::cout << "Available commands:" << std::endl;
    std::cout << "  # Fan operations" << std::endl;
    std::cout << "  get_fan_status [fan_name]           - Get fan status" << std::endl;
    std::cout << "  set_fan_speed <fan_name> <duty_cycle> - Set fan speed" << std::endl;
    std::cout << "  set_fan_speed_all <duty_cycle>      - Set all fan speeds" << std::endl;
    std::cout << "  set_fan_pwm <fan_name> <pwm_count>  - Set fan PWM" << std::endl;
    std::cout << "  make_fan_bad <fan_name>             - Make fan faulty" << std::endl;
    std::cout << "  make_fan_good <fan_name>            - Restore fan" << std::endl;
    std::cout << "  get_fan_noise <fan_name>            - Get noise level" << std::endl;
    std::cout << std::endl;
    std::cout << "  # Temperature operations" << std::endl;
    std::cout << "  get_temp_status                     - Get temperature status" << std::endl;
    std::cout << "  get_temp_history <mcu> <sensor> <count> - Get temperature history" << std::endl;
    std::cout << "  get_cooling_status                  - Get cooling status" << std::endl;
    std::cout << "  set_temp_thresholds <low> <high> <min_speed> <max_speed> - Set thresholds" << std::endl;
    std::cout << "  get_temp_thresholds                 - Get current thresholds" << std::endl;
    std::cout << std::endl;
    std::cout << "  # Alarm operations" << std::endl;
    std::cout << "  get_alarm_status                    - Get alarm status" << std::endl;
    std::cout << "  raise_alarm <name> <message> <severity> - Raise alarm" << std::endl;
    std::cout << "  get_alarm_history <count>           - Get alarm history" << std::endl;
    std::cout << "  enable_alarm <name>                 - Enable alarm" << std::endl;
    std::cout << "  disable_alarm <name>                - Disable alarm" << std::endl;
    std::cout << std::endl;
    std::cout << "  # Log operations" << std::endl;
    std::cout << "  get_log_status                      - Get log status" << std::endl;
    std::cout << "  get_recent_logs <count> [level]     - Get recent logs" << std::endl;
    std::cout << "  set_log_level <level>               - Set log level" << std::endl;
    std::cout << "  rotate_log                          - Rotate log file" << std::endl;
    std::cout << std::endl;
    std::cout << "  help                                - Show this help" << std::endl;
    std::cout << "  exit                                - Return to main menu" << std::endl;
    std::cout << "  quit                                - Exit CLI" << std::endl;
}

void CLI::stop() {
    running_ = false;
    disconnectFromService();
}

// MCU Simulator RPC implementations
void CLI::getTemperature(const std::string& mcu_name, const std::string& sensor_id) {
    mcu_simulator::TemperatureRequest request;
    request.set_mcu_name(mcu_name);
    request.set_sensor_id(sensor_id);

    mcu_simulator::TemperatureResponse response;
    grpc::ClientContext context;

    grpc::Status status = mcu_stub_->GetTemperature(&context, request, &response);
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

    grpc::Status status = mcu_stub_->GetMCUStatus(&context, request, &response);
    if (status.ok()) {
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
            std::cout << std::endl;
        }
    } else {
        std::cout << "RPC failed: " << status.error_message() << std::endl;
    }
}

void CLI::setSimulationParams(const std::string& mcu_name, const std::string& sensor_id, double start_temp, double end_temp, double step_size) {
    mcu_simulator::SimulationParams request;
    request.set_mcu_name(mcu_name);
    request.set_sensor_id(sensor_id);
    request.set_start_temp(start_temp);
    request.set_end_temp(end_temp);
    request.set_step_size(step_size);

    mcu_simulator::SimulationResponse response;
    grpc::ClientContext context;

    grpc::Status status = mcu_stub_->SetSimulationParams(&context, request, &response);
    if (status.ok()) {
        if (response.success()) {
            std::cout << "Simulation parameters set successfully" << std::endl;
        } else {
            std::cout << "Error: " << response.message() << std::endl;
        }
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

    grpc::Status status = mcu_stub_->SetMCUFault(&context, request, &response);
    if (status.ok()) {
        if (response.success()) {
            std::cout << "MCU " << mcu_name << " is now " << (is_faulty ? "faulty" : "normal") << std::endl;
            std::cout << "Current state: " << response.current_state() << std::endl;
        } else {
            std::cout << "Error: " << response.message() << std::endl;
        }
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

    grpc::Status status = mcu_stub_->SetSensorFault(&context, request, &response);
    if (status.ok()) {
        if (response.success()) {
            std::cout << "Sensor " << mcu_name << ":" << sensor_id << " is now " << (is_faulty ? "faulty" : "normal") << std::endl;
            std::cout << "Current state: " << response.current_state() << std::endl;
        } else {
            std::cout << "Error: " << response.message() << std::endl;
        }
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

    grpc::Status status = mcu_stub_->SetSensorNoise(&context, request, &response);
    if (status.ok()) {
        if (response.success()) {
            std::cout << "Sensor " << mcu_name << ":" << sensor_id << " is now " << (is_noisy ? "noisy" : "normal") << std::endl;
            std::cout << "Current state: " << response.current_state() << std::endl;
        } else {
            std::cout << "Error: " << response.message() << std::endl;
        }
    } else {
        std::cout << "RPC failed: " << status.error_message() << std::endl;
    }
}

// Fan Control System RPC implementations - TODO placeholders
void CLI::getFanStatus(const std::string& fan_name) {
    std::cout << "TODO: Implement getFanStatus for fan: " << fan_name << std::endl;
}

void CLI::setFanSpeed(const std::string& fan_name, int32_t duty_cycle) {
    std::cout << "TODO: Implement setFanSpeed for fan: " << fan_name << " duty_cycle: " << duty_cycle << std::endl;
}

void CLI::setFanSpeedAll(int32_t duty_cycle) {
    std::cout << "TODO: Implement setFanSpeedAll duty_cycle: " << duty_cycle << std::endl;
}

void CLI::setFanPWM(const std::string& fan_name, int32_t pwm_count) {
    std::cout << "TODO: Implement setFanPWM for fan: " << fan_name << " pwm_count: " << pwm_count << std::endl;
}

void CLI::makeFanBad(const std::string& fan_name) {
    std::cout << "TODO: Implement makeFanBad for fan: " << fan_name << std::endl;
}

void CLI::makeFanGood(const std::string& fan_name) {
    std::cout << "TODO: Implement makeFanGood for fan: " << fan_name << std::endl;
}

void CLI::getFanNoise(const std::string& fan_name) {
    std::cout << "TODO: Implement getFanNoise for fan: " << fan_name << std::endl;
}

void CLI::getTemperatureStatus() {
    std::cout << "TODO: Implement getTemperatureStatus" << std::endl;
}

void CLI::getTemperatureHistory(const std::string& mcu_name, const std::string& sensor_id, int32_t max_readings) {
    std::cout << "TODO: Implement getTemperatureHistory for " << mcu_name << ":" << sensor_id << " max_readings: " << max_readings << std::endl;
}

void CLI::getCoolingStatus() {
    std::cout << "TODO: Implement getCoolingStatus" << std::endl;
}

void CLI::setTemperatureThresholds(double temp_low, double temp_high, int32_t fan_speed_min, int32_t fan_speed_max) {
    std::cout << "TODO: Implement setTemperatureThresholds low: " << temp_low << " high: " << temp_high 
              << " min_speed: " << fan_speed_min << " max_speed: " << fan_speed_max << std::endl;
}

void CLI::getTemperatureThresholds() {
    std::cout << "TODO: Implement getTemperatureThresholds" << std::endl;
}

void CLI::getAlarmStatus() {
    std::cout << "TODO: Implement getAlarmStatus" << std::endl;
}

void CLI::raiseAlarm(const std::string& alarm_name, const std::string& message, const std::string& severity) {
    std::cout << "TODO: Implement raiseAlarm name: " << alarm_name << " message: " << message << " severity: " << severity << std::endl;
}

void CLI::getAlarmHistory(int32_t max_entries) {
    std::cout << "TODO: Implement getAlarmHistory max_entries: " << max_entries << std::endl;
}

void CLI::enableAlarm(const std::string& alarm_name) {
    std::cout << "TODO: Implement enableAlarm name: " << alarm_name << std::endl;
}

void CLI::disableAlarm(const std::string& alarm_name) {
    std::cout << "TODO: Implement disableAlarm name: " << alarm_name << std::endl;
}

void CLI::getLogStatus() {
    std::cout << "TODO: Implement getLogStatus" << std::endl;
}

void CLI::getRecentLogs(int32_t max_entries, const std::string& level) {
    std::cout << "TODO: Implement getRecentLogs max_entries: " << max_entries << " level: " << level << std::endl;
}

void CLI::setLogLevel(const std::string& level) {
    std::cout << "TODO: Implement setLogLevel level: " << level << std::endl;
}

void CLI::rotateLog() {
    std::cout << "TODO: Implement rotateLog" << std::endl;
}

} // namespace cli 