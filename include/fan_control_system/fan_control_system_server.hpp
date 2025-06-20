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

    grpc::Status GetTemperatureThresholds(grpc::ServerContext* context,
                                        const GetTemperatureThresholdsRequest* request,
                                        GetTemperatureThresholdsResponse* response) override;

    grpc::Status GetCoolingStatus(grpc::ServerContext* context,
                                const CoolingStatusRequest* request,
                                CoolingStatusResponse* response) override;

    // Alarm Manager operations
    grpc::Status RaiseAlarm(grpc::ServerContext* context,
                          const RaiseAlarmRequest* request,
                          RaiseAlarmResponse* response) override;

    grpc::Status GetAlarmHistory(grpc::ServerContext* context,
                               const AlarmHistoryRequest* request,
                               AlarmHistoryResponse* response) override;

    grpc::Status GetAlarmConfig(grpc::ServerContext* context,
                              const AlarmConfigRequest* request,
                              AlarmConfigResponse* response) override;

    grpc::Status GetSeverityActions(grpc::ServerContext* context,
                                  const SeverityActionsRequest* request,
                                  SeverityActionsResponse* response) override;

    grpc::Status ClearAlarmHistory(grpc::ServerContext* context,
                                 const ClearAlarmHistoryRequest* request,
                                 ClearAlarmHistoryResponse* response) override;

    grpc::Status GetAlarmStatistics(grpc::ServerContext* context,
                                  const AlarmStatisticsRequest* request,
                                  AlarmStatisticsResponse* response) override;

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