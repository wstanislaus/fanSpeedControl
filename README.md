# Fan Speed Control System

A sophisticated fan control system designed to manage multiple fans based on temperature readings from various subsystems(MCUs). The system implements an intelligent cooling strategy that balances noise levels with thermal management requirements.

## Overview

This system monitors temperature readings from multiple subsystems and controls fan speeds accordingly. It's designed to operate in environments where noise levels need to be minimized while ensuring electronics remain within safe temperature ranges.

### Key Features

- Configurable number of subsystems and fans
- Intelligent fan speed control based on temperature readings
- Linear interpolation of fan speeds between temperature thresholds
- Support for different fan models with varying PWM characteristics
- Robust error handling and sensor validation
- MQTT-based communication for 
        temperature monitoring
        alarm monitoring
- Comprehensive monitoring capabilities


## Project Structure

```
.
├── build/              # Build output directory
├── config/            # Configuration files
├── docker/            # Docker container Creation and execution files
├── include/           # Header files
├── src/              # Source files
├── scripts/          # Utility scripts
├── images/           # Documentation images
├── CMakeLists.txt    # CMake build configuration
├── Makefile          # Make build configuration
├── design.md         # Detailed system design
└── problem_statement.md  # Original problem statement
```

## Development

### Adding New Fan Models

1. Add the fan model and simulation configuration to `config/config.yaml` FanControllers section
2. Implement the fan model interface in the source code
3. Update the fan controller to support the new model

### Adding New MCUs

1. Add the MCU simulation configuration to `config/config.yaml` MCUs section
2. Configure the temperature sensors for the new MCU
3. The system will automatically detect and monitor the new MCU

## Protobuf Interfaces

### MCU Simulator Interface (`mcu_simulator.proto`)

The MCU Simulator provides a gRPC interface for:
- Getting temperature readings from sensors
- Retrieving MCU status information
- Setting simulation parameters
- Injecting faults for testing

#### Key Services:
- `GetTemperature`: Retrieve current temperature from specific sensors
- `GetMCUStatus`: Get status of all MCUs and their sensors
- `SetSimulationParams`: Configure temperature simulation parameters
- `SetMCUFault`: Make an MCU faulty for testing
- `SetSensorFault`: Make a specific sensor faulty
- `SetSensorNoise`: Add noise to sensor readings

### Fan Control System Interface (`fan_control_system.proto`)

The Fan Control System provides a comprehensive gRPC interface for testing and debugging all system components:

#### Fan Simulator Operations:
- `GetFanStatus`: Display status of all fans (speed, PWM, noise level, health)
- `SetFanSpeed`: Control fan speed by duty cycle (0-100%)
- `MakeFanBad`: Simulate a faulty fan controller
- `MakeFanGood`: Restore a fan to good status
- `SetFanPWM`: Set specific PWM count for a fan
- `GetFanNoiseLevel`: Get current noise level and category

#### Temperature Monitor Operations:
- `GetTemperatureHistory`: Retrieve historical temperature data
- `SetTemperatureThresholds`: Configure temperature thresholds for fan control
- `GetCoolingStatus`: Get overall cooling system status

#### Alarm Manager Operations:
- `GetAlarmStatus`: Display status of all configured alarms
- `RaiseAlarm`: Manually trigger an alarm for testing
- `GetAlarmHistory`: Retrieve alarm history
- `EnableAlarm`/`DisableAlarm`: Control alarm enablement

## Configuration

The system uses YAML configuration files for:
- Fan models and controllers
- MCU and sensor configurations
- Alarm definitions and actions
- MQTT communication settings

## Building and Running

### Prerequisites
- C++11/C++14 compliant compiler
- IEEE-754 compliant floating-point unit
- Docker (for running the simulation environment)
- MQTT broker for communication
- CMake 3.10 or higher
- YAML-CPP
- Mosquitto (MQTT)
- nlohmann/json

## Project Development

