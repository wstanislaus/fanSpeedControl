#pragma once

#include "common/rpc_server.hpp"
#include "fan_control_system/fan_control_system.hpp"
#include "fan_control_system.grpc.pb.h"
#include <memory>
#include <unordered_map>
#include <mutex>

namespace fan_control_system {

class FanControlSystemServiceImpl final : public FanControlSystemService::Service {
public:
    explicit FanControlSystemServiceImpl(FanControlSystem& system);

    // Fan Simulator operations
    grpc::Status GetFanStatus(grpc::ServerContext* context, 
                            const FanStatusRequest* request,
                            FanStatusResponse* response) override;

    grpc::Status SetFanSpeed(grpc::ServerContext* context,
                           const FanSpeedRequest* request,
                           FanSpeedResponse* response) override;

    grpc::Status MakeFanBad(grpc::ServerContext* context,
                          const FanFaultRequest* request,
                          FaultResponse* response) override;

    grpc::Status MakeFanGood(grpc::ServerContext* context,
                           const FanFaultRequest* request,
                           FaultResponse* response) override;

    grpc::Status SetFanPWM(grpc::ServerContext* context,
                         const FanPWMRequest* request,
                         FanPWMResponse* response) override;

    grpc::Status GetFanNoiseLevel(grpc::ServerContext* context,
                                const FanNoiseRequest* request,
                                FanNoiseResponse* response) override;

    // Temperature Monitor operations
    grpc::Status GetTemperatureHistory(grpc::ServerContext* context,
                                     const TemperatureHistoryRequest* request,
                                     TemperatureHistoryResponse* response) override;

    grpc::Status SetTemperatureThresholds(grpc::ServerContext* context,
                                        const TemperatureThresholdsRequest* request,
                                        TemperatureThresholdsResponse* response) override;

    grpc::Status GetCoolingStatus(grpc::ServerContext* context,
                                const CoolingStatusRequest* request,
                                CoolingStatusResponse* response) override;

    // Alarm Manager operations
    grpc::Status GetAlarmStatus(grpc::ServerContext* context,
                              const AlarmStatusRequest* request,
                              AlarmStatusResponse* response) override;

    grpc::Status RaiseAlarm(grpc::ServerContext* context,
                          const RaiseAlarmRequest* request,
                          RaiseAlarmResponse* response) override;

    grpc::Status GetAlarmHistory(grpc::ServerContext* context,
                               const AlarmHistoryRequest* request,
                               AlarmHistoryResponse* response) override;

    grpc::Status EnableAlarm(grpc::ServerContext* context,
                           const AlarmControlRequest* request,
                           AlarmControlResponse* response) override;

    grpc::Status DisableAlarm(grpc::ServerContext* context,
                            const AlarmControlRequest* request,
                            AlarmControlResponse* response) override;

    // Log Manager operations
    grpc::Status GetLogStatus(grpc::ServerContext* context,
                            const LogStatusRequest* request,
                            LogStatusResponse* response) override;

    grpc::Status GetRecentLogs(grpc::ServerContext* context,
                             const RecentLogsRequest* request,
                             RecentLogsResponse* response) override;

    grpc::Status SetLogLevel(grpc::ServerContext* context,
                           const LogLevelRequest* request,
                           LogLevelResponse* response) override;

    grpc::Status RotateLogFile(grpc::ServerContext* context,
                             const LogRotateRequest* request,
                             LogRotateResponse* response) override;

private:
    FanControlSystem& system_;
};

class FanControlSystemServer : public common::RPCServer {
public:
    /**
     * @brief Constructs a new Fan Control System Server
     * @param system Reference to the fan control system instance
     */
    explicit FanControlSystemServer(FanControlSystem& system);
    ~FanControlSystemServer() override = default;

protected:
    // Implement the virtual method from RPCServer
    void addServices(grpc::ServerBuilder& builder) override;

private:
    FanControlSystem& system_;
    std::unique_ptr<FanControlSystemServiceImpl> service_;
};

} // namespace fan_control_system 