# Fan Speed Control System - Code Walkthrough


## Code Architecture Walkthrough

### Project Structure Deep Dive

```
src/
├── mcu_simulator/                         # Temperature sensor simulation
│   ├── mcu_simulator.cpp                  # Main simulator logic
│   ├── mcu_simulator_server.cpp           # gRPC server implementation
│   ├── mcu.cpp                            # MCU base class
│   ├── temperature_sensor.cpp             # Sensor simulation
│   └── main.cpp                           # Entry point
├── fan_control_system/                    # Core fan control logic
│   ├── fan_control_system.cpp             # Main control system
│   ├── fan_control_system_server.cpp      # gRPC server
│   ├── fan.cpp                            # Fan management
│   ├── fan_simulator.cpp                  # Hardware simulation
│   ├── temp_monitor_and_cooling.cpp       # Core algorithm
│   ├── alarm_manager.cpp                  # Fault detection
│   ├── log_manager.cpp                    # Centralized logging
│   └── main.cpp                           # Entry point
├── common/                                # Shared utilities
│   ├── mqtt_client.cpp                    # MQTT communication
│   ├── config.cpp                         # Configuration management
│   ├── logger.cpp                         # Logging infrastructure
│   ├── rpc_server.cpp                     # gRPC base classes
│   ├── alarm.cpp                          # Alarm definitions
│   └── utils.cpp                          # Utility functions
├── cli/                                   # Interactive debugging interface
│   ├── cli.cpp                            # CLI logic
│   ├── main.cpp                           # Entry point
│   └── CMakeLists.txt                     # Build configuration
└── proto/                                 # gRPC service definitions
    ├── mcu_simulator.proto                # MCU service interface
    ├── fan_control_system.proto           # Fan control interface
    └── CMakeLists.txt                     # Protobuf build config
```

### Key Components Deep Dive

#### MCU Simulator Architecture

**Core Classes**:
```cpp
// MCU base class
class MCU {
private:
    std::string name;
    std::vector<TemperatureSensor> sensors;
    MQTTClient& mqttClient;
    Config& config;
    
public:
    void simulateTemperature();
    void publishReadings();
    void injectFault(int sensorId, bool faulty);
    MCUStatus getStatus();
};

// Temperature sensor simulation
class TemperatureSensor {
private:
    int sensorId;
    float currentTemp;
    float targetTemp;
    float stepSize;
    std::string status; // "Good", "Bad"
    
public:
    float readTemperature();
    void setSimulationParams(float target, float step);
    void setFault(bool faulty);
    std::string getStatus();
};
```

**Key Features**:
1. **Dynamic Temperature Simulation**
   ```cpp
   void MCU::simulateTemperature() {
       for (auto& sensor : sensors) {
           float current = sensor.readTemperature();
           float target = config.getSimulatorTargetTemp();
           float step = config.getSimulatorStepSize();
           
           if (current < target) {
               sensor.setSimulationParams(target, step);
           }
       }
   }
   ```

2. **Adaptive Publish Intervals**
   ```cpp
   int MCU::getPublishInterval() {
       float maxTemp = getMaxTemperature();
       
       if (maxTemp < 25.0) return 10;      // 10 seconds
       else if (maxTemp < 40.0) return 7;  // 7 seconds
       else if (maxTemp < 50.0) return 5;  // 5 seconds
       else if (maxTemp < 60.0) return 3;  // 3 seconds
       else if (maxTemp < 70.0) return 2;  // 2 seconds
       else return 1;                      // 1 second
   }
   ```

3. **Fault Injection Capabilities**
   ```cpp
   void MCU::injectFault(int sensorId, bool faulty) {
       if (sensorId < sensors.size()) {
           sensors[sensorId].setFault(faulty);
           publishAlarm(sensorId, faulty);
       }
   }
   ```

#### Fan Control System Architecture

**Core Classes**:
```cpp
// Main fan control system
class FanControlSystem {
private:
    std::vector<Fan> fans;
    TempMonitorAndCooling tempMonitor;
    AlarmManager alarmManager;
    LogManager logManager;
    MQTTClient& mqttClient;
    
public:
    void start();
    void processTemperatureUpdates();
    void controlFans();
    void handleAlarms();
};

// Temperature monitoring and cooling logic
class TempMonitorAndCooling {
private:
    std::deque<TemperatureReading> history;
    float minTemp, maxTemp, minDutyCycle, maxDutyCycle;
    
public:
    float calculateDutyCycle(float maxTemp);
    void updateTemperatureHistory(const TemperatureReading& reading);
    CoolingStatus getStatus();
};
```

