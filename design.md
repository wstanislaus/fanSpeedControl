# High level design of Temperature-Fan Controller

## Hardware Layout

Below described hardware layout assumed in solving the problem.

![HW Layout](images/HWdesign.drawio.svg)

### HW Details

* Multiple MCU's, each MCU consists of one or more temperature sensor. 
  * Example HW
    * MCU1: Connected to 3 temperature sensors
    * MCU2: Connected to 2 temperature sensors
    * MCU3: Connected to 1 temperature sensor
* Fan Control System:
  * Multiple Fan Controllers, each Fan Controller consists of one or more fans connected.
    * Fan Controlers are connected using I2C interface.
    * Fan(s) within a Fan Controller spins at the same rate
    * Fan speed is set at the particular register in the Fan controller in PWM Counts.
    * We might have different models of Fan Controller, which means different PWM Counts to represent any given percentage of duty cycle.
  * Example HW
    * Outlet Fans (Rear): 4 fans model (F4ModelOUT) arranged in a row
    * Inlet Fans (Front and both sides):
      * 4 fans model (F4ModelIN)
      * 2 fans model (F2ModelIN)
* Network Communication:
  * All MCUs communicate with a central switch via TCP/IP
  * The switch acts as a central hub for communication between MCUs
  * Fan Control unit communicates with fan controller's via I2C interface

## Software Architecture

Docker container used to simulate example hardware

![SW Architecture](images/SWDesign.drawio.svg)

### SW Components

* MCU Simulator:
  * MCU base class and create multiple instance of MCUs based on the configuration.
  * Each MCU can have one or more temperature sensors, given in configuration
  * Sensors get read every one second
  * Publishes temperature reading using MQTT messaging schema for topic: sensors/<MCUName>/temperature, for example

    ```JSON
    {
        "MCU": "MCU001",
        "NoOfTempSensors": 3,
        "MsgTimestamp": "2025-06-20 04:06:21",
        "SensorData": [
            { "SensorID": 1, "ReadAt": "2025-06-20 04:06:21", "Value": 24.34, "Status": "Good" },
            { "SensorID": 2, "ReadAt": "2025-06-20 04:06:21", "Value": 23.11, "Status": "Bad" },
            { "SensorID": 3, "ReadAt": "2025-06-20 04:06:21", "Value": 25.50, "Status": "Good" }
        ]
    }
    ```

    * MQTT publish frequency dynamically changed based on the last 5 readings, when the temperature raises, it sends more frequently while temperature drops, it sends less frequently.
      * less than 10C reading is bad
      * between 10-25C - 10 seconds
      * 25-40C - 7 seconds
      * 40-50C - 5 seconds
      * 50-60C - 3 seconds
      * 60-70C - 2 seconds
      * Greater than 70C - every second
    * Supports RPC server to accept messages from CLI for debugging and testing
      * CLI can make a MCU disappear or make a temperature sensor bad or it can make the temperature reading noisy, meaning random
      * CLI can enable/disable console print for logs. Set log levels for console print.
    * All log messages are sent over MQTT.
    * MCU Simulator configuration file example
  
    ```YAML
    # Global MCU Simulator Configuration
    MaxMCUsSupported: 10
    MaxTempSensorsPerMCU: 4
    SimulatorStartTemp: 25.0
    SimulatorEndTemp: 80.0
    SimulatorTempStepSize: 0.3

    # Current Simulation Settings
    MCUs:
        MCU001:
            NumberOfSensors: 3
            Sensors:
                Sensor1:
                    Interface: I2C
                    Address: 0x4A 
                Sensor2:
                    Interface: I2C
                    Address: 0x5C
                Sensor3:
                    Interface: SPI
                    CSLine: 0

        MCU002:
            NumberOfSensors: 2
            Sensors:
                Sensor1:
                    Interface: I2C
                    Address: 0x4B
                Sensor2:
                    Interface: I2C
                    Address: 0x5A

        MCU003:
            NumberOfSensors: 1
            Sensors:
                Sensor1:
                    Interface: I2C
                    Address: 0x44
    ```