1. Clone the repository:
```bash
git clone https://github.com/wstanislaus/fanSpeedControl.git
cd fanSpeedControl
```

## Configuration

The system is configured through global YAML configuration file:

- `config/config.yaml`: MCU and temperature sensor configuration, Fan controller and fan model configuration, Alarm settings, logging settings etc

## Usage

### Running the Simulation

The project includes Docker support for easy setup and running. Follow these steps to run the application in a Docker container:

1. Make sure Docker is installed on your system:
```bash
docker --version
```

2. Navigate to the docker directory and run the container:
```bash
cd docker
./run_container.sh
```

This script will:
- Build the Docker image if it doesn't exist
- Create and start a container with the necessary dependencies
- Mount your local source code into the container
- Build the application inside the container
- Start the MQTT broker
- Provide an interactive shell

The container includes:
- All required build dependencies
- MQTT broker (Mosquitto)
- Python environment for scripts
- Proper user permissions matching your host system

To stop and remove the container:
```bash
./cleanup.sh
```

### Running the System inside docker container
While inside the docker container bash prompt, the applications are started and running in the background.
```bash
developer@557ff1c54359:/app$ ps fax
    PID TTY      STAT   TIME COMMAND
    653 pts/0    Ss     0:00 /bin/bash
    699 pts/0    R+     0:00  \_ ps fax
      1 ?        Ss     0:00 sleep infinity
    610 ?        Ss     0:01 mosquitto -d
    617 ?        Sl     0:02 /usr/local/bin/mcu_simulator
    618 ?        Sl     0:02 /usr/local/bin/fan_control_system
developer@557ff1c54359:/app$
developer@557ff1c54359:/app$ netstat -lnp
Active Internet connections (only servers)
Proto Recv-Q Send-Q Local Address           Foreign Address         State       PID/Program name
tcp        0      0 127.0.0.1:1883          0.0.0.0:*               LISTEN      610/mosquitto
tcp6       0      0 :::50051                :::*                    LISTEN      617/mcu_simulator
tcp6       0      0 :::50052                :::*                    LISTEN      618/fan_control_sys
tcp6       0      0 ::1:1883                :::*                    LISTEN      610/mosquitto
Active UNIX domain sockets (only servers)
Proto RefCnt Flags       Type       State         I-Node   PID/Program name     Path
developer@557ff1c54359:/app$
```
Start the CLI application as described below sections to interact with the application.

## Testing and Debugging

The system provides comprehensive testing and debugging capabilities through dedicated CLI interfaces for both the MCU Simulator and Fan Control System.

### MCU Simulator Testing and Debugging

The MCU Simulator includes a debug CLI application that provides interactive testing and debugging capabilities:

#### Starting the Debug CLI
```bash
# Start the debug CLI application
debug_cli

```

#### Debug CLI Top-Level Menu
When you start the debug CLI application, you'll see a menu to choose between the services:

```bash
Fan Control System Debug CLI
============================

Available services:
1. MCU Simulator
2. Fan Control System
3. Exit

Please select a service (1-3): 1
```

#### MCU Simulator CLI Commands

After selecting "MCU Simulator", you'll be connected to the MCU Simulator service and see the MCU prompt:

```bash
Connected to MCU Simulator service (localhost:50051)
Type 'help' for available commands, 'exit' to return to main menu

mcu> help
Available commands:
  help  - Show this help message
  exit  - Return to main menu
  quit  - Exit the application
  get_temp [mcu_name]  - Get temperatures from all MCUs or a specific MCU
  get_mcu_status [mcu_name]  - Get status of all MCUs or a specific MCU
  set_sim_params <mcu_name> <sensor_id> <start_temp> <end_temp> <step_size>  - Set simulation parameters
  set_mcu_fault <mcu_name> <is_faulty>  - Set MCU fault state (0=normal, 1=faulty)
  set_sensor_fault <mcu_name> <sensor_id> <is_faulty>  - Set sensor fault state (0=normal, 1=faulty)
  set_sensor_noise <mcu_name> <sensor_id> <is_noisy>  - Set sensor noise state (0=normal, 1=noisy)
```

