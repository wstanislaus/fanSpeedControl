#include "fan_control_system/fan_control_system_server.hpp"
#include "fan_control_system/fan_simulator.hpp"
#include <iostream>
#include "common/config.hpp"

namespace fan_control_system {

FanControlSystemServiceImpl::FanControlSystemServiceImpl(FanControlSystem& system)
    : system_(system) {
}

// Fan Simulator operations
grpc::Status FanControlSystemServiceImpl::GetFanStatus(grpc::ServerContext* context, 
                                                      const FanStatusRequest* request,
                                                      FanStatusResponse* response) {
    const auto& fan_simulator = system_.get_fan_simulator();
    if (!fan_simulator) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Fan simulator not available");
    }
    if (request->fan_name().empty()) {
        // Return status of all fans
        for (const auto& fan_pair : fan_simulator->get_fans()) {
            auto* fan_status = response->add_fans();
            fan_status->set_name(fan_pair.second->getName());
            fan_status->set_model(fan_pair.second->getModelName());
            fan_status->set_is_online(fan_pair.second->getStatus() != "Bad");
            fan_status->set_current_duty_cycle(fan_pair.second->getDutyCycle());
            fan_status->set_current_pwm(fan_pair.second->getPWMCount());
            fan_status->set_noise_level_db(fan_pair.second->getNoiseLevel());
            fan_status->set_status(fan_pair.second->getStatus());
            fan_status->set_interface(fan_pair.second->getModelName());
            fan_status->set_i2c_address(fan_pair.second->getI2CAddress());
            fan_status->set_pwm_min(fan_pair.second->getPWMMin());
            fan_status->set_pwm_max(fan_pair.second->getPWMMax());
            fan_status->set_duty_cycle_min(fan_pair.second->getDutyCycleMin());
            fan_status->set_duty_cycle_max(fan_pair.second->getDutyCycleMax());
        }
    } else {
        // Return status of specific fan
        auto fan = fan_simulator->get_fan(request->fan_name());
        if (!fan) {
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "Fan not found: " + request->fan_name());
        }
        auto* fan_status = response->add_fans();
        fan_status->set_name(fan->getName());
        fan_status->set_model(fan->getModelName());
        fan_status->set_is_online(fan->getStatus() != "Bad");
        fan_status->set_current_duty_cycle(fan->getDutyCycle());
        fan_status->set_current_pwm(fan->getPWMCount());
        fan_status->set_noise_level_db(fan->getNoiseLevel());
        fan_status->set_status(fan->getStatus());
        fan_status->set_interface(fan->getModelName());
        fan_status->set_i2c_address(fan->getI2CAddress());
        fan_status->set_pwm_min(fan->getPWMMin());
        fan_status->set_pwm_max(fan->getPWMMax());
        fan_status->set_duty_cycle_min(fan->getDutyCycleMin());
        fan_status->set_duty_cycle_max(fan->getDutyCycleMax());
    }
    return grpc::Status::OK;
}

grpc::Status FanControlSystemServiceImpl::SetFanSpeed(grpc::ServerContext* context,
                                                     const FanSpeedRequest* request,
                                                     FanSpeedResponse* response) {
    const auto& fan_simulator = system_.get_fan_simulator();
    if (!fan_simulator) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Fan simulator not available");
    }
    if (!fan_simulator->set_fan_speed(request->fan_name(), request->duty_cycle())) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to set fan speed");
    }
    response->set_success(true);
    response->set_message("Fan speed set successfully");
    return grpc::Status::OK;
}

grpc::Status FanControlSystemServiceImpl::MakeFanBad(grpc::ServerContext* context,
                                                    const FanFaultRequest* request,
                                                    FaultResponse* response) {
    const auto& fan_simulator = system_.get_fan_simulator();
    if (!fan_simulator) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Fan simulator not available");
    }
    if (!fan_simulator->make_fan_bad(request->fan_name())) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to make fan bad");
    }
    response->set_success(true);
    response->set_message("Fan made bad successfully");
    return grpc::Status::OK;
}

grpc::Status FanControlSystemServiceImpl::MakeFanGood(grpc::ServerContext* context,
                                                     const FanFaultRequest* request,
                                                     FaultResponse* response) {
    const auto& fan_simulator = system_.get_fan_simulator();
    if (!fan_simulator) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Fan simulator not available");
    }
    if (!fan_simulator->make_fan_good(request->fan_name())) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to make fan good");
    }
    response->set_success(true);
    response->set_message("Fan made good successfully");
    return grpc::Status::OK;
}

grpc::Status FanControlSystemServiceImpl::SetFanPWM(grpc::ServerContext* context,
                                                   const FanPWMRequest* request,
                                                   FanPWMResponse* response) {
    const auto& fan_simulator = system_.get_fan_simulator();
    if (!fan_simulator) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Fan simulator not available");
    }
    if (!fan_simulator->set_fan_pwm(request->fan_name(), request->pwm_count())) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to set fan PWM");
    }
    response->set_success(true);
    response->set_message("Fan PWM set successfully");
    return grpc::Status::OK;
}

