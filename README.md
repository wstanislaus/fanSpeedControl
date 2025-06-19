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
        system logging infrastructure 
        alarm monitoring
- Comprehensive logging and monitoring capabilities


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
- `GetTemperatureStatus`: Get current temperature readings from all sensors
- `GetTemperatureHistory`: Retrieve historical temperature data
- `SetTemperatureThresholds`: Configure temperature thresholds for fan control
- `GetCoolingStatus`: Get overall cooling system status

#### Alarm Manager Operations:
- `GetAlarmStatus`: Display status of all configured alarms
- `RaiseAlarm`: Manually trigger an alarm for testing
- `GetAlarmHistory`: Retrieve alarm history
- `EnableAlarm`/`DisableAlarm`: Control alarm enablement

#### Log Manager Operations:
- `GetLogStatus`: Get current logging configuration and status
- `GetRecentLogs`: Retrieve recent log entries with filtering
- `SetLogLevel`: Change logging level dynamically
- `RotateLogFile`: Manually trigger log file rotation


## Configuration

The system uses YAML configuration files for:
- Fan models and controllers
- MCU and sensor configurations
- Alarm definitions and actions
- Logging settings
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

## Building the Project

1. Clone the repository:
```bash
git clone https://github.com/wstanislaus/fanSpeedControl.git
cd fanSpeedControl
```

2. Create and activate a virtual environment (optional but recommended to use python scripts):
```bash
python3 -m venv venv
source venv/bin/activate
pip install -r scripts/requirements.txt
```

3. Build the project:
```bash
make clean && make
```

## Configuration

The system is configured through global YAML configuration file:

- `config/config.yaml`: MCU and temperature sensor configuration
- `config/config.yaml`: Fan controller and fan model configuration

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
```bash
# Start MCU Simulator, the application is placed under /usr/local/bin as part of "make install" from run_container.sh script
mcu_simulator

# Start Fan Control System, the application is placed under /usr/local/bin as part of "make install" from run_container.sh script
fan_control_system
```

## Testing and Debugging

The system provides comprehensive testing and debugging capabilities through dedicated CLI interfaces for both the MCU Simulator and Fan Control System.

### MCU Simulator Testing and Debugging

The MCU Simulator includes a debug CLI application that provides interactive testing and debugging capabilities:

#### Starting the Debug CLI
```bash
# Start the debug CLI application
debug_cli [optional config.yaml]

```

#### Debug CLI Top-Level Menu
When you start the debug CLI application, you'll see a menu to choose between the two services:

```bash
Fan Control System Debug CLI
============================

Available services:
1. MCU Simulator
2. Fan Control System

Please select a service (1-2): 1
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
  get_temp <mcu_name> <sensor_id>  - Get temperature from a specific sensor
  get_mcu_status [mcu_name]  - Get status of all MCUs or a specific MCU
  set_sim_params <mcu_name> <sensor_id> <start_temp> <end_temp> <step_size>  - Set simulation parameters
  set_mcu_fault <mcu_name> <is_faulty>  - Set MCU fault state (0=normal, 1=faulty)
  set_sensor_fault <mcu_name> <sensor_id> <is_faulty>  - Set sensor fault state (0=normal, 1=faulty)
  set_sensor_noise <mcu_name> <sensor_id> <is_noisy>  - Set sensor noise state (0=normal, 1=noisy)
```

**Temperature Operations:**
```bash
# Get temperature from specific sensor
mcu> get_temp mcu1 sensor1
Temperature: 45.2°C

# Get temperature from all sensors (not implemented in current CLI)
# Use get_mcu_status to see all sensors
```

**MCU Status Operations:**
```bash
# Get status of all MCUs
mcu> get_mcu_status
MCU: mcu1
  Online: Yes
  Active Sensors: 2
  Sensors:
    ID: sensor1
    Active: Yes
    Interface: I2C
    Address: 0x48
    Noisy: No
    ID: sensor2
    Active: Yes
    Interface: I2C
    Address: 0x49
    Noisy: No

# Get status of specific MCU
mcu> get_mcu_status mcu1
MCU: mcu1
  Online: Yes
  Active Sensors: 2
  Sensors:
    ID: sensor1
    Active: Yes
    Interface: I2C
    Address: 0x48
    Noisy: No
```