**Temperature Operations:**
```bash
# Get temperatures from all MCUs and all sensors
mcu> get_temp
MCU001:
    Sensor1: 43.3°C (Good)
    Sensor2: 46.6°C (Good)
    Sensor3: 48.7°C (Good)

MCU002:
    Sensor1: 42.1°C (Good)
    Sensor2: 44.2°C (Good)

MCU003:
    Sensor1: 45.5°C (Good)

# Get temperatures from a specific MCU
mcu> get_temp MCU001
MCU001:
    Sensor1: 43.3°C (Good)
    Sensor2: 46.6°C (Good)
    Sensor3: 48.7°C (Good)

# Example with faulty sensor
mcu> get_temp MCU001
MCU001:
    Sensor1: 0.0°C (Bad) - Faulty sensor
    Sensor2: 46.6°C (Good)
    Sensor3: 48.7°C (Good)
```

**MCU Status Operations:**
```bash
# Get status of all MCUs
mcu> get_mcu_status
MCU: MCU001
  Online: Yes
  Active Sensors: 3
  Sensors:
    ID: 1
    Active: Yes
    Interface: I2C
    Address: 0x4A
    Noisy: No
    ID: 2
    Active: Yes
    Interface: I2C
    Address: 0x5C
    Noisy: No
    ID: 3
    Active: Yes
    Interface: SPI
    Address: 0x00
    Noisy: No

MCU: MCU002
  Online: Yes
  Active Sensors: 2
  Sensors:
    ID: 1
    Active: Yes
    Interface: I2C
    Address: 0x4B
    Noisy: No
    ID: 2
    Active: Yes
    Interface: I2C
    Address: 0x5A
    Noisy: No

MCU: MCU003
  Online: Yes
  Active Sensors: 1
  Sensors:
    ID: 1
    Active: Yes
    Interface: I2C
    Address: 0x44
    Noisy: No

# Get status of specific MCU
mcu> get_mcu_status MCU001
MCU: MCU001
  Online: Yes
  Active Sensors: 3
  Sensors:
    ID: 1
    Active: Yes
    Interface: I2C
    Address: 0x4A
    Noisy: No
    ID: 2
    Active: Yes
    Interface: I2C
    Address: 0x5C
    Noisy: No
    ID: 3
    Active: Yes
    Interface: SPI
    Address: 0x00
    Noisy: No
```

**Simulation Control:**
```bash
# Set simulation parameters for temperature
mcu> set_sim_params MCU001 1 30.0 80.0 5.0
Simulation parameters set successfully
```

**Fault Injection Testing:**
```bash
# Make an MCU faulty
mcu> set_mcu_fault MCU001 1
MCU MCU001 is now faulty
Current state: faulty

# Check MCU status when faulty
mcu> get_mcu_status MCU001
MCU: MCU001
  Online: No
  Active Sensors: 0
  Sensors:
    ID: 1
    Active: No
    Interface: I2C
    Address: 0x4A
    Noisy: No
    ID: 2
    Active: No
    Interface: I2C
    Address: 0x5C
    Noisy: No
    ID: 3
    Active: No
    Interface: SPI
    Address: 0x00
    Noisy: No

# Restore MCU to good status
mcu> set_mcu_fault MCU001 0
MCU MCU001 is now normal
Current state: normal

# Add noise to a sensor
mcu> set_sensor_noise MCU003 1 1
Sensor MCU003:1 is now noisy
Current state: noisy

# Remove noise from sensor
mcu> set_sensor_noise MCU003 1 0
Sensor MCU003:1 is now normal
Current state: normal
```

**Returning to Main Menu:**
```bash
mcu> exit
Disconnected from MCU Simulator service

Fan Control System Debug CLI
============================

Available services:
1. MCU Simulator
2. Fan Control System
3. Exit

Please select a service (1-3): 
```

### Fan Control System Testing and Debugging

