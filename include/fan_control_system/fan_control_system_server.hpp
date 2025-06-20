#pragma once

#include "common/rpc_server.hpp"
#include "fan_control_system/fan_control_system.hpp"
#include "fan_control_system.grpc.pb.h"
#include <memory>
#include <unordered_map>
#include <mutex>

namespace fan_control_system {

/**
 * @class FanControlSystemServiceImpl
 * @brief Implementation of the Fan Control System gRPC service
 * 
 * This class implements all the gRPC service methods defined in the protobuf
 * interface. It provides remote access to fan control, temperature monitoring,
 * and alarm management functionality.
 */
class FanControlSystemServiceImpl final : public FanControlSystemService::Service {
public:
    /**
     * @brief Constructs a new Fan Control System Service implementation
     * @param system Reference to the fan control system instance
     * @note This constructor establishes the connection between the service and the system
     */
    explicit FanControlSystemServiceImpl(FanControlSystem& system);

    // Fan Simulator operations
    /**
     * @brief Gets the status of one or all fans
     * @param context gRPC server context
     * @param request Request containing optional fan name filter
     * @param response Response containing fan status information
     * @return gRPC status indicating success or failure
     * @note This method retrieves operational status, speed, and health of fans
     */
    grpc::Status GetFanStatus(grpc::ServerContext* context, 
                            const FanStatusRequest* request,
                            FanStatusResponse* response) override;

    /**
     * @brief Sets the speed of a specific fan
     * @param context gRPC server context
     * @param request Request containing fan name and desired speed
     * @param response Response containing operation result
     * @return gRPC status indicating success or failure
     * @note This method controls fan speed using PWM duty cycle (0-100%)
     */
    grpc::Status SetFanSpeed(grpc::ServerContext* context,
                           const FanSpeedRequest* request,
                           FanSpeedResponse* response) override;

    /**
     * @brief Simulates a bad/faulty fan condition
     * @param context gRPC server context
     * @param request Request containing fan name to make faulty
     * @param response Response containing operation result
     * @return gRPC status indicating success or failure
     * @note This method simulates fan hardware faults for testing alarm systems
     */
    grpc::Status MakeFanBad(grpc::ServerContext* context,
                          const FanFaultRequest* request,
                          FaultResponse* response) override;

    /**
     * @brief Restores a fan to good working condition
     * @param context gRPC server context
     * @param request Request containing fan name to restore
     * @param response Response containing operation result
     * @return gRPC status indicating success or failure
     * @note This method clears simulated fault conditions on a fan
     */
    grpc::Status MakeFanGood(grpc::ServerContext* context,
                           const FanFaultRequest* request,
                           FaultResponse* response) override;

    /**
     * @brief Sets the raw PWM value for a specific fan
     * @param context gRPC server context
     * @param request Request containing fan name and PWM count
     * @param response Response containing operation result
     * @return gRPC status indicating success or failure
     * @note This method sets the raw PWM value directly, bypassing duty cycle calculation
     */
    grpc::Status SetFanPWM(grpc::ServerContext* context,
                         const FanPWMRequest* request,
                         FanPWMResponse* response) override;

    /**
     * @brief Gets noise level information for a fan
     * @param context gRPC server context
     * @param request Request containing fan name to query
     * @param response Response containing noise level data
     * @return gRPC status indicating success or failure
     * @note This method retrieves acoustic noise measurements from the fan
     */
    grpc::Status GetFanNoiseLevel(grpc::ServerContext* context,
                                const FanNoiseRequest* request,
                                FanNoiseResponse* response) override;

    // Temperature Monitor operations
    /**
     * @brief Gets temperature history from a sensor
     * @param context gRPC server context
     * @param request Request containing sensor details and max readings
     * @param response Response containing historical temperature data
     * @return gRPC status indicating success or failure
     * @note This method retrieves historical temperature data for analysis
     */
    grpc::Status GetTemperatureHistory(grpc::ServerContext* context,
                                     const TemperatureHistoryRequest* request,
                                     TemperatureHistoryResponse* response) override;

    /**
     * @brief Sets temperature thresholds for automatic fan control
     * @param context gRPC server context
     * @param request Request containing temperature and fan speed thresholds
     * @param response Response containing operation result
     * @return gRPC status indicating success or failure
     * @note This method configures automatic temperature-based fan control parameters
     */
    grpc::Status SetTemperatureThresholds(grpc::ServerContext* context,
                                        const TemperatureThresholdsRequest* request,
                                        TemperatureThresholdsResponse* response) override;

