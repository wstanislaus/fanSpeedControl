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
// MCU base class - manages multiple temperature sensors and MQTT communication
class MCU {
private:
    std::string name_;                                          // Name of the MCU
    std::vector<std::unique_ptr<TemperatureSensor>> sensors_;   // Vector of temperature sensors
    std::shared_ptr<common::MQTTClient> mqtt_client_;           // MQTT client for communication
    bool running_;                                              // Flag indicating if the MCU is running
    bool is_faulty_ = false;                                    // Flag indicating if the MCU is in a faulty state
    
public:
    // Starts the temperature reading and publishing loop
    void start();
    
    // Stops the temperature reading and publishing loop
    void stop();
    
    // Makes a specific sensor report bad readings (for testing)
    bool makeSensorBad(int sensor_id, bool is_bad);
    
    // Makes a specific sensor report noisy readings (for testing)
    bool makeSensorNoisy(int sensor_id, bool is_noisy);
    
    // Sets the simulation parameters for a specific sensor
    bool setSimulationParams(int sensor_id, double start_temp, double end_temp, double step_size);
    
private:
    // Reads temperatures from all sensors and publishes them
    void readAndPublishTemperatures();
    
    // Calculates the next publish interval based on current temperature
    std::chrono::seconds calculatePublishInterval(float temperature) const;
};

// Temperature sensor simulation - represents individual temperature sensors
class TemperatureSensor {
private:
    int id_;                                                    // Unique identifier for the sensor
    std::string name_;                                          // Name of the sensor
    std::string status_;                                        // Current status of the sensor ("Good" or "Bad")
    bool is_noisy_;                                             // Whether the sensor is in noisy mode
    float start_temp_;                                          // Start temperature for simulation (°C)
    float end_temp_;                                            // End temperature for simulation (°C)
    float step_size_;                                           // Step size for simulation (°C)
    
public:
    // Reads the current temperature from the sensor
    float readTemperature();
    
    // Sets the status of the sensor (good/bad)
    void setStatus(bool is_bad);
    
    // Sets whether the sensor should report noisy readings
    void setNoisy(bool noisy);
    
    // Sets the simulation parameters for temperature generation
    void setSimulationParams(const double start_temp, const double end_temp, const double step_size);
};
```

**Key Features**:
1. **Dynamic Temperature Simulation**
   ```cpp
   void MCU::readAndPublishTemperatures() {
       // Reads temperatures from all sensors and publishes via MQTT
       // Includes adaptive publish intervals based on temperature
       // Handles erratic reading detection and alarm conditions
   }
   ```

2. **Adaptive Publish Intervals**
   ```cpp
   std::chrono::seconds MCU::calculatePublishInterval(float temperature) const {
       // Returns publish interval based on temperature thresholds
       // Higher temperatures = more frequent publishing
       // Lower temperatures = less frequent publishing
   }
   ```

3. **Fault Injection Capabilities**
   ```cpp
   bool MCU::makeSensorBad(int sensor_id, bool is_bad) {
       // Sets a specific sensor to bad/good state for testing
       // Triggers alarm conditions when sensor is bad
       // Used for testing fault detection and alarm systems
   }
   ```

#### Fan Control System Architecture

**Core Classes**:
```cpp
// Main fan control system - coordinates all components
class FanControlSystem {
private:
    std::shared_ptr<fan_control_system::FanSimulator> fan_simulator_;      // Fan simulator for controlling fan speeds
    std::shared_ptr<fan_control_system::TempMonitorAndCooling> temp_monitor_; // Temperature monitoring and cooling control
    std::shared_ptr<fan_control_system::AlarmManager> alarm_manager_;      // System-wide alarm management
    std::thread main_thread_;                                              // Main system thread
    std::atomic<bool> running_{false};                                     // System running state
    
public:
    // Starts the fan control system
    bool start();
    
    // Stops the fan control system
    void stop();
    
    // Checks if the system is currently running
    bool is_running() const;
    
private:
    // Main thread function that coordinates system operations
    void main_thread_function();
};

// Temperature monitoring and cooling logic - core control algorithm
class TempMonitorAndCooling {
private:
    std::map<std::string, std::map<int, TemperatureHistory>> temperature_history_;  // Temperature history for each sensor
    std::shared_ptr<FanSimulator> fan_simulator_;          // Fan simulator for speed control
    std::shared_ptr<common::MQTTClient> mqtt_client_;      // MQTT client for communication
    double temp_threshold_low_{25.0};                     // Below this, fans run at 20%
    double temp_threshold_high_{75.0};                    // Above this, fans run at 100%
    int fan_speed_min_{20};                              // Minimum fan speed percentage
    int fan_speed_max_{100};                             // Maximum fan speed percentage
    
public:
    // Starts the temperature monitor
    bool start();
    
    // Gets the current temperature for a specific MCU and sensor
    double get_temperature(const std::string& mcu_name, int sensor_id) const;
    
