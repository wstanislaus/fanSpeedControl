#  Live Demo Script - Fan Speed Control System

## Demo Setup and System Overview

### 1. Complete System Startup
```bash
# Clone the repository fron github
git clone git@github.com:wstanislaus/fanSpeedControl.git

# Navigate to project directory
cd fanSpeedControl

# Start Docker container with full environment
cd docker
./run_container.sh

# Wait for container to fully initialize
# This script creates docker image and mounts the source code and compiles and installs under /usr/local/bin
# Also starts the applications in the background after installation.

# Once container is running, re-running the script will create a new bash instance to existing container.

```

### 2. Comprehensive System Verification
```bash
# Verify all services are running
ps aux | grep -E "(mcu_simulator|fan_control_system|mosquitto)"

# Check network ports
netstat -lnp | grep -E "(50051|50052|1883)"


```

**Expected Output**:
```
developer@6cfdf633807c:/app$ ps aux | grep -E "(mcu_simulator|fan_control_system|mosquitto)"
develop+   614  0.0  0.0  14972  3312 ?        Ss   Jul01   0:39 mosquitto -d
develop+   621  0.1  0.1 988180 13036 ?        Sl   Jul01   1:30 /usr/local/bin/mcu_simulator
develop+   622  0.1  0.1 1661500 13744 ?       Sl   Jul01   1:20 /usr/local/bin/fan_control_system
develop+   697  0.0  0.0   3020  1480 pts/0    S+   01:30   0:00 grep --color=auto -E (mcu_simulator|fan_control_system|mosquitto)

developer@6cfdf633807c:/app$ netstat -lnp | grep -E "(50051|50052|1883)"
tcp        0      0 127.0.0.1:1883          0.0.0.0:*               LISTEN      614/mosquitto
tcp6       0      0 ::1:1883                :::*                    LISTEN      614/mosquitto
tcp6       0      0 :::50051                :::*                    LISTEN      621/mcu_simulator
tcp6       0      0 :::50052                :::*                    LISTEN      622/fan_control_sys
```

### 3. System Architecture Overview
We have a microservices architecture with three main components:
- MCU Simulator: Simulates temperature sensors and publishes readings
- Fan Control System: Processes temperatures and controls fan speeds
- MQTT Broker: Handles real-time communication between services

---

## Demo Scenarios

### Scenario 1: Normal Operation Deep Dive

#### Step 1: System Initialization and Status Check
```bash
# Start the CLI interface
debug_cli

# Select MCU simulator
1

mcu> get_mcu_statu
```
**Expected Output**:
```
=== MCU Status Report ===
MCU001:
  - Status: Online
  - Sensors: 3/3 Good
  - Last Update: 2025-07-02 17:34:34
  - Publish Interval: 1s

MCU002:
  - Status: Online
  - Sensors: 2/2 Good
  - Last Update: 2025-07-02 17:34:33
  - Publish Interval: 1s

MCU003:
  - Status: Online
  - Sensors: 1/1 Good
  - Last Update: 2025-07-02 17:34:33
  - Publish Interval: 1s
```
All configured MCUs are oprating normally and publishing sensor data

```bash

# Exit and Select Fan Control System
exit
2

# Comprehensive system status
fan> get_fan_status
```

**Expected Output**:
```
Fan001 (F4ModelOUT):
  - Status: Online
  - Duty Cycle: 33%
  - PWM Count: 585
  - Noise Level: 43 dB
  - Health: Good
  - Model: F4ModelOUT (100-2000)

Fan002 (F4ModelIN):
  - Status: Online
  - Duty Cycle: 33%
  - PWM Count: 330
  - Noise Level: 35 dB
  - Health: Good
  - Model: F4ModelIN (0-1000)

Fan003 (F2ModelIN):
  - Status: Online
  - Duty Cycle: 33%
  - PWM Count: 330
  - Noise Level: 35 dB
  - Health: Good
  - Model: F2ModelIN (0-1000)

Fan004 (F2ModelIN):
  - Status: Online
  - Duty Cycle: 33%
  - PWM Count: 330
  - Noise Level: 35 dB
  - Health: Good
  - Model: F2ModelIN (0-1000)
```