The Fan Control System provides a comprehensive CLI interface for testing all components:

#### Fan Control System CLI Commands

After selecting "Fan Control System" from the main menu, you'll be connected to the Fan Control System service and see the fan prompt:

```bash
Connected to Fan Control System service (localhost:50052)
Type 'help' for available commands, 'exit' to return to main menu

fan> help
Available commands:
  # Fan operations
  get_fan_status [fan_name]           - Get fan status
  set_fan_speed <fan_name> <duty_cycle> - Set fan speed
  set_fan_speed_all <duty_cycle>      - Set all fan speeds
  set_fan_pwm <fan_name> <pwm_count>  - Set fan PWM
  make_fan_bad <fan_name>             - Make fan faulty
  make_fan_good <fan_name>            - Restore fan
  get_fan_noise <fan_name>            - Get noise level

  # Temperature operations
  get_temp_history <mcu> <sensor> <count> - Get temperature history
  get_cooling_status                  - Get cooling status
  set_temp_thresholds <low> <high> <min_speed> <max_speed> - Set thresholds
  get_temp_thresholds                 - Get current thresholds

  # Alarm operations
  raise_alarm <name> <message> <severity> - Raise alarm
  get_alarm_history <count>           - Get alarm history
  clear_alarm_history [alarm_name]    - Clear alarm history (all if no name)
  get_alarm_statistics [alarm_name] [time_window_hours] - Get alarm statistics

  help                                - Show this help
  exit                                - Return to main menu
  quit                                - Exit CLI
```

**Fan Simulator Operations:**
```bash
# Get status of all fans
fan> get_fan_status
Fan: Fan001
  Model: F4ModelOUT
  Online: Yes
  Duty Cycle: 42
  PWM Count: 775
  Noise Level (dB): 43
  Status: Good
  Interface: I2C
  I2C Address: 0x4a
  PWM Range: 100-2000
  Duty Cycle Range: 10%-100%

Fan: Fan002
  Model: F4ModelIN
  Online: Yes
  Duty Cycle: 42
  PWM Count: 420
  Noise Level (dB): 35
  Status: Good
  Interface: I2C
  I2C Address: 0x4c
  PWM Range: 0-1000
  Duty Cycle Range: 0%-100%

Fan: Fan003
  Model: F2ModelIN
  Online: Yes
  Duty Cycle: 42
  PWM Count: 420
  Noise Level (dB): 35
  Status: Good
  Interface: I2C
  I2C Address: 0x50
  PWM Range: 0-1000
  Duty Cycle Range: 0%-100%

Fan: Fan004
  Model: F2ModelIN
  Online: Yes
  Duty Cycle: 42
  PWM Count: 420
  Noise Level (dB): 35
  Status: Good
  Interface: I2C
  I2C Address: 0x55
  PWM Range: 0-1000
  Duty Cycle Range: 0%-100%

# Make a fan controller bad
fan> make_fan_bad Fan004
Fan Fan004 made bad successfully
Message: Fan made bad successfully

# Restore fan to good status
fan> make_fan_good Fan004
Fan Fan004 made good successfully
Message: Fan made good successfully

# Set speed for all fans
fan> set_fan_speed_all 100
All fan speeds set successfully
Message: Fan speed set successfully for all fans
  Fan: Fan001
    Success: Yes
    Previous duty cycle: 78%
    New duty cycle: 100%
  Fan: Fan002
    Success: Yes
    Previous duty cycle: 78%
    New duty cycle: 100%
  Fan: Fan003
    Success: Yes
    Previous duty cycle: 78%
    New duty cycle: 100%
  Fan: Fan004
    Success: Yes
    Previous duty cycle: 78%
    New duty cycle: 100%
```