* Fan Controller Simulator:
  * Fan controller base class and create multiple instances of fan controller based on the fan model described in configuration
  * Configuration contains supported Fan models and Datasheet for PWM counts to duty cycle percentage and vice versa conversion
    * Number of Fans per model
    * PWM counts (16-bit value) to duty cycle percentage. For example PWM counts (0-1500) to duty cycle percentage (0-100%)
  * Fan Controller Simulator configuration file example

    ```YAML
    # Global Simulator Settings
    MaxFanControllers: 10
    FansTooLoudAlarm: 1  # Raise alarm after 30 mins of continuous noise above 50 dB

    # Supported Fan Models and Datasheet Mappings
    FanModels:
        F4ModelOUT:
            NumberOfFans: 4
            PWMRange:
                Min: 100
                Max: 2000
            DutyCycleRange:
                Min: 10
                Max: 100
            Interface: I2C
            PWM_REG: 0x10
            NoiseProfile:
                # Duty cycle (%) to noise level (dB)
                - DutyCycle: 10
                    NoiseLevel_dB: 25
                - DutyCycle: 25
                    NoiseLevel_dB: 32
                - DutyCycle: 50
                    NoiseLevel_dB: 43
                - DutyCycle: 75
                    NoiseLevel_dB: 57
                - DutyCycle: 100
                    NoiseLevel_dB: 65

        F4ModelIN:
            NumberOfFans: 4
            PWMRange:
                Min: 0
                Max: 1000
            DutyCycleRange:
                Min: 0
                Max: 100
            Interface: I2C
            PWM_REG: 0x10
            NoiseProfile:
                - DutyCycle: 0
                    NoiseLevel_dB: 20
                - DutyCycle: 25
                    NoiseLevel_dB: 28
                - DutyCycle: 50
                    NoiseLevel_dB: 35
                - DutyCycle: 75
                    NoiseLevel_dB: 42
                - DutyCycle: 100
                    NoiseLevel_dB: 48

        F2ModelIN:
            NumberOfFans: 2
            PWMRange:
                Min: 0
                Max: 1000
            DutyCycleRange:
                Min: 0
                Max: 100
            Interface: I2C
            PWM_REG: 0x1A
            NoiseProfile:
                - DutyCycle: 0
                    NoiseLevel_dB: 20
                - DutyCycle: 25
                    NoiseLevel_dB: 28
                - DutyCycle: 50
                    NoiseLevel_dB: 35
                - DutyCycle: 75
                    NoiseLevel_dB: 42
                - DutyCycle: 100
                    NoiseLevel_dB: 48

    # Fan Controller Instances
    FanControllers:
        Fan001:
            Model: F4ModelOUT
            Mode: Manual
            SetDutyCyclePercent: 10
            I2CAddress: 0x4A

        Fan002:
            Model: F4ModelIN
            Mode: Manual
            SetDutyCyclePercent: 5
            I2CAddress: 0x4C

        Fan003:
            Model: F2ModelIN
            Mode: Manual
            SetDutyCyclePercent: 5
            I2CAddress: 0x50

        Fan004:
            Model: F2ModelIN
            Mode: Manual
            SetDutyCyclePercent: 5
            I2CAddress: 0x55
    ```

    * Supports RPC server to accept messages from CLI for debugging and testing

* Temperature Monitoring and Cooling:
  * Subscribes to MCU temperature MQTT messages
  * Understands the overall system temperature
  * Controls the fan duty cycle
  * Supports RPC server to accept messages from CLI for debugging and testing
  * Configuration example:

    ```YAML
    # Temperature Monitoring Configuration
    TemperatureHistoryDurationMinutes: 10
    TemperatureMonitor:
        MinTemp: 25.0
        MaxTemp: 75.0
        MinDutyCycle: 20
        MaxDutyCycle: 100
        UpdateIntervalMs: 2000  # Update every 2 seconds
    ```