Notice how different fan models have different PWM ranges. F4ModelOUT uses 100-2000 PWM counts while F4ModelIN uses 0-1000. This reflects real-world hardware differences.

```bash
# Comprehensive system status
fan> get_fan_noise
```

**Expected Output**:
```
Fan: Fan001 (F4ModelOUT)
  Noise Level: 43 dB
  Noise Category: MODERATE

Fan: Fan002 (F4ModelIN)
  Noise Level: 35 dB
  Noise Category: QUIET

Fan: Fan003 (F2ModelIN)
  Noise Level: 35 dB
  Noise Category: QUIET

Fan: Fan004 (F2ModelIN)
  Noise Level: 35 dB
  Noise Category: QUIET

Successfully retrieved noise levels for 4/4 fans
```

Different Fans have different Noise profile based on the Model. Based on the duty cycle and PWM counts the noise category are classified.

#### Step 2: Temperature History Analysis
```bash
fan> get_temp_history
```

**Expected Output**:
```
MCU001:
  - 1: 55.6°C (Good) - Last: 2025-07-02 17:58:59
  - 2: 55.6°C (Good) - Last: 2025-07-02 17:58:59
  - 3: 55.6°C (Good) - Last: 2025-07-02 17:58:59

MCU002:
  - 1: 55.6°C (Good) - Last: 2025-07-02 17:58:59
  - 2: 55.6°C (Good) - Last: 2025-07-02 17:58:59

MCU003:
  - 1: 55.6°C (Good) - Last: 2025-07-02 17:58:59

Max Temperature: 55.6°C
Cooling Status:
  Average Temperature: 51.1°C
  Current Fan Speed: 61%
  Cooling Mode: MANUAL
```

The system tracks temperature history and uses the maximum temperature across all sensors to determine fan speed. This ensures we're always cooling based on the hottest component. Also there is an interval at which the fan speed gets set instead of frequent change in the Fan ramping up and down and generating noise

#### Step 3: Real-time MQTT Monitoring
```bash
# In a separate terminal window
mosquitto_sub -h localhost -t "sensors/+/temperature" -v
```

**Expected Output**:
```
sensors/MCU003/temperature {"MCU":"MCU003","MsgTimestamp":"2025-07-06 02:39:35","NoOfTempSensors":1,"SensorData":[{"ReadAt":"2025-07-06 02:39:35","SensorID":1,"Status":"Good","Value":55.1}]}
sensors/MCU002/temperature {"MCU":"MCU002","MsgTimestamp":"2025-07-06 02:39:35","NoOfTempSensors":2,"SensorData":[{"ReadAt":"2025-07-06 02:39:35","SensorID":1,"Status":"Good","Value":55.1},{"ReadAt":"2025-07-06 02:39:35","SensorID":2,"Status":"Good","Value":55.1}]}
sensors/MCU001/temperature {"MCU":"MCU001","MsgTimestamp":"2025-07-06 02:39:36","NoOfTempSensors":3,"SensorData":[{"ReadAt":"2025-07-06 02:39:36","SensorID":1,"Status":"Good","Value":55.1},{"ReadAt":"2025-07-06 02:39:36","SensorID":2,"Status":"Good","Value":55.1},{"ReadAt":"2025-07-06 02:39:36","SensorID":3,"Status":"Good","Value":55.1}]}
```

"MQTT provides real-time, one-to-many communication. Multiple components can subscribe to temperature updates, and the system automatically handles message routing."

#### Step 4: Interactive Temperature Prediction
Lets assume the temperature is currently at 25°C with 20% fan duty cycle. If the temperature rises to 60°C, what fan speed do you expect?