**Temperature Monitor Operations:**
```bash
# Get temperature history
fan> get_temp_history MCU001 1 10
Temperature History for MCU001:1
Total readings: 10

Timestamp: 2025-06-20 04:06:21
  Temperature: 29.5°C
  Status: Good

Timestamp: 2025-06-20 04:06:28
  Temperature: 31.6°C
  Status: Good

Timestamp: 2025-06-20 04:06:35
  Temperature: 33.7°C
  Status: Good

Timestamp: 2025-06-20 04:06:42
  Temperature: 35.8°C
  Status: Good

Timestamp: 2025-06-20 04:06:49
  Temperature: 37.9°C
  Status: Good

Timestamp: 2025-06-20 04:06:56
  Temperature: 40°C
  Status: Good

Timestamp: 2025-06-20 04:07:01
  Temperature: 41.5°C
  Status: Good

Timestamp: 2025-06-20 04:07:06
  Temperature: 43°C
  Status: Good

Timestamp: 2025-06-20 04:07:11
  Temperature: 44.8°C
  Status: Good

Timestamp: 2025-06-20 04:07:16
  Temperature: 46.3°C
  Status: Good

# Get cooling status
fan> get_cooling_status
Cooling Status:
  Average Temperature: 66.7°C
  Current Fan Speed: 86%
  Cooling Mode: MANUAL
```

**Alarm Manager Operations:**
```bash
# Get alarm history
fan> get_alarm_history 10
Alarm History
Total entries: 10

Timestamp: 2025-06-20 04:08:08
  Alarm: MCU001
  Message: MCU MCU001 set to faulty state
  Severity: 2

Timestamp: 2025-06-20 04:10:18
  Alarm: MCU001
  Message: MCU MCU001 set to faulty state
  Severity: 2

Timestamp: 2025-06-20 04:11:11
  Alarm: MCU003
  Message: MCU MCU003 Sensor 1 set to noisy mode
  Severity: 0

Timestamp: 2025-06-20 04:11:11
  Alarm: MCU003
  Message: MCU MCU003 Sensor 1 showing erratic readings
  Severity: 2

# Get alarm statistics
fan> get_alarm_statistics
Alarm Statistics (time window: 24 hours):
Total statistics entries: 2

Alarm: MCU001
  Total Count: 2
  Active Count: 2
  Acknowledged Count: 0
  First Occurrence: 2025-06-20 04:08:08
  Last Occurrence: 2025-06-20 04:10:18
  Severity Breakdown:
    ERROR: 2

Alarm: MCU003
  Total Count: 29
  Active Count: 29
  Acknowledged Count: 0
  First Occurrence: 2025-06-20 04:11:11
  Last Occurrence: 2025-06-20 04:11:37
  Severity Breakdown:
    INFO: 2
    ERROR: 27

# Clear alarm history
fan> clear_alarm_history
Alarm history cleared successfully
Cleared entries: 31
Message: Alarm history cleared successfully
```

**Returning to Main Menu:**
```bash
fan> exit
Disconnected from Fan Control System service

Fan Control System Debug CLI
============================

Available services:
1. MCU Simulator
2. Fan Control System
3. Exit

Please select a service (1-3): 
```

**Exiting the Application:**
```bash
Please select a service (1-3): 3
Goodbye!
```

### CLI Navigation Flow

The debug CLI application provides a hierarchical navigation structure:

```
debug_cli
├── Main Menu
│   ├── 1. MCU Simulator → mcu> prompt
│   │   ├── Commands for MCU operations
│   │   └── exit → returns to main menu
│   ├── 2. Fan Control System → fan> prompt
│   │   ├── Commands for fan control operations
│   │   └── exit → returns to main menu
│   └── 3. Exit → exits application
```

**Key Features:**
- **Service Selection**: Choose between MCU Simulator and Fan Control System
- **Context-Aware Prompts**: Each service has its own prompt (mcu> or fan>)
- **Automatic Connection**: Automatically connects to the appropriate RPC server
- **Easy Navigation**: Use 'exit' to return to main menu, 'quit' to exit application
- **Service Isolation**: Each service maintains its own connection and state
- **Enhanced Alarm Management**: New alarm statistics and history clearing capabilities

## MQTT Communication

