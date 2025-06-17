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

## System Requirements

- C++11/C++14 compliant compiler
- IEEE-754 compliant floating-point unit
- Docker (for running the simulation environment)
- MQTT broker for communication
- CMake 3.10 or higher

## Building the Project

1. Clone the repository:
```bash
git clone https://github.com/wstanislaus/fanSpeedControl.git
cd fanSpeedControl
```

2. Create and activate a virtual environment (optional but recommended to use python scripts):
```bash
python3 -m venv venv
source venv/bin/activate  # On Linux
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

### Fan Control Algorithm

The system implements the following control logic:
- Below 25°C: Fans run at 20% duty cycle
- Above 75°C: Fans run at 100% duty cycle
- Between 25°C and 75°C: Linear interpolation between 20% and 100% duty cycle

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
