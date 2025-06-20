#include "fan_control_system/fan_control_system_server.hpp"
#include "fan_control_system/fan_simulator.hpp"
#include "fan_control_system/temp_monitor_and_cooling.hpp"
#include "fan_control_system/alarm_manager.hpp"
#include <iostream>
#include "common/config.hpp"
#include <chrono>
#include <sstream>
#include <iomanip>

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
            fan_status->set_interface(fan_pair.second->getInterface());
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
        fan_status->set_interface(fan->getInterface());
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
    
    if (request->fan_name().empty()) {
        int current_duty_cycle = 0;
        for (const auto& fan_pair : fan_simulator->get_fans()) {
            current_duty_cycle = fan_pair.second->getDutyCycle();
            // We just need to get one fan to get the current duty cycle
            break;
        }
        // Set speed for all fans
        if (!fan_simulator->set_fan_speed(request->duty_cycle())) {
            return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to set fan speed for all fans");
        }
        
        // Add results for each fan
        for (const auto& fan_pair : fan_simulator->get_fans()) {
            auto* result = response->add_results();
            result->set_fan_name(fan_pair.first);
            result->set_success(true);
            result->set_previous_duty_cycle(current_duty_cycle);
            result->set_new_duty_cycle(request->duty_cycle());
        }
        
        response->set_success(true);
        response->set_message("Fan speed set successfully for all fans");
    } else {
        const auto fan_speed = fan_simulator->get_fan_speed(request->fan_name());
        if (fan_speed == -1) {
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "Fan not found: " + request->fan_name());
        }
        // Set speed for specific fan
        if (!fan_simulator->set_fan_speed(request->fan_name(), request->duty_cycle())) {
            return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to set fan speed");
        }
        
        // Add result for the specific fan
        auto* result = response->add_results();
        result->set_fan_name(request->fan_name());
        result->set_previous_duty_cycle(fan_speed);
        result->set_success(true);
        result->set_new_duty_cycle(request->duty_cycle());
        
        response->set_success(true);
        response->set_message("Fan speed set successfully");
    }
    
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
    response->set_noise_category(fan_simulator->get_fan_noise_category(request->fan_name()));
    return grpc::Status::OK;
}

// Temperature Monitor operations
grpc::Status FanControlSystemServiceImpl::GetTemperatureHistory(grpc::ServerContext* context,
                                                               const TemperatureHistoryRequest* request,
                                                               TemperatureHistoryResponse* response) {
    const auto& temp_monitor = system_.get_temp_monitor_and_cooling();
    if (!temp_monitor) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Temperature monitor not available");
    }
    
    auto temperature_history = temp_monitor->get_temperature_history(request->mcu_name(), request->sensor_id(), request->max_readings());
    
    if (temperature_history.empty()) {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Temperature history not found");
    }
    
    // Convert to proto format
    for (const auto& reading : temperature_history) {
        auto* proto_reading = response->add_readings();
        proto_reading->set_mcu_name(reading.mcu_name);
        proto_reading->set_sensor_id(reading.sensor_id);
        proto_reading->set_temperature(reading.temperature);
        proto_reading->set_status(reading.status);
        
        // Convert timestamp to string
        auto time_t = std::chrono::system_clock::to_time_t(reading.timestamp);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        proto_reading->set_timestamp(ss.str());
    }
    
    response->set_total_readings(temperature_history.size());
    return grpc::Status::OK;
}

grpc::Status FanControlSystemServiceImpl::SetTemperatureThresholds(grpc::ServerContext* context,
                                                                  const TemperatureThresholdsRequest* request,
                                                                  TemperatureThresholdsResponse* response) {
    const auto& temp_monitor = system_.get_temp_monitor_and_cooling();
    if (!temp_monitor) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Temperature monitor not available");
    }
    temp_monitor->set_thresholds(request->temp_threshold_low(), request->temp_threshold_high(), request->fan_speed_min(), request->fan_speed_max());
    response->set_success(true);
    response->set_message("Temperature thresholds set successfully");
    return grpc::Status::OK;
}

grpc::Status FanControlSystemServiceImpl::GetTemperatureThresholds(grpc::ServerContext* context,
                                                                  const GetTemperatureThresholdsRequest* request,
                                                                  GetTemperatureThresholdsResponse* response) {
    const auto& temp_monitor = system_.get_temp_monitor_and_cooling();
    if (!temp_monitor) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Temperature monitor not available");
    }
    
    auto thresholds = temp_monitor->get_thresholds();
    response->set_temp_threshold_low(thresholds.temp_threshold_low);
    response->set_temp_threshold_high(thresholds.temp_threshold_high);
    response->set_fan_speed_min(thresholds.fan_speed_min);
    response->set_fan_speed_max(thresholds.fan_speed_max);
    
    return grpc::Status::OK;
}

grpc::Status FanControlSystemServiceImpl::GetCoolingStatus(grpc::ServerContext* context,
                                                          const CoolingStatusRequest* request,
                                                          CoolingStatusResponse* response) {
    const auto& temp_monitor = system_.get_temp_monitor_and_cooling();
    if (!temp_monitor) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Temperature monitor not available");
    }
    auto cooling_status = temp_monitor->get_cooling_status();
    response->set_average_temperature(cooling_status.average_temperature);
    response->set_current_fan_speed(cooling_status.current_fan_speed);
    response->set_cooling_mode(cooling_status.cooling_mode);
    return grpc::Status::OK;
}