    // Sets the temperature thresholds
    void set_thresholds(double temp_threshold_low, double temp_threshold_high, int fan_speed_min, int fan_speed_max);
    
    // Gets the cooling status
    CoolingStatus get_cooling_status() const;
    
private:
    // Processes a new temperature reading
    void process_temperature_reading(const std::string& mcu_name, int sensor_id, double temperature, const std::string& status);
    
    // Calculates required fan speed based on current temperatures
    CoolingStatus calculate_fan_speed() const;
    
    // Updates the fan speed
    void update_fan_speed();
};
```

**Control Algorithm Implementation**:
```cpp
CoolingStatus TempMonitorAndCooling::calculate_fan_speed() const {
    // Core algorithm: linear interpolation based on temperature
    // Calculates fan speed between min and max thresholds
    // Returns cooling status with average temperature and fan speed
    // Handles multiple sensor readings and temperature history
}
```

**Fan Management**:
```cpp
class Fan {
private:
    std::string name_;                                          // Name of the fan
    std::string model_name_;                                    // Model name of the fan
    uint8_t i2c_address_;                                       // I2C address of the fan controller
    uint8_t pwm_reg_;                                          // PWM register address
    std::string status_;                                        // Current status of the fan
    int current_pwm_count_;                                    // Current pwm count based on the duty cycle
    int current_duty_cycle_;                                    // Current duty cycle based on the pwm count
    int pwm_min_;                                                // PWM minimum value
    int pwm_max_;                                                // PWM maximum value
    int duty_cycle_min_;                                         // Duty cycle minimum value
    int duty_cycle_max_;                                         // Duty cycle maximum value
    
public:
    // Initializes the fan and its components
    bool initialize();
    
    // Gets the current duty cycle of the fan
    int getDutyCycle() const;
    
    // Gets the current PWM count of the fan
    int getPWMCount() const;
    
    // Sets the duty cycle of the fan
    bool setPwmCount(int duty_cycle, int pwm_count);
    
    // Makes the fan report bad status (for testing)
    bool makeBad();
    
    // Makes the fan report good status (for testing)
    bool makeGood();
    
private:
    // Reads the current pwm count from the I2C register
    int readPwmCount();
    
    // Writes the pwm count to the I2C register
    bool writePwmCount(int pwm_count);
    
    // Publishes fan status via MQTT
    void publishStatus();
};

void Fan::setPwmCount(int duty_cycle, int pwm_count) {
    // Sets duty cycle and PWM count with bounds checking
    // Converts duty cycle to PWM counts based on fan model
    // Updates fan status and publishes via MQTT
    // Handles error conditions and logging
}
```

#### Common Infrastructure

**MQTT Client**:
```cpp
class MQTTClient {
private:
    std::string client_id_;                                     // Unique identifier for this MQTT client
    Settings settings_;                                         // MQTT client settings
    mosquitto* client_;                                         // Pointer to mosquitto client instance
    
public:
    // Connects to the MQTT broker
    bool connect();
    
    // Publishes a message to an MQTT topic
    bool publish(const std::string& topic, const std::string& payload);
    
    // Subscribes to an MQTT topic
    bool subscribe(const std::string& topic, int qos);
    
    // Disconnects from the MQTT broker
    void disconnect();
};
```

**Configuration Management**:
```cpp
class Config {
private:
    YAML::Node config_;                                         // Loaded configuration data
    bool loaded_ = false;                                       // Whether configuration has been loaded
    
public:
    // Gets the singleton instance of the Config class
    static Config& getInstance();
    
    // Loads configuration from a YAML file
    bool load(const std::string& config_file);
    
    // Gets the MQTT settings from the configuration
    MQTTClient::Settings getMQTTSettings() const;
    
    // Gets the configuration
    YAML::Node getConfig() const;
    
private:
    // Private constructor to enforce singleton pattern
    Config() = default;
    
    // Deleted copy constructor to prevent copying
    Config(const Config&) = delete;
};
```

### Communication Patterns Deep Dive

#### MQTT Message Formats

**Temperature Readings**:
```json
{
  "MCU": "MCU001",
  "NoOfTempSensors": 3,
  "MsgTimestamp": "2025-07-02 14:30:25",
  "SensorData": [
    {
      "SensorID": 1,
      "ReadAt": "2025-07-02 14:30:25",
      "Value": 43.3,
      "Status": "Good"
    },
    {
      "SensorID": 2,
      "ReadAt": "2025-07-02 14:30:25",
      "Value": 46.6,
      "Status": "Good"
    },
    {
      "SensorID": 3,
      "ReadAt": "2025-07-02 14:30:25",
      "Value": 48.7,
      "Status": "Good"
    }
  ]
}
```

**Alarm Messages**:
```json
{
    "message":"MCU MCU001 set to faulty state",
    "severity":2,
    "source":"MCU001",
    "state":"raised",
    "timestamp":"2025-07-02 14:35:45"
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