The system uses MQTT (Message Queuing Telemetry Transport) for real-time communication between components. All MQTT messages use JSON format with standardized timestamp formatting.

### Timestamp Format

All timestamps in MQTT JSON messages use the **ISO 8601-like format**: `"YYYY-MM-DD HH:MM:SS"`

**Examples**:
- `"2025-06-20 04:06:21"`
- `"2025-01-15 14:30:25"`
- `"2024-12-31 23:59:59"`

This format is consistent across all system components and provides human-readable timestamps for easy debugging and monitoring.

### MQTT Topics and Message Formats

The system publishes data to various MQTT topics for monitoring and debugging purposes:

#### 1. Temperature Sensor Data
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

**Publish Intervals**:
- 0-25°C: 10 seconds
- 25-40°C: 7 seconds
- 40-50°C: 5 seconds
- 50-60°C: 3 seconds
- 60-70°C: 2 seconds
- >70°C: 1 second

**Debug Command**:
```bash
# Monitor all temperature data
mosquitto_sub -h localhost -t 'sensors/+/temperature' -F "%t => %p"

# Monitor specific MCU
mosquitto_sub -h localhost -t 'sensors/MCU001/temperature' -F "%t => %p"
```

#### 2. Fan Status and Configuration
**Topic Pattern**: `fan/{FAN_NAME}/{config|status}`

**Description**: Fan configuration and real-time status updates including PWM count, duty cycle, and noise levels.

**Example Topics**:
- `fan/Fan001/config` - Initial fan configuration
- `fan/Fan001/status` - Real-time fan status
- `fan/Fan002/config`
- `fan/Fan002/status`
- `fan/Fan003/config`
- `fan/Fan003/status`
- `fan/Fan004/config`
- `fan/Fan004/status`

**Config Message Format**:
```json
{
  "name": "Fan001",
  "model": "F4ModelOUT",
  "i2c_address": "0x4a",
  "pwm_reg": "0x10",
  "status": "Good",
  "timestamp": "2025-06-20 04:06:21"
}
```

**Status Message Format**:
```json
{
  "name": "Fan001",
  "model": "F4ModelOUT",
  "status": "Good",
  "pwm_count": 775,
  "duty_cycle": 42,
  "i2c_address": "0x4a",
  "pwm_reg": "0x10",
  "timestamp": "2025-06-20 04:06:21"
}
```

**Debug Commands**:
```bash
# Monitor all fan configurations
mosquitto_sub -h localhost -t 'fan/+/config' -F "%t => %p"

# Monitor all fan status updates
mosquitto_sub -h localhost -t 'fan/+/status' -F "%t => %p"

# Monitor specific fan
mosquitto_sub -h localhost -t 'fan/Fan001/#' -F "%t => %p"
```

#### 3. Temperature Monitor
**Topic Pattern**: `temp_monitor/{config|cooling_status}`

**Description**: Temperature monitoring system configuration and cooling status updates.

**Topics**:
- `temp_monitor/config` - Temperature monitor configuration
- `temp_monitor/cooling_status` - Real-time cooling system status

**Config Message Format**:
```json
{
  "status": "initialized",
  "mcu_count": 3,
  "temp_threshold_low": 25.0,
  "temp_threshold_high": 75.0,
  "fan_speed_min": 20,
  "fan_speed_max": 100,
  "history_duration_minutes": 10,
  "std_dev_threshold": 5.0,
  "timestamp": "2025-06-20 04:06:21"
}
```

**Cooling Status Message Format**:
```json
{
  "cooling_mode": "MANUAL",
  "average_temperature": 66.7,
  "current_fan_speed": 86,
  "timestamp": "2025-06-20 04:06:21"
}
```

**Debug Commands**:
```bash
# Monitor temperature monitor configuration
mosquitto_sub -h localhost -t 'temp_monitor/config' -F "%t => %p"

# Monitor cooling status
mosquitto_sub -h localhost -t 'temp_monitor/cooling_status' -F "%t => %p"

# Monitor all temperature monitor topics
mosquitto_sub -h localhost -t 'temp_monitor/#' -F "%t => %p"
```