* Log Manager:
  * All the modules or subsystem sends the log using MQTT infrastructure to centralized log manager.
  * Log Manager subscribes to this Topic and listens.
  * Log Manager write the logs into log file in JSON format which can be later used for log analysis with filtering support (lnav-Logfile Navigator for log analysis)
  * Configuration example:

    ```YAML
    # Logging Configuration
    Logging:
        Level: INFO
        FilePath: "/var/log/fan_control_system"
        FileName: "fan_control_system.log"
        MaxFileSizeMB: 10
        MaxFiles: 5

    AppLogLevel:
        MCUSimulator: INFO
        FanControlSystem:
            FanSimulator: INFO
            TempMonitor: INFO
            AlarmManager: INFO
            LogManager: INFO
    ```
  
* Alarm Manager:
  * Alarm manager subscribes to Critical Events Topic and takes appropriate actions.
  * For example, if all the MCUs or Fan Controller fails, it shuts down the system.
  * Policies and Actions can be defined for any alarm events.
  * Enhanced alarm management with severity-based actions and runtime database
  * Configuration example:

    ```YAML
    # Alarm Configuration
    Alarms:
        AlarmHistory: 100
        SeverityActions:
            INFO: ["LogEvent", "SendNotification"]
            WARNING: ["LogEvent", "SendNotification", "IncreaseFanSpeed"]
            ERROR: ["LogEvent", "SendNotification", "IncreaseFanSpeed", "SendEmail"]
            CRITICAL: ["LogEvent", "SendNotification", "MaxFanSpeed", "SendEmail", "EmergencyShutdown"]

    AlarmTriggers:
        CriticalTemp: 80.0
        SensorFailureThreshold: 3
        FanFailureThreshold: 2
    ```
  
* Command Line Interface (CLI) Application:
  * We will have a CLI interface to debug and test the system.
  * CLI uses RPC calls to interact with subsystem
  * Enhanced CLI with service selection and context-aware prompts

## Communication Architecture

### RPC Server Configuration

The system uses gRPC for inter-service communication with the following server configuration:

```YAML
# RPC Server Configuration
RPCServers:
    MCUSimulator:
        Port: 50051
        MaxConnections: 10
    FanControlSystem:
        Port: 50052
        MaxConnections: 10
    CLI:
        Port: 50053
        MaxConnections: 5
```

### MQTT Communication

The system uses MQTT (Message Queuing Telemetry Transport) for real-time communication between components. All MQTT messages use JSON format with standardized timestamp formatting.

#### Timestamp Format

All timestamps in MQTT JSON messages use the **ISO 8601-like format**: `"YYYY-MM-DD HH:MM:SS"`

**Examples**:
- `"2025-06-20 04:06:21"`
- `"2025-01-15 14:30:25"`
- `"2024-12-31 23:59:59"`

#### MQTT Configuration

```YAML
# MQTT Settings
MQTTSettings:
    Broker: localhost
    Port: 1883
    KeepAlive: 60
    QoS: 0
    Retain: false
```

#### MQTT Topics and Message Formats

The system publishes data to various MQTT topics for monitoring and debugging purposes:

##### 1. Temperature Sensor Data
**Topic Pattern**: `sensors/{MCU_NAME}/temperature`

**Description**: Real-time temperature readings from MCU sensors with dynamic publish intervals based on temperature ranges.

**Example Topics**:
- `sensors/MCU001/temperature`
- `sensors/MCU002/temperature`
- `sensors/MCU003/temperature`

**Message Format**:
```json
{
  "MCU": "MCU001",
  "NoOfTempSensors": 3,
  "MsgTimestamp": "2025-06-20 04:06:21",
  "SensorData": [
    {
      "SensorID": 1,
      "ReadAt": "2025-06-20 04:06:21",
      "Value": 43.3,
      "Status": "Good"
    },
    {
      "SensorID": 2,
      "ReadAt": "2025-06-20 04:06:21",
      "Value": 46.6,
      "Status": "Good"
    },
    {
      "SensorID": 3,
      "ReadAt": "2025-06-20 04:06:21",
      "Value": 48.7,
      "Status": "Good"
    }
  ]
}
```

##### 2. Log Messages
**Topic Pattern**: `logs/{COMPONENT_NAME}`

**Description**: Centralized logging system where all components publish their log messages.