**Simulation Control:**
```bash
# Set simulation parameters for temperature
mcu> set_sim_params mcu1 sensor1 30.0 80.0 5.0
Simulation parameters set successfully

# View current simulation parameters (not implemented in current CLI)
```

**Fault Injection Testing:**
```bash
# Make an MCU faulty
mcu> set_mcu_fault mcu1 1
MCU mcu1 is now faulty
Current state: FAULTY

# Make a specific sensor faulty
mcu> set_sensor_fault mcu1 sensor1 1
Sensor mcu1:sensor1 is now faulty
Current state: FAULTY

# Add noise to a sensor
mcu> set_sensor_noise mcu1 sensor2 1
Sensor mcu1:sensor2 is now noisy
Current state: NOISY

# Restore MCU to good status
mcu> set_mcu_fault mcu1 0
MCU mcu1 is now normal
Current state: NORMAL

# Restore sensor to good status
mcu> set_sensor_fault mcu1 sensor1 0
Sensor mcu1:sensor1 is now normal
Current state: NORMAL

# Remove noise from sensor
mcu> set_sensor_noise mcu1 sensor2 0
Sensor mcu1:sensor2 is now normal
Current state: NORMAL
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

Please select a service (1-2): 
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
  get_temp_status                     - Get temperature status
  get_temp_history <mcu> <sensor> <count> - Get temperature history
  get_cooling_status                  - Get cooling status
  set_temp_thresholds <low> <high> <min_speed> <max_speed> - Set thresholds
  get_temp_thresholds                 - Get current thresholds

  # Alarm operations
  get_alarm_status                    - Get alarm status
  raise_alarm <name> <message> <severity> - Raise alarm
  get_alarm_history <count>           - Get alarm history
  enable_alarm <name>                 - Enable alarm
  disable_alarm <name>                - Disable alarm

  # Log operations
  get_log_status                      - Get log status
  get_recent_logs <count> [level]     - Get recent logs
  set_log_level <level>               - Set log level
  rotate_log                          - Rotate log file

  # System operations
  get_system_status                   - Get system status
  start_system                        - Start system
  stop_system                         - Stop system
  restart_system                      - Restart system
  help                                - Show this help
  exit                                - Return to main menu
  quit                                - Exit CLI
```

**Fan Simulator Operations:**
```bash
# Get status of all fans
fan> get_fan_status
Fan: fan1 (Model: Noctua NF-A12x25)
  Status: GOOD
  Duty Cycle: 65%
  PWM: 166/255
  Noise Level: 42 dB (QUIET)
  Interface: I2C (0x60)

# Get status of specific fan
fan> get_fan_status fan1
Fan: fan1 (Model: Noctua NF-A12x25)
  Status: GOOD
  Duty Cycle: 65%
  PWM: 166/255
  Noise Level: 42 dB (QUIET)
  Interface: I2C (0x60)
  PWM Range: 51-255
  Duty Cycle Range: 20-100%

# Set fan speed by duty cycle
fan> set_fan_speed fan1 75
Fan fan1 speed set to 75% (PWM: 192/255)

# Set speed for all fans
fan> set_fan_speed_all 80
All fans set to 80% duty cycle

# Set specific PWM count
fan> set_fan_pwm fan1 200
Fan fan1 PWM set to 200/255 (78% duty cycle)

# Make a fan controller bad
fan> make_fan_bad fan1
Fan fan1 is now faulty

# Restore fan to good status
fan> make_fan_good fan1
Fan fan1 is now good

# Check noise levels
fan> get_fan_noise fan1
Noise Level: 42 dB (QUIET)
Threshold: 60 dB
Status: Below threshold
```

