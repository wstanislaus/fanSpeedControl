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
  * Publishes temperture reading using MQTT messaging schema for topic: sensors/<MCUName>/temperature, for example

    ```JSON
    {
        "MCU": "MCU001",
        "NoOfTempSensors": 3,
        "MsgTimestamp":  "2025-06-13T08:25:50Z"
        "SensorData": [
            { "SensorID": 1, "ReadAt": "2025-06-13T08:25:30Z", "Value": 24.34, "Status": "Good" },
            { "SensorID": 2, "ReadAt": "2025-06-12T04:02:00Z", "Value": 23.11, "Status": "Bad" },
            { "SensorID": 3, "ReadAt": "2025-06-13T08:25:30Z", "Value": 25.50, "Status": "Good" }
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
      * Great than 70C - every second
    * Supports RPC server to accept messages from CLI for debugging and testing
      * CLI can make a MCU disapear or make a temperature sensor bad or it can make the temperature reading noisy, meaning random
      * CLI can enable/disable console print for logs. Set log levels for console print.
    * All log messages are send over MQTT.
    * MCU Simulator configuration file example
  
    ```YAML
    # Global MCU Simulator Configuration
    MaxMCUsSupported: 10
    MaxTempSensorsPerMCU: 4

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
                    NoiseLevel_dB: 28
                - DutyCycle: 50
                    NoiseLevel_dB: 37
                - DutyCycle: 75
                    NoiseLevel_dB: 45
                - DutyCycle: 100
                    NoiseLevel_dB: 50

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

* Log Manager:
  * All the modules or subsystem sends the log using MQTT infrastructure to centralized log manager.
  * Log Manager subscribes to this Topic and listens.
  * Log Manager write the logs into log file in JSON format which can be later used for log analysis with filtering support (lnav-Logfile Navigator for log analyzsis)
  
* Alarm Manager:
  * Alarm manager Subscribes to Critical Events Topic and takes appropirate actions.
  * For example, if all the MCUs or Fan Controller fails, it shuts down the system.
  * Policies and Actions can be defined for any alarm events.
  
* Command Line Interface (CLI) Application:
  * We will have a CLI interface to debug and test the system.
  * CLI uses RPC calls to interact with subsystem
  
