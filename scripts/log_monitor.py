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

# Initialize colorama
init()

# Configure logging
log_dir = Path("logs")
log_dir.mkdir(exist_ok=True)
logging.basicConfig(
    filename=log_dir / "system.log",
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)

def parse_arguments():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(description='Generic Log Monitor with Filtering')
    parser.add_argument('--topic', help='Filter by specific MQTT topic')
    parser.add_argument('--level', choices=['INFO', 'WARNING', 'ERROR', 'DEBUG'], help='Filter by log level')
    parser.add_argument('--source', help='Filter by source/device name')
    parser.add_argument('--time-window', type=int, help='Show only logs from last N minutes')
    return parser.parse_args()

def format_log_level(level):
    """Format log level with appropriate color."""
    colors = {
        'INFO': Fore.GREEN,
        'WARNING': Fore.YELLOW,
        'ERROR': Fore.RED,
        'DEBUG': Fore.CYAN
    }
    return f"{colors.get(level, Fore.WHITE)}{level}{Style.RESET_ALL}"

def format_timestamp(timestamp):
    """Format timestamp with color."""
    return f"{Fore.BLUE}{timestamp}{Style.RESET_ALL}"

def should_display_log(log_data, args):
    """Check if log entry should be displayed based on filters."""
    if args.level and log_data.get('level') != args.level:
        return False
    if args.source and log_data.get('source') != args.source:
        return False
    if args.time_window:
        log_time = datetime.datetime.fromisoformat(log_data.get('timestamp', '').replace('Z', '+00:00'))
        time_diff = datetime.datetime.now(datetime.timezone.utc) - log_time
        if time_diff.total_seconds() > args.time_window * 60:
            return False
    return True

def on_connect(client, userdata, flags, rc):
    """Callback when connected to MQTT broker."""
    print(f"{Fore.CYAN}Connected to MQTT broker{Style.RESET_ALL}")
    args = userdata['args']
    if args.topic:
        client.subscribe(args.topic)
    else:
        client.subscribe("logs/#")

def on_message(client, userdata, msg):
    """Callback when message is received."""
    try:
        data = json.loads(msg.payload.decode())
        args = userdata['args']
        
        # Log all data regardless of filters
        logging.info(f"Topic: {msg.topic}, Data: {data}")
        
        # Print filtered data
        if should_display_log(data, args):
            print("\n" + "=" * 80)
            print(f"Topic: {Fore.YELLOW}{msg.topic}{Style.RESET_ALL}")
            print(f"Timestamp: {format_timestamp(data.get('timestamp', 'N/A'))}")
            print(f"Level: {format_log_level(data.get('level', 'INFO'))}")
            print(f"Source: {Fore.MAGENTA}{data.get('source', 'Unknown')}{Style.RESET_ALL}")
            print("-" * 80)
            print(f"Message: {data.get('message', 'No message')}")
            if 'details' in data:
                print("\nDetails:")
                for key, value in data['details'].items():
                    print(f"  {key}: {value}")
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
    print(f"{Fore.CYAN}Starting Log Monitor...{Style.RESET_ALL}")
    print(f"{Fore.CYAN}Press Ctrl+C to exit{Style.RESET_ALL}")
    print(f"{Fore.CYAN}Log file: {log_dir}/system.log{Style.RESET_ALL}")
    
    try:
        client.loop_forever()
    except KeyboardInterrupt:
        print(f"\n{Fore.CYAN}Stopping Log Monitor...{Style.RESET_ALL}")
        client.disconnect()

if __name__ == "__main__":
    main() 