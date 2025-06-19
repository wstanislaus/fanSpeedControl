#pragma once

#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "common/config.hpp"
#include "mcu_simulator.grpc.pb.h"
#include "fan_control_system.grpc.pb.h"

namespace cli {

enum class ServiceType {
    MCU_SIMULATOR,
    FAN_CONTROL_SYSTEM,
    EXIT
};

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
    // Service selection and connection
    ServiceType selectService();
    bool connectToService(ServiceType service);
    void disconnectFromService();
    
    // Process user commands
    void processCommand(const std::string& command);
    void processMCUCommand(const std::string& cmd, std::istringstream& iss);
    void processFanCommand(const std::string& cmd, std::istringstream& iss);
    
    // Display help
    void showMainMenu();
    void showMCUHelp();
    void showFanHelp();

    // MCU Simulator RPC methods
    void getTemperature(const std::string& mcu_name, const std::string& sensor_id);
    void getMCUStatus(const std::string& mcu_name = "");
    void setSimulationParams(const std::string& mcu_name, const std::string& sensor_id, double start_temp, double end_temp, double step_size);
    void setMCUFault(const std::string& mcu_name, bool is_faulty);
    void setSensorFault(const std::string& mcu_name, const std::string& sensor_id, bool is_faulty);
    void setSensorNoise(const std::string& mcu_name, const std::string& sensor_id, bool is_noisy);

    // Fan Control System RPC methods
    void getFanStatus(const std::string& fan_name = "");
    void setFanSpeed(const std::string& fan_name, int32_t duty_cycle);
    void setFanSpeedAll(int32_t duty_cycle);
    void setFanPWM(const std::string& fan_name, int32_t pwm_count);
    void makeFanBad(const std::string& fan_name);
    void makeFanGood(const std::string& fan_name);
    void getFanNoise(const std::string& fan_name);
    
    void getTemperatureHistory(const std::string& mcu_name, const std::string& sensor_id, int32_t max_readings);
    void getCoolingStatus();
    void setTemperatureThresholds(double temp_low, double temp_high, int32_t fan_speed_min, int32_t fan_speed_max);
    void getTemperatureThresholds();
    
    void getAlarmStatus();
    void raiseAlarm(const std::string& alarm_name, const std::string& message, const std::string& severity);
    void getAlarmHistory(int32_t max_entries);
    void enableAlarm(const std::string& alarm_name);
    void disableAlarm(const std::string& alarm_name);
    
    void getLogStatus();
    void getRecentLogs(int32_t max_entries, const std::string& level = "");
    void setLogLevel(const std::string& level);
    void rotateLog();


    // Service connections
    std::shared_ptr<grpc::ChannelInterface> mcu_channel_;
    std::unique_ptr<mcu_simulator::MCUSimulatorService::Stub> mcu_stub_;
    
    std::shared_ptr<grpc::ChannelInterface> fan_channel_;
    std::unique_ptr<fan_control_system::FanControlSystemService::Stub> fan_stub_;
    
    ServiceType current_service_;
    bool running_{false};
};

} // namespace cli 