    /**
     * @brief Gets current temperature thresholds
     * @param context gRPC server context
     * @param request Request for threshold information
     * @param response Response containing current threshold values
     * @return gRPC status indicating success or failure
     * @note This method retrieves the currently configured temperature control thresholds
     */
    grpc::Status GetTemperatureThresholds(grpc::ServerContext* context,
                                        const GetTemperatureThresholdsRequest* request,
                                        GetTemperatureThresholdsResponse* response) override;

    /**
     * @brief Gets the current cooling system status
     * @param context gRPC server context
     * @param request Request for cooling status
     * @param response Response containing cooling system performance data
     * @return gRPC status indicating success or failure
     * @note This method retrieves overall cooling system performance and status
     */
    grpc::Status GetCoolingStatus(grpc::ServerContext* context,
                                const CoolingStatusRequest* request,
                                CoolingStatusResponse* response) override;

    // Alarm Manager operations
    /**
     * @brief Raises an alarm in the system
     * @param context gRPC server context
     * @param request Request containing alarm details
     * @param response Response containing operation result
     * @return gRPC status indicating success or failure
     * @note This method triggers alarms for testing alarm management systems
     */
    grpc::Status RaiseAlarm(grpc::ServerContext* context,
                          const RaiseAlarmRequest* request,
                          RaiseAlarmResponse* response) override;

    /**
     * @brief Gets alarm history
     * @param context gRPC server context
     * @param request Request containing filter parameters
     * @param response Response containing historical alarm data
     * @return gRPC status indicating success or failure
     * @note This method retrieves historical alarm data for analysis
     */
    grpc::Status GetAlarmHistory(grpc::ServerContext* context,
                               const AlarmHistoryRequest* request,
                               AlarmHistoryResponse* response) override;

    /**
     * @brief Gets alarm system configuration
     * @param context gRPC server context
     * @param request Request for configuration data
     * @param response Response containing alarm configuration
     * @return gRPC status indicating success or failure
     * @note This method retrieves the current alarm system configuration
     */
    grpc::Status GetAlarmConfig(grpc::ServerContext* context,
                              const AlarmConfigRequest* request,
                              AlarmConfigResponse* response) override;

    /**
     * @brief Gets severity-based actions configuration
     * @param context gRPC server context
     * @param request Request for severity actions
     * @param response Response containing severity action mappings
     * @return gRPC status indicating success or failure
     * @note This method retrieves the configured actions for each alarm severity level
     */
    grpc::Status GetSeverityActions(grpc::ServerContext* context,
                                  const SeverityActionsRequest* request,
                                  SeverityActionsResponse* response) override;

    /**
     * @brief Clears alarm history
     * @param context gRPC server context
     * @param request Request containing optional alarm name filter
     * @param response Response containing operation result
     * @return gRPC status indicating success or failure
     * @note This method clears historical alarm data from the system
     */
    grpc::Status ClearAlarmHistory(grpc::ServerContext* context,
                                 const ClearAlarmHistoryRequest* request,
                                 ClearAlarmHistoryResponse* response) override;

    /**
     * @brief Gets alarm statistics
     * @param context gRPC server context
     * @param request Request containing filter and time window parameters
     * @param response Response containing statistical analysis data
     * @return gRPC status indicating success or failure
     * @note This method provides statistical analysis of alarm occurrences
     */
    grpc::Status GetAlarmStatistics(grpc::ServerContext* context,
                                  const AlarmStatisticsRequest* request,
                                  AlarmStatisticsResponse* response) override;

private:
    FanControlSystem& system_;  ///< Reference to the fan control system instance
};

/**
 * @class FanControlSystemServer
 * @brief gRPC server for the Fan Control System
 * 
 * This class extends the base RPCServer to provide gRPC functionality
 * specifically for the Fan Control System, managing the service implementation
 * and server lifecycle.
 */
class FanControlSystemServer : public common::RPCServer {
public:
    /**
     * @brief Constructs a new Fan Control System Server
     * @param system Reference to the fan control system instance
     * @note This constructor initializes the server with the system reference
     */
    explicit FanControlSystemServer(FanControlSystem& system);

    /**
     * @brief Destructor for proper cleanup
     * @note This destructor ensures proper cleanup of server resources
     */
    ~FanControlSystemServer() override = default;

protected:
    /**
     * @brief Adds the Fan Control System service to the gRPC server
     * @param builder gRPC ServerBuilder instance for registering services
     * @note This method registers the FanControlSystemServiceImpl with the server
     */
    void addServices(grpc::ServerBuilder& builder) override;

private:
    FanControlSystem& system_;                                    ///< Reference to the fan control system instance
    std::unique_ptr<FanControlSystemServiceImpl> service_;        ///< Service implementation instance
};

} // namespace fan_control_system 