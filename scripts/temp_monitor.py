#!/usr/bin/env python3

import paho.mqtt.client as mqtt
import json
import datetime
import sys
import os
from colorama import init, Fore, Style

# Initialize colorama
init()

def format_temperature(value):
    """Format temperature with color based on value."""
    if value < 10:
        return f"{Fore.RED}{value:.2f}°C{Style.RESET_ALL}"
    elif value > 70:
        return f"{Fore.RED}{value:.2f}°C{Style.RESET_ALL}"
    elif value > 60:
        return f"{Fore.YELLOW}{value:.2f}°C{Style.RESET_ALL}"
    elif value > 40:
        return f"{Fore.GREEN}{value:.2f}°C{Style.RESET_ALL}"
    else:
        return f"{Fore.CYAN}{value:.2f}°C{Style.RESET_ALL}"

def format_status(status):
    """Format status with color."""
    if status == "Good":
        return f"{Fore.GREEN}{status}{Style.RESET_ALL}"
    else:
        return f"{Fore.RED}{status}{Style.RESET_ALL}"

def on_connect(client, userdata, flags, rc):
    """Callback when connected to MQTT broker."""
    print(f"{Fore.CYAN}Connected to MQTT broker{Style.RESET_ALL}")
    client.subscribe("sensors/+/temperature")

def on_message(client, userdata, msg):
    """Callback when message is received."""
    try:
        data = json.loads(msg.payload.decode())
        
        # Print MCU header
        print(f"\n{Fore.YELLOW}MCU: {data['MCU']}{Style.RESET_ALL}")
        print(f"{Fore.YELLOW}Timestamp: {data['MsgTimestamp']}{Style.RESET_ALL}")
        print(f"{Fore.YELLOW}Number of Sensors: {data['NoOfTempSensors']}{Style.RESET_ALL}")
        
        # Print sensor data
        print("\nSensor Readings:")
        print("-" * 80)
        print(f"{'Sensor ID':<10} {'Read At':<20} {'Temperature':<15} {'Status':<10}")
        print("-" * 80)
        
        for sensor in data['SensorData']:
            print(f"{sensor['SensorID']:<10} "
                  f"{sensor['ReadAt']:<20} "
                  f"{format_temperature(sensor['Value']):<15} "
                  f"{format_status(sensor['Status']):<10}")
        
        print("-" * 80)
        
    except json.JSONDecodeError:
        print(f"{Fore.RED}Error: Invalid JSON message{Style.RESET_ALL}")
    except Exception as e:
        print(f"{Fore.RED}Error processing message: {str(e)}{Style.RESET_ALL}")

def main():
    # Create MQTT client
    client = mqtt.Client()
    
    # Set callbacks
    client.on_connect = on_connect
    client.on_message = on_message
    
    # Connect to broker
    try:
        client.connect("localhost", 1883, 60)
    except Exception as e:
        print(f"{Fore.RED}Error connecting to MQTT broker: {str(e)}{Style.RESET_ALL}")
        sys.exit(1)
    
    # Start the loop
    print(f"{Fore.CYAN}Starting MQTT monitor...{Style.RESET_ALL}")
    print(f"{Fore.CYAN}Press Ctrl+C to exit{Style.RESET_ALL}")
    
    try:
        client.loop_forever()
    except KeyboardInterrupt:
        print(f"\n{Fore.CYAN}Stopping MQTT monitor...{Style.RESET_ALL}")
        client.disconnect()

if __name__ == "__main__":
    main() 