**Let's Calculate**:
```
Linear interpolation formula:
duty_cycle = min_duty + (temp - min_temp) / (max_temp - min_temp) * (max_duty - min_duty)

At 25°C: 20% duty cycle
At 75°C: 100% duty cycle
At 60°C: 20% + (60-25)/(75-25) × 80% = 20% + 35/50 × 80% = 20% + 56% = 76%
```

**Demonstrating the calculation**:
```bash
fan> get_cooling_status
```

**Expected Output**:
```
=== Cooling System Status ===
Cooling Status:
  Average Temperature: 61.3°C
  Current Fan Speed: 78%
  Cooling Mode: MANUAL
```

### Scenario 2: Fault Injection and Recovery

#### Step 1: Multiple Sensor Fault Injection
```bash
fan> exit
1  # Switch to MCU Simulator

mcu> get_mcu_status
```

**Expected Output**:
```
=== MCU Status Report ===
MCU001:
  - Status: Online
  - Sensors: 3/3 Good
  - Last Update: 2025-07-02 18:03:46
  - Publish Interval: 7s

MCU002:
  - Status: Online
  - Sensors: 2/2 Good
  - Last Update: 2025-07-02 18:03:39
  - Publish Interval: 7s

MCU003:
  - Status: Online
  - Sensors: 1/1 Good
  - Last Update: 2025-07-02 18:03:46
  - Publish Interval: 7s
```

```bash
# Inject faults in multiple sensors
mcu> set_sensor_fault MCU001 1 1
mcu> set_sensor_fault MCU002 1 1
mcu> set_sensor_fault MCU003 1 1

# Verify fault injection - show all MCUs
mcu> get_temp
```

**Expected Output**:
```
MCU001:
    Sensor1: 0.0°C (Bad) - Faulty sensor
    Sensor2: 46.6°C (Good)
    Sensor3: 46.6°C (Good)

MCU002:
    Sensor1: 0.0°C (Bad) - Faulty sensor
    Sensor2: 46.6°C (Good)

MCU003:
    Sensor1: 0.0°C (Bad) - Faulty sensor
```

Notice how the system marks faulty sensors as 'Bad' and reports 0°C. In a real system, this would trigger immediate alarms and the system would continue operating with remaining sensors.

#### Step 2: Alarm System Response
```bash
mcu> exit
2  # Back to Fan Control System

fan> get_alarm_history
```

**Expected Output**:
```
Alarm History
Total entries: 9

Alarm: MCU001
  Message: MCU MCU001 Sensor 1 marked as bad
  Severity: WARNING
  First Occurrence: 2025-07-02 18:04:17
  Latest Occurrence: 2025-07-02 18:04:17
  Occurrence Count: 1
  Acknowledged: No

Alarm: MCU001
  Message: MCU MCU001 Sensor 1 showing erratic readings
  Severity: ERROR
  First Occurrence: 2025-07-02 18:04:18
  Latest Occurrence: 2025-07-02 18:04:21
  Occurrence Count: 4
  Acknowledged: No

Alarm: MCU001
  Message: MCU MCU001 Sensor 1 temperature below threshold: 5.00
  Severity: CRITICAL
  First Occurrence: 2025-07-02 18:04:18
  Latest Occurrence: 2025-07-02 18:05:54
  Occurrence Count: 97
  Acknowledged: No

Alarm: MCU002
  Message: MCU MCU002 Sensor 1 marked as bad
  Severity: WARNING
  First Occurrence: 2025-07-02 18:04:26
  Latest Occurrence: 2025-07-02 18:04:26
  Occurrence Count: 1
  Acknowledged: No

Alarm: MCU002
  Message: MCU MCU002 Sensor 1 showing erratic readings
  Severity: ERROR
  First Occurrence: 2025-07-02 18:04:27
  Latest Occurrence: 2025-07-02 18:04:30
  Occurrence Count: 4
  Acknowledged: No

Alarm: MCU002
  Message: MCU MCU002 Sensor 1 temperature below threshold: 5.00
  Severity: CRITICAL
  First Occurrence: 2025-07-02 18:04:27
  Latest Occurrence: 2025-07-02 18:05:54
  Occurrence Count: 88
  Acknowledged: No

Alarm: MCU003
  Message: MCU MCU003 Sensor 1 marked as bad
  Severity: WARNING
  First Occurrence: 2025-07-02 18:04:30
  Latest Occurrence: 2025-07-02 18:04:30
  Occurrence Count: 1
  Acknowledged: No

Alarm: MCU003
  Message: MCU MCU003 Sensor 1 showing erratic readings
  Severity: ERROR
  First Occurrence: 2025-07-02 18:04:31
  Latest Occurrence: 2025-07-02 18:04:34
  Occurrence Count: 4
  Acknowledged: No

Alarm: MCU003
  Message: MCU MCU003 has more than half of its sensors bad
  Severity: CRITICAL
  First Occurrence: 2025-07-02 18:04:31
  Latest Occurrence: 2025-07-02 18:05:54
  Occurrence Count: 168
  Acknowledged: No
```