**Control Algorithm Implementation**:
```cpp
float TempMonitorAndCooling::calculateDutyCycle(float maxTemp) {
    // Core algorithm: linear interpolation
    if (maxTemp <= minTemp) {
        return minDutyCycle;  // 20% at 25°C or below
    }
    
    if (maxTemp >= maxTemp) {
        return maxDutyCycle;  // 100% at 75°C or above
    }
    
    // Linear interpolation between 25°C and 75°C
    float tempRange = maxTemp - minTemp;
    float dutyRange = maxDutyCycle - minDutyCycle;
    float slope = dutyRange / tempRange;
    
    return minDutyCycle + slope * (maxTemp - minTemp);
}
```

**Fan Management**:
```cpp
class Fan {
private:
    std::string name;
    std::string model;
    int pwmCount;
    float dutyCycle;
    FanModel& modelConfig;
    
public:
    void setDutyCycle(float dutyCycle);
    void setPWMCount(int pwmCount);
    float getNoiseLevel();
    FanStatus getStatus();
};

void Fan::setDutyCycle(float newDutyCycle) {
    dutyCycle = std::clamp(newDutyCycle, 
                          modelConfig.getMinDutyCycle(), 
                          modelConfig.getMaxDutyCycle());
    
    // Convert duty cycle to PWM counts
    int newPWM = modelConfig.dutyCycleToPWM(dutyCycle);
    setPWMCount(newPWM);
    
    // Publish status update
    publishStatus();
}
```

#### Common Infrastructure

**MQTT Client**:
```cpp
class MQTTClient {
private:
    mosqpp::mosquittopp client;
    std::string broker;
    int port;
    
public:
    void connect();
    void publish(const std::string& topic, const std::string& message);
    void subscribe(const std::string& topic);
    void on_message(const mosquitto_message* message);
};
```

**Configuration Management**:
```cpp
class Config {
private:
    YAML::Node config;
    
public:
    template<typename T>
    T get(const std::string& key, const T& defaultValue = T{});
    
    std::vector<MCUConfig> getMCUConfigs();
    std::vector<FanConfig> getFanConfigs();
    AlarmConfig getAlarmConfig();
};
```

### Communication Patterns Deep Dive

#### MQTT Message Formats

**Temperature Readings**:
```json
{
  "MCU": "MCU001",
  "NoOfTempSensors": 3,
  "MsgTimestamp": "2025-01-15 14:30:25",
  "SensorData": [
    {
      "SensorID": 1,
      "ReadAt": "2025-01-15 14:30:25",
      "Value": 43.3,
      "Status": "Good"
    },
    {
      "SensorID": 2,
      "ReadAt": "2025-01-15 14:30:25",
      "Value": 46.6,
      "Status": "Good"
    },
    {
      "SensorID": 3,
      "ReadAt": "2025-01-15 14:30:25",
      "Value": 48.7,
      "Status": "Good"
    }
  ]
}
```

**Alarm Messages**:
```json
{
  "timestamp": "2025-01-15 14:30:25",
  "severity": 2,
  "source": "MCU001",
  "message": "Temperature sensor failure detected",
  "state": "raised",
  "actions": ["LogEvent", "SendNotification"]
}
```

#### gRPC Service Definitions

**MCU Simulator Interface**:
```protobuf
service MCUSimulator {
    rpc GetTemperature(GetTemperatureRequest) returns (GetTemperatureResponse);
    rpc GetMCUStatus(GetMCUStatusRequest) returns (GetMCUStatusResponse);
    rpc SetSimulationParams(SetSimulationParamsRequest) returns (SetSimulationParamsResponse);
    rpc SetMCUFault(SetMCUFaultRequest) returns (SetMCUFaultResponse);
    rpc SetSensorFault(SetSensorFaultRequest) returns (SetSensorFaultResponse);
    rpc SetSensorNoise(SetSensorNoiseRequest) returns (SetSensorNoiseResponse);
}
```

**Fan Control System Interface**:
```protobuf
service FanControlSystem {
    // Fan operations
    rpc GetFanStatus(GetFanStatusRequest) returns (GetFanStatusResponse);
    rpc SetFanSpeed(SetFanSpeedRequest) returns (SetFanSpeedResponse);
    rpc MakeFanBad(MakeFanBadRequest) returns (MakeFanBadResponse);
    rpc GetFanNoiseLevel(GetFanNoiseLevelRequest) returns (GetFanNoiseLevelResponse);
    
    // Temperature monitoring
    rpc GetTemperatureHistory(GetTemperatureHistoryRequest) returns (GetTemperatureHistoryResponse);
    rpc SetTemperatureThresholds(SetTemperatureThresholdsRequest) returns (SetTemperatureThresholdsResponse);
    rpc GetCoolingStatus(GetCoolingStatusRequest) returns (GetCoolingStatusResponse);
    
    // Alarm management
    rpc GetAlarmStatus(GetAlarmStatusRequest) returns (GetAlarmStatusResponse);
    rpc RaiseAlarm(RaiseAlarmRequest) returns (RaiseAlarmResponse);
    rpc GetAlarmHistory(GetAlarmHistoryRequest) returns (GetAlarmHistoryResponse);
}
```