**Example Topics**:
- `logs/MCU001`
- `logs/FanSimulator`
- `logs/TempMonitor`
- `logs/AlarmManager`

**Message Format**:
```json
{
  "timestamp": "2025-06-20 04:06:21",
  "level": "INFO",
  "component": "MCU001",
  "message": "Temperature reading published successfully"
}
```

##### 3. Alarm Messages
**Topic Pattern**: `alarms/{COMPONENT_NAME}/raise` and `alarms/{COMPONENT_NAME}/clear`

**Description**: Alarm system messages for raising and clearing alarms.

**Example Topics**:
- `alarms/MCU001/raise`
- `alarms/FanSimulator/clear`

**Message Format**:
```json
{
  "timestamp": "2025-06-20 04:06:21",
  "severity": 2,
  "source": "MCU001",
  "message": "Temperature sensor failure detected",
  "state": "raised"
}
```

##### 4. Fan Status Messages
**Topic Pattern**: `fan/{FAN_NAME}/config` and `fan/{FAN_NAME}/status`

**Description**: Fan configuration and status updates.

**Example Topics**:
- `fan/Fan001/config`
- `fan/Fan001/status`

**Message Format**:
```json
{
  "name": "Fan001",
  "model": "F4ModelOUT",
  "i2c_address": 74,
  "pwm_reg": 16,
  "status": "online",
  "timestamp": "2025-06-20 04:06:21"
}
```

### Temperature Monitoring Settings

```YAML
# Temperature Monitoring Settings
TemperatureSettings:
    BadThreshold: 10.0  # Temperature below this is considered bad
    ErraticThreshold: 5.0  # Standard deviation threshold for erratic readings
    PublishIntervals:
        - Range: [0, 25]  # 0-25C
          Interval: 10    # 10 seconds
        - Range: [25, 40] # 25-40C
          Interval: 7     # 7 seconds
        - Range: [40, 50] # 40-50C
          Interval: 5     # 5 seconds
        - Range: [50, 60] # 50-60C
          Interval: 3     # 3 seconds
        - Range: [60, 70] # 60-70C
          Interval: 2     # 2 seconds
        - Range: [70, 999] # >70C
          Interval: 1     # 1 second
```

## Enhanced Features

### Advanced Alarm Management

The alarm system now includes:

1. **Severity-Based Actions**: Configurable actions for different alarm severity levels
2. **Runtime Database**: In-memory storage of alarm history with configurable retention
3. **Alarm Statistics**: Comprehensive statistics including counts, time windows, and severity distributions
4. **Action Callbacks**: Registerable callback functions for custom alarm responses
5. **MQTT Integration**: Automatic alarm publishing and subscription to external alarm events

### Enhanced CLI Interface

The CLI now provides:

1. **Service Selection**: Choose between MCU Simulator and Fan Control System
2. **Context-Aware Prompts**: Each service has its own prompt (mcu> or fan>)
3. **Automatic Connection**: Automatically connects to the appropriate RPC server
4. **Easy Navigation**: Use 'exit' to return to main menu, 'quit' to exit application
5. **Service Isolation**: Each service maintains its own connection and state
6. **Enhanced Alarm Management**: New alarm statistics and history clearing capabilities

### Improved Configuration Management

The configuration system now supports:

1. **Centralized Configuration**: Single YAML file for all system settings
2. **RPC Server Management**: Configurable ports and connection limits for each service
3. **Granular Logging**: Component-specific log levels
4. **Temperature Simulation**: Configurable temperature simulation parameters
5. **Fan Model Definitions**: Detailed fan model specifications with noise profiles
6. **Alarm Triggers**: Configurable thresholds for automatic alarm generation

### Subsystem Communication Patterns

1. **MQTT for Real-time Data**: Temperature readings, logs, and alarms
2. **gRPC for Control Operations**: CLI commands, status queries, and configuration changes
3. **Centralized Logging**: All components publish logs to a central log manager
4. **Alarm Propagation**: Alarms are published via MQTT and processed by the alarm manager
5. **Configuration Distribution**: Configuration is loaded centrally and distributed to components