#### 4. Alarm System
**Topic Pattern**: `alarms/{ALARM_NAME}/{raise|clear}`

**Description**: Alarm system messages for raising and clearing alarms with severity levels.

**Example Topics**:
- `alarms/MCU001/raise` - MCU001 alarm raised
- `alarms/MCU001/clear` - MCU001 alarm cleared
- `alarms/Fan001/raise` - Fan001 alarm raised
- `alarms/Fan001/clear` - Fan001 alarm cleared

**Raise Message Format**:
```json
{
  "timestamp": "2025-06-20 04:08:08",
  "severity": 2,
  "source": "MCU001",
  "message": "MCU MCU001 set to faulty state",
  "is_clear": false
}
```

**Clear Message Format**:
```json
{
  "timestamp": "2025-06-20 04:10:18",
  "severity": 2,
  "source": "MCU001",
  "message": "MCU MCU001 restored to normal state",
  "is_clear": true
}
```

**Severity Levels**:
- 0: INFO
- 1: WARNING
- 2: ERROR
- 3: CRITICAL

**Debug Commands**:
```bash
# Monitor all alarm messages
mosquitto_sub -h localhost -t 'alarms/#' -F "%t => %p"

# Monitor only alarm raises
mosquitto_sub -h localhost -t 'alarms/+/raise' -F "%t => %p"

# Monitor only alarm clears
mosquitto_sub -h localhost -t 'alarms/+/clear' -F "%t => %p"

# Monitor specific component alarms
mosquitto_sub -h localhost -t 'alarms/MCU001/#' -F "%t => %p"
```

#### 5. Logging System
**Topic Pattern**: `logs/{COMPONENT_NAME}/{debug|info|warning|error}`

**Description**: System-wide logging with different severity levels for debugging and monitoring.

**Example Topics**:
- `logs/MCUSimulator/debug`
- `logs/MCUSimulator/info`
- `logs/MCUSimulator/warning`
- `logs/MCUSimulator/error`
- `logs/FanSimulator/debug`
- `logs/FanSimulator/info`
- `logs/TempMonitor/debug`
- `logs/TempMonitor/info`
- `logs/AlarmManager/debug`
- `logs/AlarmManager/info`
- `logs/LogManager/debug`
- `logs/LogManager/info`

**Message Format**:
```json
{
  "timestamp": "2025-06-20 04:06:21",
  "level": 1,
  "source": "MCUSimulator",
  "message": "MCU MCU001 initialized successfully"
}
```

**Log Levels**:
- 0: DEBUG
- 1: INFO
- 2: WARNING
- 3: ERROR

**Debug Commands**:
```bash
# Monitor all log messages
mosquitto_sub -h localhost -t 'logs/#' -F "%t => %p"

# Monitor only error logs
mosquitto_sub -h localhost -t 'logs/+/error' -F "%t => %p"

# Monitor specific component logs
mosquitto_sub -h localhost -t 'logs/MCUSimulator/#' -F "%t => %p"

# Monitor info and error logs for all components
mosquitto_sub -h localhost -t 'logs/+/info' -t 'logs/+/error' -F "%t => %p"
```

### Advanced MQTT Debugging Techniques

#### 1. Wildcard Subscriptions
Use MQTT wildcards to monitor multiple topics:

```bash
# Monitor all system topics
mosquitto_sub -h localhost -t '#' -F "%t => %p"

# Monitor all sensor and fan data
mosquitto_sub -h localhost -t 'sensors/#' -t 'fan/#' -F "%t => %p"

# Monitor all alarms and logs
mosquitto_sub -h localhost -t 'alarms/#' -t 'logs/#' -F "%t => %p"
```

#### 2. Topic Filtering with Format
Use custom formats to make output more readable:

```bash
# Show timestamp with topic and payload
mosquitto_sub -h localhost -t 'sensors/+/temperature' -F "%U => %t => %p"

# Show only temperature values
mosquitto_sub -h localhost -t 'sensors/+/temperature' -F "Temp: %p"

# Show fan status in a readable format
mosquitto_sub -h localhost -t 'fan/+/status' -F "Fan Status: %t => %p"
```

#### 3. Real-time System Monitoring
Create a comprehensive monitoring dashboard:

```bash
# Terminal 1: Monitor temperature data
mosquitto_sub -h localhost -t 'sensors/+/temperature' -F "%t => %p"

# Terminal 2: Monitor fan status
mosquitto_sub -h localhost -t 'fan/+/status' -F "%t => %p"

# Terminal 3: Monitor alarms
mosquitto_sub -h localhost -t 'alarms/#' -F "%t => %p"

# Terminal 4: Monitor errors
mosquitto_sub -h localhost -t 'logs/+/error' -F "%t => %p"
```

#### 4. Data Analysis and Filtering
Use command-line tools to analyze MQTT data:

```bash
# Count temperature messages per MCU
mosquitto_sub -h localhost -t 'sensors/+/temperature' | grep -o '"MCU":"[^"]*"' | sort | uniq -c

# Monitor temperature trends
mosquitto_sub -h localhost -t 'sensors/+/temperature' | jq -r '.SensorData[].Value' | tail -20

# Check for high temperatures
mosquitto_sub -h localhost -t 'sensors/+/temperature' | jq -r '.SensorData[] | select(.Value > 70) | "High temp: \(.Value)°C"'
```

#### 5. Performance Monitoring
Monitor system performance and message rates:

```bash
# Count messages per second
mosquitto_sub -h localhost -t '#' | pv -l > /dev/null

# Monitor specific topic message rates
mosquitto_sub -h localhost -t 'sensors/+/temperature' | pv -l > /dev/null
```

### Troubleshooting MQTT Issues

#### 1. Connection Issues
```bash
# Test MQTT broker connectivity
mosquitto_pub -h localhost -t 'test/topic' -m 'test message'
mosquitto_sub -h localhost -t 'test/topic' -C 1
```

#### 2. Message Flow Verification
```bash
# Verify temperature data flow
mosquitto_sub -h localhost -t 'sensors/+/temperature' -C 5

# Verify fan status updates
mosquitto_sub -h localhost -t 'fan/+/status' -C 5

# Verify alarm system
mosquitto_sub -h localhost -t 'alarms/#' -C 5
```

#### 3. Debugging Specific Components
```bash
# Debug MCU Simulator
mosquitto_sub -h localhost -t 'logs/MCUSimulator/#' -t 'sensors/+/temperature' -F "%t => %p"

# Debug Fan Control System
mosquitto_sub -h localhost -t 'logs/FanSimulator/#' -t 'fan/+/status' -t 'temp_monitor/#' -F "%t => %p"

# Debug Alarm System
mosquitto_sub -h localhost -t 'logs/AlarmManager/#' -t 'alarms/#' -F "%t => %p"
```

### MQTT Topic Summary

| Category        | Topic Pattern                       | Description               | Frequency    |
|-----------------|-------------------------------------|---------------------------|--------------|
| **Temperature** | `sensors/{MCU}/temperature`         | Real-time sensor readings | 1-10 seconds |
| **Fan Status**  | `fan/{FAN}/status`                  | Fan operational status    | Continuous   |
| **Fan Config**  | `fan/{FAN}/config`                  | Fan configuration         | On startup   |
| **Cooling**     | `temp_monitor/cooling_status`       | Cooling system status     | 2 seconds    |
| **Alarms**      | `alarms/{COMPONENT}/{raise\|clear}` | Alarm events | On demand  |
| **Logs**        | `logs/{COMPONENT}/{level}`          | System logs | On events   |

This MQTT debugging system provides comprehensive visibility into all aspects of the Fan Speed Control System, enabling real-time monitoring, debugging, and performance analysis.



