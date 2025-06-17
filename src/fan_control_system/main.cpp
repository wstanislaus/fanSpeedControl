#include "fan_control_system/fan_control_system.hpp"
#include <iostream>
#include <csignal>
#include <cstdlib>

/**
 * @brief Global pointer to the fan control system instance
 */
std::unique_ptr<FanControlSystem> g_system;

/**
 * @brief Signal handler for graceful shutdown
 * 
 * Handles SIGINT and SIGTERM signals by stopping the fan control system
 * and exiting the program.
 * 
 * @param signal The signal number that triggered the handler
 */
void signal_handler(int signal) {
    if (g_system) {
        std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
        g_system->stop();
    }
    exit(signal);
}

/**
 * @brief Main entry point for the fan control system
 * 
 * Parses command line arguments, sets up signal handlers, and starts
 * the fan control system. Waits for a shutdown signal before exiting.
 * 
 * @param argc Number of command line arguments
 * @param argv Array of command line argument strings
 * @return 0 on successful execution, 1 on error
 */
int main(int argc, char* argv[]) {
    // config file is optional if default config file is present in /etc/fan_control_system/config.yaml
    std::string config_file = "/etc/fan_control_system/config.yaml";
    if (argc == 2) {
        config_file = argv[1];
    }

    if (argc != 2) {
        //Check if config file exists
        if (access(config_file.c_str(), F_OK) == -1) {
            std::cerr << "Error: Config file " << config_file << " does not exist" << std::endl;
            std::cerr << "Provide config file as argument or place it in /etc/fan_control_system/config.yaml" << std::endl;
            std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
            return 1;
        }
    }

    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    try {
        // Create and start the fan control system
        g_system = std::make_unique<FanControlSystem>(config_file);
        if (!g_system->start()) {
            std::cerr << "Failed to start fan control system" << std::endl;
            return 1;
        }

        std::cout << "Fan control system started successfully" << std::endl;

        // Wait for shutdown signal
        while (g_system->is_running()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
} 