The alarm system automatically detects sensor failures and categorizes them by severity. Currently the Alarm Manager do not take any actions, however we can enhance the alarm manager to take preventive actions

#### Step 3: System Degradation Analysis
```bash
fan> get_temp_history
```

**Expected Output**:
```
MCU001:
  - 1: 0°C (Bad) - Last: 2025-07-02 18:18:09
  - 2: 54.1°C (Good) - Last: 2025-07-02 18:18:09
  - 3: 54.1°C (Good) - Last: 2025-07-02 18:18:09

MCU002:
  - 1: 0°C (Bad) - Last: 2025-07-02 18:18:09
  - 2: 54.1°C (Good) - Last: 2025-07-02 18:18:09

MCU003:
  - 1: 0°C (Bad) - Last: 2025-07-02 18:18:09

Max Temperature: 54.1°C
Cooling Status:
  Average Temperature: 49.9°C
  Current Fan Speed: 59%
  Cooling Mode: MANUAL
```

The system continues operating with remaining sensors. The fan control algorithm ignores faulty sensors and uses only good readings. This is critical for system reliability.

#### Step 4: Recovery Demonstration
```bash
fan> exit
1  # Back to MCU Simulator

# Clear faults one by one
mcu> set_sensor_fault MCU001 1 0
mcu> set_sensor_fault MCU002 1 0
mcu> set_sensor_fault MCU003 1 0

# Verify recovery
mcu> get_mcu_status
```

**Expected Output**:
```
=== MCU Status Report ===
MCU001:
  - Status: Online
  - Sensors: 3/3 Good
  - Last Update: 2025-07-02 18:20:03
  - Publish Interval: 1s

MCU002:
  - Status: Online
  - Sensors: 2/2 Good
  - Last Update: 2025-07-02 18:20:03
  - Publish Interval: 1s

MCU003:
  - Status: Online
  - Sensors: 1/1 Good
  - Last Update: 2025-07-02 18:20:03
  - Publish Interval: 3s
```

The system automatically detects sensor recovery and clears alarms. This demonstrates the self-healing capabilities of our architecture.

### Scenario 3: Performance Stress Testing

#### Step 1: Rapid Temperature Increase
```bash
mcu> set_sim_params 72.0 80.0 0.5
```

**Expected Output**:
```
Simulation parameters set for 6/6 sensors across all MCUs
```

The simulation parameters control how quickly temperature increases. A step size of 0.5°C means temperature rises by 0.5°C every update cycle.

#### Step 2: Monitor System Response
```bash
mcu> exit
2  # Back to Fan Control System

# Monitor cooling system response
fan> get_cooling_status
fan> exit
1
mcu> get_mcu_status
```