**Temperature Monitor Operations:**
```bash
# Get current temperature status
fan> get_temp_status
MCU: mcu1 (ONLINE)
  Sensor 1: 45.2°C (GOOD) - 2024-01-15 14:30:25
  Sensor 2: 42.8°C (GOOD) - 2024-01-15 14:30:25

MCU: mcu2 (ONLINE)
  Sensor 1: 38.5°C (GOOD) - 2024-01-15 14:30:25

# Get temperature history
fan> get_temp_history mcu1 sensor1 10
Temperature history for mcu1:sensor1 (last 10 readings):
  14:30:25 - 45.2°C (GOOD)
  14:30:20 - 45.1°C (GOOD)
  14:30:15 - 45.0°C (GOOD)
  ...

# Get cooling status
fan> get_cooling_status
Average Temperature: 42.2°C
Current Fan Speed: 65%
Cooling Mode: AUTO
Cooling Adequate: Yes
Active Alarms: None

# Set temperature thresholds
fan> set_temp_thresholds 20.0 70.0 15 90
Temperature thresholds updated:
  Low: 20.0°C (Fan speed: 15%)
  High: 70.0°C (Fan speed: 90%)

# View current thresholds
fan> get_temp_thresholds
Current thresholds:
  Low: 20.0°C (Fan speed: 15%)
  High: 70.0°C (Fan speed: 90%)
```

**Alarm Manager Operations:**
```bash
# Get alarm status
fan> get_alarm_status
Alarm: high_temperature
  Severity: WARNING
  Enabled: Yes
  Active: No
  Last Triggered: Never
  Trigger Count: 0

Alarm: fan_failure
  Severity: ERROR
  Enabled: Yes
  Active: No
  Last Triggered: Never
  Trigger Count: 0

# Raise a test alarm
fan> raise_alarm high_temperature "Test alarm message" WARNING
Alarm 'high_temperature' raised successfully
Alarm ID: alarm_20240115_143025_001

# Get alarm history
fan> get_alarm_history 20
Alarm history (last 20 entries):
  14:30:25 - high_temperature (WARNING): Test alarm message
  Actions executed: log_alarm, send_notification

# Enable/disable alarms
fan> enable_alarm high_temperature
Alarm 'high_temperature' enabled

fan> disable_alarm fan_failure
Alarm 'fan_failure' disabled
```

**Log Manager Operations:**
```bash
# Get log status
fan> get_log_status
Current Log File: /var/log/fan_control_20240115.log
Log Size: 1.2 MB / 10 MB
Max Files: 5
Log Level: INFO
Pending Entries: 0
Status: Running

# Get recent logs
fan> get_recent_logs 20 WARNING
Recent logs (WARNING and above, last 20 entries):
  14:30:25 [WARNING] Temperature approaching threshold: 45.2°C
  14:30:20 [INFO] Fan speed adjusted to 65%
  14:30:15 [INFO] Temperature reading received: 45.1°C

# Set log level
fan> set_log_level DEBUG
Log level changed from INFO to DEBUG

# Rotate log file
fan> rotate_log
Log file rotated successfully
New file: /var/log/fan_control_20240115_143025.log
```

**System-wide Operations:**
```bash
# Get overall system status
fan> get_system_status
System Status: RUNNING
Health: HEALTHY
Uptime: 2h 15m 30s

Components:
  Fan Simulator: RUNNING (OK)
  Temperature Monitor: RUNNING (OK)
  Alarm Manager: RUNNING (OK)
  Log Manager: RUNNING (OK)

Active Alarms: None

# System control
fan> stop_system
System stopped successfully

fan> start_system
System started successfully

fan> restart_system
System restarted successfully
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

Please select a service (1-2): 
```

**Exiting the Application:**
```bash
Please select a service (1-2): quit
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
│   └── 2. Fan Control System → fan> prompt
│       ├── Commands for fan control operations
│       └── exit → returns to main menu
└── quit → exits application
```

**Key Features:**
- **Service Selection**: Choose between MCU Simulator and Fan Control System
- **Context-Aware Prompts**: Each service has its own prompt (mcu> or fan>)
- **Automatic Connection**: Automatically connects to the appropriate RPC server
- **Easy Navigation**: Use 'exit' to return to main menu, 'quit' to exit application
- **Service Isolation**: Each service maintains its own connection and state
