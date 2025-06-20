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
mcu> get_temp MCU001 1
Temperature: 43.3°C

mcu> get_temp MCU001 2
Temperature: 46.6°C

mcu> get_temp MCU001 3
Temperature: 48.7°C
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