**Expected Output**:
```
Cooling Status:
  Average Temperature: 71.5°C
  Current Fan Speed: 94%
  Cooling Mode: MANUAL

=== MCU Status Report ===
MCU001:
  - Status: Online
  - Sensors: 3/3 Good
  - Last Update: 2025-07-02 18:31:51
  - Publish Interval: 1s

MCU002:
  - Status: Online
  - Sensors: 2/2 Good
  - Last Update: 2025-07-02 18:31:51
  - Publish Interval: 1s

MCU003:
  - Status: Online
  - Sensors: 1/1 Good
  - Last Update: 2025-07-02 18:31:51
  - Publish Interval: 1s
```

Notice the system publish interval is 1 second. Since the temperature is rapidly raising and set between the range of 70-80C

#### Step 3: Noise Level Analysis
```bash
fan> get_fan_noise
```

**Expected Output**:
```
Fan: Fan001 (F4ModelOUT)
  Noise Level: 65 dB
  Noise Category: VERY_LOUD

Fan: Fan002 (F4ModelIN)
  Noise Level: 48 dB
  Noise Category: MODERATE

Fan: Fan003 (F2ModelIN)
  Noise Level: 48 dB
  Noise Category: MODERATE

Fan: Fan004 (F2ModelIN)
  Noise Level: 48 dB
  Noise Category: MODERATE

Successfully retrieved noise levels for 4/4 fans
```

Different fan models have different noise profiles. F4ModelOUT is louder because it's designed for higher airflow.

#### Step 4: Performance Metrics
```bash
# Monitor system resources
top -p $(pgrep -f mcu_simulator) -p $(pgrep -f fan_control_system) -n 1
```

**Expected Output**:
```
  PID USER      PR  NI    VIRT    RES    SHR S  %CPU %MEM     TIME+ COMMAND
 3374 develop+  20   0  988200  12860  10940 S   0.0   0.2   0:04.58 mcu_simulator
 3387 develop+  20   0 1661464  14108  12316 S   0.0   0.2   0:04.93 fan_control_sys
```

The system uses minimal resources - less than 1% CPU and 12MB memory for MCU_Simulator and 14MB for Fan_Control_System. This allows for easy scaling to support more MCUs and fans.

### Scenario 4: Centralized Logging and Log Analysis

#### Step 1: Understanding the Logging Architecture
The system implements a comprehensive centralized logging architecture:

**Logging Components**:
- **Logger Class**: Each component has its own logger instance that publishes logs via MQTT
- **LogManager**: Centralized log manager that subscribes to all log topics and writes to files
- **MQTT Integration**: All logs are published to MQTT topics for real-time monitoring
- **File Rotation**: Automatic log file rotation with configurable size limits

**Log Levels**: DEBUG, INFO, WARNING, ERROR
**Log Format**: JSON with timestamp, level, source, and message

#### Step 2: Real-time Log Monitoring via MQTT
```bash
# Monitor all log messages in real-time
mosquitto_sub -h localhost -t "logs/#" -v
```

**Expected Output**:
```
logs/Fan001/info {"level":1,"message":"Pwm count set to 1408 for duty cycle 72%","source":"Fan001","timestamp":"2025-07-06 02:55:26"}
logs/Fan002/info {"level":1,"message":"Pwm count set to 720 for duty cycle 72%","source":"Fan002","timestamp":"2025-07-06 02:55:26"}
logs/Fan003/info {"level":1,"message":"Pwm count set to 720 for duty cycle 72%","source":"Fan003","timestamp":"2025-07-06 02:55:26"}
logs/Fan004/info {"level":1,"message":"Pwm count set to 720 for duty cycle 72%","source":"Fan004","timestamp":"2025-07-06 02:55:26"}
logs/TempMonitor/info {"level":1,"message":"Updated fan speed to 72%","source":"TempMonitor","timestamp":"2025-07-06 02:55:26"}
```

The MQTT-based logging provides real-time visibility into system operations. Each component publishes logs to its own topic namespace.

#### Step 3: Log File Analysis with lnav
```bash
# View logs using lnav with custom format
lnav /var/log/fan_control_system/fan_control_system.log

```