// Helper function to convert ProtoAlarmSeverity to AlarmSeverity
AlarmSeverity convertProtoSeverity(ProtoAlarmSeverity proto_severity) {
    switch (proto_severity) {
        case ProtoAlarmSeverity::PROTO_ALARM_INFO:
            return AlarmSeverity::INFO;
        case ProtoAlarmSeverity::PROTO_ALARM_WARNING:
            return AlarmSeverity::WARNING;
        case ProtoAlarmSeverity::PROTO_ALARM_ERROR:
            return AlarmSeverity::ERROR;
        case ProtoAlarmSeverity::PROTO_ALARM_CRITICAL:
            return AlarmSeverity::CRITICAL;
        default:
            return AlarmSeverity::INFO; // Default fallback
    }
}

// Alarm Manager operations
grpc::Status FanControlSystemServiceImpl::RaiseAlarm(grpc::ServerContext* context,
                                                    const RaiseAlarmRequest* request,
                                                    RaiseAlarmResponse* response) {
    const auto& alarm_manager = system_.get_alarm_manager();
    if (!alarm_manager) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Alarm manager not available");
    }
    
    AlarmSeverity severity = convertProtoSeverity(request->severity());
    alarm_manager->raise_alarm(request->alarm_source(), severity, request->message());
    response->set_success(true);
    response->set_message("Alarm raised successfully");
    return grpc::Status::OK;
}

grpc::Status FanControlSystemServiceImpl::GetAlarmHistory(grpc::ServerContext* context,
                                                         const AlarmHistoryRequest* request,
                                                         AlarmHistoryResponse* response) {
    const auto& alarm_manager = system_.get_alarm_manager();
    if (!alarm_manager) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Alarm manager not available");
    }
    
    auto alarm_history = alarm_manager->get_alarm_history(request->alarm_name(), request->max_entries());
    
    for (const auto& entry : alarm_history) {
        auto* proto_entry = response->add_entries();
        proto_entry->set_alarm_name(entry.name);
        proto_entry->set_message(entry.message);
        proto_entry->set_severity(static_cast<ProtoAlarmSeverity>(entry.severity));
        proto_entry->set_timestamp(entry.timestamp);
        proto_entry->set_was_acknowledged(entry.acknowledged);
    }
    
    response->set_total_entries(alarm_history.size());
    return grpc::Status::OK;
}

grpc::Status FanControlSystemServiceImpl::GetAlarmConfig(grpc::ServerContext* context,
                                                        const AlarmConfigRequest* request,
                                                        AlarmConfigResponse* response) {
    const auto& alarm_manager = system_.get_alarm_manager();
    if (!alarm_manager) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Alarm manager not available");
    }
    
    auto alarm_config = alarm_manager->get_alarm_config();
    
    // Create a single config entry representing the system configuration
    auto* proto_config = response->add_configs();
    proto_config->set_alarm_history_size(alarm_config.alarm_history_size);
    
    // Add severity actions
    for (const auto& pair : alarm_config.severity_actions) {
        auto& proto_actions = proto_config->mutable_severity_actions()->operator[](pair.first);
        for (const auto& action : pair.second) {
            proto_actions.add_actions(action);
        }
    }
    return grpc::Status::OK;
}

grpc::Status FanControlSystemServiceImpl::GetSeverityActions(grpc::ServerContext* context,
                                                           const SeverityActionsRequest* request,
                                                           SeverityActionsResponse* response) {
    const auto& alarm_manager = system_.get_alarm_manager();
    if (!alarm_manager) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Alarm manager not available");
    }
    
    auto severity_actions = alarm_manager->get_severity_actions();
    
    for (const auto& pair : severity_actions) {
        auto& proto_actions = response->mutable_severity_actions()->operator[](pair.first);
        for (const auto& action : pair.second) {
            proto_actions.add_actions(action);
        }
    }
    
    return grpc::Status::OK;
}

grpc::Status FanControlSystemServiceImpl::ClearAlarmHistory(grpc::ServerContext* context,
                                                          const ClearAlarmHistoryRequest* request,
                                                          ClearAlarmHistoryResponse* response) {
    const auto& alarm_manager = system_.get_alarm_manager();
    if (!alarm_manager) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Alarm manager not available");
    }
    
    int cleared_count = alarm_manager->clear_alarm_history(request->alarm_name());
    response->set_success(true);
    response->set_message("Alarm history cleared successfully");
    response->set_cleared_entries(cleared_count);
    return grpc::Status::OK;
}

grpc::Status FanControlSystemServiceImpl::GetAlarmStatistics(grpc::ServerContext* context,
                                                           const AlarmStatisticsRequest* request,
                                                           AlarmStatisticsResponse* response) {
    const auto& alarm_manager = system_.get_alarm_manager();
    if (!alarm_manager) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Alarm manager not available");
    }
    
    auto statistics = alarm_manager->get_alarm_statistics(request->alarm_name(), request->time_window_hours());
    
    for (const auto& stat : statistics) {
        auto* proto_stat = response->add_statistics();
        proto_stat->set_alarm_name(stat.alarm_name);
        proto_stat->set_total_count(stat.total_count);
        proto_stat->set_active_count(stat.active_count);
        proto_stat->set_acknowledged_count(stat.acknowledged_count);
        proto_stat->set_last_occurrence(stat.last_occurrence);
        proto_stat->set_first_occurrence(stat.first_occurrence);
        
        for (const auto& severity_pair : stat.severity_counts) {
            proto_stat->mutable_severity_counts()->operator[](severity_pair.first) = severity_pair.second;
        }
    }
    
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