#!/usr/bin/env python3

import paho.mqtt.client as mqtt
import json
import datetime
import sys
import os
import argparse
from colorama import init, Fore, Style
import logging
from pathlib import Path
import time

# Initialize colorama
init()

# Configure logging
log_dir = Path("logs")
log_dir.mkdir(exist_ok=True)
logging.basicConfig(
    filename=log_dir / "alarms.log",
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)

# Alarm severity levels and their colors
ALARM_LEVELS = {
    'CRITICAL': Fore.RED,
    'HIGH': Fore.RED,
    'MEDIUM': Fore.YELLOW,
    'LOW': Fore.GREEN,
    'INFO': Fore.CYAN
}

def parse_arguments():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(description='Alarm Monitor with Filtering')
    parser.add_argument('--level', choices=list(ALARM_LEVELS.keys()), help='Filter by alarm level')
    parser.add_argument('--source', help='Filter by source/device name')
    parser.add_argument('--active-only', action='store_true', help='Show only active alarms')
    parser.add_argument('--acknowledged', action='store_true', help='Show only acknowledged alarms')
    parser.add_argument('--time-window', type=int, help='Show only alarms from last N minutes')
    return parser.parse_args()

def format_alarm_level(level):
    """Format alarm level with appropriate color."""
    return f"{ALARM_LEVELS.get(level, Fore.WHITE)}{level}{Style.RESET_ALL}"

def format_timestamp(timestamp):
    """Format timestamp with color."""
    return f"{Fore.BLUE}{timestamp}{Style.RESET_ALL}"

def format_alarm_status(status):
    """Format alarm status with color."""
    if status == 'ACTIVE':
        return f"{Fore.RED}{status}{Style.RESET_ALL}"
    elif status == 'ACKNOWLEDGED':
        return f"{Fore.YELLOW}{status}{Style.RESET_ALL}"
    else:
        return f"{Fore.GREEN}{status}{Style.RESET_ALL}"

def should_display_alarm(alarm_data, args):
    """Check if alarm should be displayed based on filters."""
    if args.level and alarm_data.get('level') != args.level:
        return False
    if args.source and alarm_data.get('source') != args.source:
        return False
    if args.active_only and alarm_data.get('status') != 'ACTIVE':
        return False
    if args.acknowledged and alarm_data.get('status') != 'ACKNOWLEDGED':
        return False
    if args.time_window:
        alarm_time = datetime.datetime.fromisoformat(alarm_data.get('timestamp', '').replace('Z', '+00:00'))
        time_diff = datetime.datetime.now(datetime.timezone.utc) - alarm_time
        if time_diff.total_seconds() > args.time_window * 60:
            return False
    return True

def on_connect(client, userdata, flags, rc):
    """Callback when connected to MQTT broker."""
    print(f"{Fore.CYAN}Connected to MQTT broker{Style.RESET_ALL}")
    client.subscribe("alarms/#")

def on_message(client, userdata, msg):
    """Callback when message is received."""
    try:
        data = json.loads(msg.payload.decode())
        args = userdata['args']
        
        # Log all alarms regardless of filters
        logging.info(f"Topic: {msg.topic}, Alarm: {data}")
        
        # Print filtered alarms
        if should_display_alarm(data, args):
            print("\n" + "=" * 80)
            print(f"Alarm ID: {Fore.YELLOW}{data.get('alarm_id', 'N/A')}{Style.RESET_ALL}")
            print(f"Timestamp: {format_timestamp(data.get('timestamp', 'N/A'))}")
            print(f"Level: {format_alarm_level(data.get('level', 'INFO'))}")
            print(f"Status: {format_alarm_status(data.get('status', 'UNKNOWN'))}")
            print(f"Source: {Fore.MAGENTA}{data.get('source', 'Unknown')}{Style.RESET_ALL}")
            print("-" * 80)
            print(f"Message: {data.get('message', 'No message')}")
            if 'details' in data:
                print("\nDetails:")
                for key, value in data['details'].items():
                    print(f"  {key}: {value}")
            if 'acknowledged_by' in data:
                print(f"\nAcknowledged by: {data['acknowledged_by']}")
                print(f"Acknowledged at: {format_timestamp(data.get('acknowledged_at', 'N/A'))}")
            print("=" * 80)
        
    except json.JSONDecodeError:
        print(f"{Fore.RED}Error: Invalid JSON message{Style.RESET_ALL}")
        logging.error("Invalid JSON message received")
    except Exception as e:
        print(f"{Fore.RED}Error processing message: {str(e)}{Style.RESET_ALL}")
        logging.error(f"Error processing message: {str(e)}")

def main():
    args = parse_arguments()
    
    # Create MQTT client
    client = mqtt.Client()
    client.user_data_set({'args': args})
    
    # Set callbacks
    client.on_connect = on_connect
    client.on_message = on_message
    
    # Connect to broker
    try:
        client.connect("localhost", 1883, 60)
    except Exception as e:
        print(f"{Fore.RED}Error connecting to MQTT broker: {str(e)}{Style.RESET_ALL}")
        logging.error(f"Error connecting to MQTT broker: {str(e)}")
        sys.exit(1)
    
    # Start the loop
    print(f"{Fore.CYAN}Starting Alarm Monitor...{Style.RESET_ALL}")
    print(f"{Fore.CYAN}Press Ctrl+C to exit{Style.RESET_ALL}")
    print(f"{Fore.CYAN}Log file: {log_dir}/alarms.log{Style.RESET_ALL}")
    
    try:
        client.loop_forever()
    except KeyboardInterrupt:
        print(f"\n{Fore.CYAN}Stopping Alarm Monitor...{Style.RESET_ALL}")
        client.disconnect()

if __name__ == "__main__":
    main() 