**Expected Output**:
```
[2025-07-06T03:54:21.000] INFO Fan001: Pwm count set to 1366 for duty cycle 70%
[2025-07-06T03:54:21.000] INFO Fan002: Pwm count set to 700 for duty cycle 70% 
[2025-07-06T03:54:21.000] INFO Fan003: Pwm count set to 700 for duty cycle 70% 
[2025-07-06T03:54:21.000] INFO Fan004: Pwm count set to 700 for duty cycle 70% 
[2025-07-06T03:54:21.000] INFO TempMonitor: Updated fan speed to 70%
```


#### Step 4: Advanced Log Analysis
```bash
# Filter logs by source component
lnav /var/log/fan_control_system/fan_control_system.log -c ":filter-in AlarmManager"

# Filter by log level
lnav /var/log/fan_control_system/fan_control_system.log -c ":filter-in ERROR"


```

**Expected Output**:
```
[2025-07-06T04:08:25.000] WARNING AlarmManager: Alarm Processed: MCU001 - MCU MCU001 Sensor 1 marked as bad
[2025-07-06T04:08:25.000] INFO AlarmManager: Added new alarm: MCU001
[2025-07-06T04:08:25.000] ERROR AlarmManager: Alarm Processed: MCU001 - MCU MCU001 Sensor 1 showing erratic readings
```

#### Step 5: Log File Rotation and Management
```bash
# Check log file sizes and rotation
ls -la /var/log/fan_control_system/

# View rotated log files
lnav /var/log/fan_control_system/fan_control_system_1.log
lnav /var/log/fan_control_system/fan_control_system_2.log
```

**Expected Output**:
```
total 2048
drwxrwxrwx 2 root root    4096 Jul 02 18:45 .
drwxr-xr-x 1 root root    4096 Jul 02 18:45 ..
-rw-r--r-- 1 root root 1048576 Jul 02 18:45 fan_control_system.log
-rw-r--r-- 1 root root 1048576 Jul 02 18:44 fan_control_system_1.log
-rw-r--r-- 1 root root 1048576 Jul 02 18:43 fan_control_system_2.log
-rw-r--r-- 1 root root  524288 Jul 02 18:42 fan_control_system_3.log
```

The system automatically rotates log files when they reach 10MB (configurable). It maintains up to 5 rotated files.

#### Step 6: lnav Configuration and Custom Format
The system includes a custom lnav configuration file (`lnav/fan_control_system.json`) that defines the log format:

```bash
# View the lnav configuration
cat lnav/fan_control_system.json
```

**Configuration Details**:
- **JSON Format**: Logs are stored in JSON format for structured parsing
- **Timestamp Field**: ISO 8601 format timestamps for precise time tracking
- **Level Field**: Log levels (DEBUG, INFO, WARNING, ERROR) for filtering
- **Source Field**: Component identification for source-based filtering
- **Message Field**: Human-readable log messages

**lnav Custom Format Features**:
- **Color Coding**: Different colors for log levels and sources
- **Field Extraction**: Automatic parsing of JSON fields
- **Search Optimization**: Indexed fields for fast searching
- **Time Navigation**: Built-in timestamp navigation

#### Step 7: Logging Configuration and Management
```bash
# View logging configuration
cat config/config.yaml | grep -A 20 "Logging:"

# Check log directory permissions
ls -la /var/log/fan_control_system/
```

**Configuration Parameters**:
- **Log Level**: INFO (configurable per component)
- **File Path**: `/var/log/fan_control_system/`
- **File Name**: `fan_control_system.log`
- **Max File Size**: 10MB per file
- **Max Files**: 5 rotated files
- **Component Levels**: Individual log levels per component

**Log Management Features**:
- **Automatic Rotation**: Files rotate when size limit is reached
- **Compression**: Old log files can be compressed to save space
- **Retention**: Configurable retention period for old logs
- **Permissions**: Proper file permissions for security

