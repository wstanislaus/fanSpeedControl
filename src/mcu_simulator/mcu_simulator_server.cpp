#include "mcu_simulator/mcu_simulator_server.hpp"
#include "mcu_simulator/temperature_sensor.hpp"
#include "common/config.hpp"
#include "common/logger.hpp"
#include <iostream>

namespace mcu_simulator {

/**
 * @brief Constructs a new MCU Simulator Service implementation
 * 
 * Initializes the RPC service implementation with a reference to the MCU simulator.
 * This service handles all gRPC requests for MCU simulator operations.
 * 
 * @param simulator Reference to the MCU simulator instance
 */
MCUSimulatorServiceImpl::MCUSimulatorServiceImpl(MCUSimulator& simulator)
    : simulator_(simulator) {
}

/**
 * @brief Handles GetTemperature RPC requests
 * 
 * Retrieves the current temperature from a specific sensor on a specific MCU.
 * Validates that the MCU exists and is not faulty before attempting to read
 * the temperature.
 * 
 * @param context gRPC server context
 * @param request Temperature request containing MCU name and sensor ID
 * @param response Temperature response containing the temperature value and validity
 * @return gRPC status indicating success or failure
 */
grpc::Status MCUSimulatorServiceImpl::GetTemperature(grpc::ServerContext* context,
                                                   const TemperatureRequest* request,
                                                   TemperatureResponse* response) {
    try {
        const auto& mcus = simulator_.getAllMCUs();
        auto it = std::find_if(mcus.begin(), mcus.end(),
                             [&](const auto& mcu) { return mcu->getName() == request->mcu_name(); });
        
        if (it == mcus.end()) {
            response->set_is_valid(false);
            response->set_error_message("MCU not found: " + request->mcu_name());
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "MCU not found");
        } else if ((*it)->isFaulty()) {
            response->set_is_valid(false);
            response->set_error_message("MCU is faulty: " + request->mcu_name());
            return grpc::Status(grpc::StatusCode::INTERNAL, "MCU: " + request->mcu_name() + " is faulty");
        }

        // Get temperature from the specified sensor
        double temperature;
        bool valid = (*it)->getSensorTemperature(request->sensor_id(), temperature);
        
        // Round temperature to 2 decimal places
        temperature = std::round(temperature * 100.0) / 100.0;
        
        response->set_temperature(temperature);
        response->set_is_valid(valid);
        if (!valid) {
            response->set_error_message("Invalid temperature reading from sensor: " + request->sensor_id());
        }
        
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}

/**
 * @brief Handles GetMCUStatus RPC requests
 * 
 * Retrieves the status of MCUs and their sensors. If no specific MCU name is provided,
 * returns status for all MCUs. Includes information about sensor interfaces, addresses,
 * and operational status.
 * 
 * @param context gRPC server context
 * @param request Status request containing optional MCU name
 * @param response Status response containing MCU and sensor status information
 * @return gRPC status indicating success or failure
 */
grpc::Status MCUSimulatorServiceImpl::GetMCUStatus(grpc::ServerContext* context,
                                                 const StatusRequest* request,
                                                 StatusResponse* response) {
    try {
        const auto& mcus = simulator_.getAllMCUs();
        
        if (request->mcu_name().empty()) {
            // Return status of all MCUs
            for (const auto& mcu : mcus) {
                auto* mcu_status = response->add_mcu_status();
                mcu_status->set_mcu_name(mcu->getName());
                mcu_status->set_is_online(!mcu->isFaulty());
                mcu_status->set_active_sensors(mcu->isFaulty() ? 0 : mcu->getActiveSensorCount());

                // Add sensor status for all sensors, but mark them as inactive if MCU is faulty
                for (const auto& sensor : mcu->getSensors()) {
                    auto* sensor_status = mcu_status->add_sensors();
                    sensor_status->set_sensor_id(std::to_string(sensor->getId()));
                    sensor_status->set_is_active(!mcu->isFaulty() && sensor->getStatus() != "Bad");
                    sensor_status->set_interface(sensor->getInterface());
                    sensor_status->set_address(sensor->getAddress());
                    sensor_status->set_is_noisy(sensor->getNoisy());
                }
            }
        } else {
            // Return status of specific MCU
            auto it = std::find_if(mcus.begin(), mcus.end(),
                                 [&](const auto& mcu) { return mcu->getName() == request->mcu_name(); });
            
            if (it == mcus.end()) {
                return grpc::Status(grpc::StatusCode::NOT_FOUND, "MCU not found");
            }

            auto* mcu_status = response->add_mcu_status();
            mcu_status->set_mcu_name((*it)->getName());
            mcu_status->set_is_online(!(*it)->isFaulty());
            mcu_status->set_active_sensors((*it)->isFaulty() ? 0 : (*it)->getActiveSensorCount());

            // Add sensor status for all sensors, but mark them as inactive if MCU is faulty
            for (const auto& sensor : (*it)->getSensors()) {
                auto* sensor_status = mcu_status->add_sensors();
                sensor_status->set_sensor_id(std::to_string(sensor->getId()));
                sensor_status->set_is_active(!(*it)->isFaulty() && sensor->getStatus() != "Bad");
                sensor_status->set_interface(sensor->getInterface());
                sensor_status->set_address(sensor->getAddress());
                sensor_status->set_is_noisy(sensor->getNoisy());
            }
        }
        
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}

/**
 * @brief Handles SetSimulationParams RPC requests
 * 
 * Sets the temperature simulation parameters for a specific sensor on a specific MCU.
 * Parameters include start temperature, end temperature, and step size for the
 * temperature oscillation simulation.
 * 
 * @param context gRPC server context
 * @param request Simulation parameters request
 * @param response Simulation response indicating success or failure
 * @return gRPC status indicating success or failure
 */
grpc::Status MCUSimulatorServiceImpl::SetSimulationParams(grpc::ServerContext* context,
                                                        const SimulationParams* request,
                                                        SimulationResponse* response) {
    try {
        const auto& mcus = simulator_.getAllMCUs();
        auto it = std::find_if(mcus.begin(), mcus.end(),
                             [&](const auto& mcu) { return mcu->getName() == request->mcu_name(); });
        
        if (it == mcus.end()) {
            response->set_success(false);
            response->set_message("MCU not found: " + request->mcu_name());
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "MCU not found");
        }
        bool success = (*it)->setSimulationParams(std::stoi(request->sensor_id()), request->start_temp(), request->end_temp(), request->step_size());
        if (!success) {
            response->set_success(false);
            response->set_message("Failed to set simulation parameters for sensor: " + request->sensor_id());
            return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to set simulation parameters for sensor: " + request->sensor_id());
        }
        response->set_success(true);
        response->set_message("Simulation parameters updated successfully");
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        response->set_success(false);
        response->set_message(e.what());
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}

/**
 * @brief Handles SetMCUFault RPC requests
 * 
 * Sets the fault state of a specific MCU. When an MCU is set to faulty,
 * all its sensors will report invalid readings.
 * 
 * @param context gRPC server context
 * @param request MCU fault request containing MCU name and fault state
 * @param response Fault response indicating success or failure
 * @return gRPC status indicating success or failure
 */
grpc::Status MCUSimulatorServiceImpl::SetMCUFault(grpc::ServerContext* context,
                                                const MCUFaultRequest* request,
                                                FaultResponse* response) {
    try {
        const auto& mcus = simulator_.getAllMCUs();
        auto it = std::find_if(mcus.begin(), mcus.end(),
                             [&](const auto& mcu) { return mcu->getName() == request->mcu_name(); });
        
        if (it == mcus.end()) {
            response->set_success(false);
            response->set_message("MCU not found: " + request->mcu_name());
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "MCU not found");
        }

        (*it)->setFaulty(request->is_faulty());
        response->set_success(true);
        response->set_message("MCU fault state updated successfully");
        response->set_current_state(request->is_faulty() ? "faulty" : "normal");
        
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        response->set_success(false);
        response->set_message(e.what());
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}

/**
 * @brief Handles SetSensorFault RPC requests
 * 
 * Sets the fault state of a specific sensor on a specific MCU. When a sensor
 * is set to faulty, it will report invalid temperature readings.
 * 
 * @param context gRPC server context
 * @param request Sensor fault request containing MCU name, sensor ID, and fault state
 * @param response Fault response indicating success or failure
 * @return gRPC status indicating success or failure
 */
grpc::Status MCUSimulatorServiceImpl::SetSensorFault(grpc::ServerContext* context,
                                                   const SensorFaultRequest* request,
                                                   FaultResponse* response) {
    try {
        const auto& mcus = simulator_.getAllMCUs();
        auto it = std::find_if(mcus.begin(), mcus.end(),
                             [&](const auto& mcu) { return mcu->getName() == request->mcu_name(); });
        
        if (it == mcus.end()) {
            response->set_success(false);
            response->set_message("MCU not found: " + request->mcu_name());
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "MCU not found");
        }

        bool success = (*it)->makeSensorBad(std::stoi(request->sensor_id()), request->is_faulty());
        if (!success) {
            response->set_success(false);
            response->set_message("Sensor not found: " + request->sensor_id());
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "Sensor not found");
        }

        response->set_success(true);
        response->set_message("Sensor fault state updated successfully");
        response->set_current_state(request->is_faulty() ? "faulty" : "normal");
        
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        response->set_success(false);
        response->set_message(e.what());
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}

/**
 * @brief Handles SetSensorNoise RPC requests
 * 
 * Sets the noise state of a specific sensor on a specific MCU. When a sensor
 * is set to noisy, it will add random variations to its temperature readings.
 * 
 * @param context gRPC server context
 * @param request Sensor noise request containing MCU name, sensor ID, and noise state
 * @param response Fault response indicating success or failure
 * @return gRPC status indicating success or failure
 */
grpc::Status MCUSimulatorServiceImpl::SetSensorNoise(grpc::ServerContext* context,
                                                   const SensorNoiseRequest* request,
                                                   FaultResponse* response) {
    try {
        const auto& mcus = simulator_.getAllMCUs();
        auto it = std::find_if(mcus.begin(), mcus.end(),
                             [&](const auto& mcu) { return mcu->getName() == request->mcu_name(); });
        
        if (it == mcus.end()) {
            response->set_success(false);
            response->set_message("MCU not found: " + request->mcu_name());
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "MCU not found");
        }

        bool success = (*it)->makeSensorNoisy(std::stoi(request->sensor_id()), request->is_noisy());
        if (!success) {
            response->set_success(false);
            response->set_message("Sensor not found: " + request->sensor_id());
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "Sensor not found");
        }

        response->set_success(true);
        response->set_message("Sensor noise state updated successfully");
        response->set_current_state(request->is_noisy() ? "noisy" : "normal");
        
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        response->set_success(false);
        response->set_message(e.what());
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}

// MCUSimulatorServer implementation
MCUSimulatorServer::MCUSimulatorServer(MCUSimulator& simulator)
    : RPCServer("MCUSimulator", 
                common::Config::getInstance().getRPCServerConfig("MCUSimulator")->port,
                common::Config::getInstance().getRPCServerConfig("MCUSimulator")->max_connections)
    , simulator_(simulator)
    , service_(std::make_unique<MCUSimulatorServiceImpl>(simulator)) {
}

void MCUSimulatorServer::addServices(grpc::ServerBuilder& builder) {
    builder.RegisterService(service_.get());
}

} // namespace mcu_simulator 