grpc::Status FanControlSystemServiceImpl::GetFanNoiseLevel(grpc::ServerContext* context,
                                                          const FanNoiseRequest* request,
                                                          FanNoiseResponse* response) {
    const auto& fan_simulator = system_.get_fan_simulator();
    if (!fan_simulator) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Fan simulator not available");
    }
    int noise_level = fan_simulator->get_fan_noise_level(request->fan_name());
    if (noise_level == -1) {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Fan not found: " + request->fan_name());
    }
    response->set_noise_level_db(noise_level);
    return grpc::Status::OK;
}

// Temperature Monitor operations
grpc::Status FanControlSystemServiceImpl::GetTemperatureHistory(grpc::ServerContext* context,
                                                               const TemperatureHistoryRequest* request,
                                                               TemperatureHistoryResponse* response) {
    // TODO: Implement GetTemperatureHistory
    std::cout << "TODO: Implement GetTemperatureHistory" << std::endl;
    return grpc::Status::OK;
}

grpc::Status FanControlSystemServiceImpl::SetTemperatureThresholds(grpc::ServerContext* context,
                                                                  const TemperatureThresholdsRequest* request,
                                                                  TemperatureThresholdsResponse* response) {
    // TODO: Implement SetTemperatureThresholds
    std::cout << "TODO: Implement SetTemperatureThresholds" << std::endl;
    return grpc::Status::OK;
}

grpc::Status FanControlSystemServiceImpl::GetCoolingStatus(grpc::ServerContext* context,
                                                          const CoolingStatusRequest* request,
                                                          CoolingStatusResponse* response) {
    // TODO: Implement GetCoolingStatus
    std::cout << "TODO: Implement GetCoolingStatus" << std::endl;
    return grpc::Status::OK;
}

// Alarm Manager operations
grpc::Status FanControlSystemServiceImpl::GetAlarmStatus(grpc::ServerContext* context,
                                                        const AlarmStatusRequest* request,
                                                        AlarmStatusResponse* response) {
    // TODO: Implement GetAlarmStatus
    std::cout << "TODO: Implement GetAlarmStatus" << std::endl;
    return grpc::Status::OK;
}

grpc::Status FanControlSystemServiceImpl::RaiseAlarm(grpc::ServerContext* context,
                                                    const RaiseAlarmRequest* request,
                                                    RaiseAlarmResponse* response) {
    // TODO: Implement RaiseAlarm
    std::cout << "TODO: Implement RaiseAlarm" << std::endl;
    return grpc::Status::OK;
}

grpc::Status FanControlSystemServiceImpl::GetAlarmHistory(grpc::ServerContext* context,
                                                         const AlarmHistoryRequest* request,
                                                         AlarmHistoryResponse* response) {
    // TODO: Implement GetAlarmHistory
    std::cout << "TODO: Implement GetAlarmHistory" << std::endl;
    return grpc::Status::OK;
}

grpc::Status FanControlSystemServiceImpl::EnableAlarm(grpc::ServerContext* context,
                                                     const AlarmControlRequest* request,
                                                     AlarmControlResponse* response) {
    // TODO: Implement EnableAlarm
    std::cout << "TODO: Implement EnableAlarm" << std::endl;
    return grpc::Status::OK;
}

grpc::Status FanControlSystemServiceImpl::DisableAlarm(grpc::ServerContext* context,
                                                      const AlarmControlRequest* request,
                                                      AlarmControlResponse* response) {
    // TODO: Implement DisableAlarm
    std::cout << "TODO: Implement DisableAlarm" << std::endl;
    return grpc::Status::OK;
}

// Log Manager operations
grpc::Status FanControlSystemServiceImpl::GetLogStatus(grpc::ServerContext* context,
                                                      const LogStatusRequest* request,
                                                      LogStatusResponse* response) {
    // TODO: Implement GetLogStatus
    std::cout << "TODO: Implement GetLogStatus" << std::endl;
    return grpc::Status::OK;
}

grpc::Status FanControlSystemServiceImpl::GetRecentLogs(grpc::ServerContext* context,
                                                       const RecentLogsRequest* request,
                                                       RecentLogsResponse* response) {
    // TODO: Implement GetRecentLogs
    std::cout << "TODO: Implement GetRecentLogs" << std::endl;
    return grpc::Status::OK;
}

grpc::Status FanControlSystemServiceImpl::SetLogLevel(grpc::ServerContext* context,
                                                     const LogLevelRequest* request,
                                                     LogLevelResponse* response) {
    // TODO: Implement SetLogLevel
    std::cout << "TODO: Implement SetLogLevel" << std::endl;
    return grpc::Status::OK;
}

grpc::Status FanControlSystemServiceImpl::RotateLogFile(grpc::ServerContext* context,
                                                       const LogRotateRequest* request,
                                                       LogRotateResponse* response) {
    // TODO: Implement RotateLogFile
    std::cout << "TODO: Implement RotateLogFile" << std::endl;
    return grpc::Status::OK;
}

// FanControlSystemServer implementation
FanControlSystemServer::FanControlSystemServer(FanControlSystem& system)
    : RPCServer("FanControlSystem", 
                common::Config::getInstance().getRPCServerConfig("FanControlSystem")->port,
                common::Config::getInstance().getRPCServerConfig("FanControlSystem")->max_connections)
    , system_(system)
    , service_(std::make_unique<FanControlSystemServiceImpl>(system)) {
}

void FanControlSystemServer::addServices(grpc::ServerBuilder& builder) {
    builder.RegisterService(service_.get());
}

} // namespace fan_control_system 