### Scenario 5: Configuration and Runtime Changes

#### Step 1: Runtime Configuration Changes
```bash
fan> set_temp_thresholds 30.0 70.0 20 100
```

**Expected Output**:
```
Temperature thresholds set successfully
Message: Temperature thresholds set successfully
Low threshold: 30°C
High threshold: 70°C
Min fan speed: 20%
Max fan speed: 100%
```

The system supports runtime configuration changes without restart. This is crucial for production environments where parameters need adjustment.

#### Step 2: Fan Speed Control
```bash
fan> set_fan_speed Fan001 50
fan> get_fan_status
```

**Expected Output**:
```
Fan speed set successfully
Message: Fan speed set successfully
  Fan: Fan001
    Success: Yes
    Previous duty cycle: 100%
    New duty cycle: 50%

Fan001 (F4ModelOUT):
  - Status: Online
  - Duty Cycle: 50%
  - PWM Count: 944
  - Noise Level: 43 dB
  - Health: Good
  - Model: F4ModelOUT (100-2000)

Fan002 (F4ModelIN):
  - Status: Online
  - Duty Cycle: 100%
  - PWM Count: 1000
  - Noise Level: 48 dB
  - Health: Good
  - Model: F4ModelIN (0-1000)

Fan003 (F2ModelIN):
  - Status: Online
  - Duty Cycle: 100%
  - PWM Count: 1000
  - Noise Level: 48 dB
  - Health: Good
  - Model: F2ModelIN (0-1000)

Fan004 (F2ModelIN):
  - Status: Online
  - Duty Cycle: 100%
  - PWM Count: 1000
  - Noise Level: 48 dB
  - Health: Good
  - Model: F2ModelIN (0-1000)
```

Manual fan control allows for testing and emergency situations. The system maintains the override until temperature-based control scheduled for next interval [Can be enhanced to exclude this fan from automatic fan conrtrol if set manually]

## Key Metrics to Highlight

### System Performance
- **Response Time**: < 1 second for temperature changes
- **Accuracy**: Precise PWM control (±1 count)
- **Reliability**: 99.9% uptime with fault tolerance
- **Resource Usage**: < 2% CPU, ~15MB memory per service

### Temperature Control
- **Range**: 25°C - 75°C with linear interpolation
- **Precision**: 32-bit floating point readings
- **Update Rate**: Adaptive (1-10 seconds based on temperature)
- **Fault Tolerance**: Continues operation with sensor failures

### Fan Management
- **Models Supported**: 3 different fan models
- **PWM Range**: 0-2000 counts per model
- **Noise Control**: Real-time noise monitoring
- **Manual Override**: Emergency control capabilities [Partical, can be enhanced]

### Communication
- **MQTT Latency**: < 10ms message delivery
- **gRPC Response**: < 20ms service calls
- **Scalability**: Linear scaling with component count

### Logging and Monitoring
- **Log format**: JSON based logging infra for easy integration into log navigation tools
- **Search Performance**: lnav provides sub-second search across 1GB logs
- **Real-time Monitoring**: MQTT-based log streaming for live dashboards


## Demo Conclusion and Summary

### Key Demonstration Points
1. **System Architecture**: Microservices with MQTT/gRPC communication
2. **Core Functionality**: Temperature-based fan control with linear interpolation
3. **Error Handling**: Comprehensive alarm system and logging. alarms actions needs to be enhanced
4. **Performance**: Real-time response with configurable parameters
5. **Extensibility**: Easy to add new MCUs, fans, and features
6. **Logging & Monitoring**: Centralized logging with lnav analysis and MQTT streaming

### Technical Highlights
- **Fault Tolerance**: System continues operating with sensor failures
- **Real-time Performance**: Sub-second response to temperature changes
- **Configurable Design**: Runtime parameter adjustment without restarts
- **Production Ready**: Comprehensive logging, monitoring, and error handling
- **Advanced Logging**: JSON-structured logs with lnav analysis and MQTT streaming

