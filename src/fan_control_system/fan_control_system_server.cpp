#include "fan_control_system/fan_control_system_server.hpp"
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
    // TODO: Implement GetFanStatus
    std::cout << "TODO: Implement GetFanStatus" << std::endl;
    return grpc::Status::OK;
}

grpc::Status FanControlSystemServiceImpl::SetFanSpeed(grpc::ServerContext* context,
                                                     const FanSpeedRequest* request,
                                                     FanSpeedResponse* response) {
    // TODO: Implement SetFanSpeed
    std::cout << "TODO: Implement SetFanSpeed" << std::endl;
    return grpc::Status::OK;
}

grpc::Status FanControlSystemServiceImpl::MakeFanBad(grpc::ServerContext* context,
                                                    const FanFaultRequest* request,
                                                    FaultResponse* response) {
    // TODO: Implement MakeFanBad
    std::cout << "TODO: Implement MakeFanBad" << std::endl;
    return grpc::Status::OK;
}

grpc::Status FanControlSystemServiceImpl::MakeFanGood(grpc::ServerContext* context,
                                                     const FanFaultRequest* request,
                                                     FaultResponse* response) {
    // TODO: Implement MakeFanGood
    std::cout << "TODO: Implement MakeFanGood" << std::endl;
    return grpc::Status::OK;
}

grpc::Status FanControlSystemServiceImpl::SetFanPWM(grpc::ServerContext* context,
                                                   const FanPWMRequest* request,
                                                   FanPWMResponse* response) {
    // TODO: Implement SetFanPWM
    std::cout << "TODO: Implement SetFanPWM" << std::endl;
    return grpc::Status::OK;
}

grpc::Status FanControlSystemServiceImpl::GetFanNoiseLevel(grpc::ServerContext* context,
                                                          const FanNoiseRequest* request,
                                                          FanNoiseResponse* response) {
    // TODO: Implement GetFanNoiseLevel
    std::cout << "TODO: Implement GetFanNoiseLevel" << std::endl;
    return grpc::Status::OK;
}

// Temperature Monitor operations
grpc::Status FanControlSystemServiceImpl::GetTemperatureStatus(grpc::ServerContext* context,
                                                              const TemperatureStatusRequest* request,
                                                              TemperatureStatusResponse* response) {
    // TODO: Implement GetTemperatureStatus
    std::cout << "TODO: Implement GetTemperatureStatus" << std::endl;
    return grpc::Status::OK;
}

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