#include "cli/cli.hpp"
#include <iostream>
#include <string>

/**
 * @brief Main entry point for the CLI application
 * 
 * Initializes and runs the Command Line Interface (CLI) for the fan speed control system.
 * The CLI provides an interactive interface for monitoring and controlling the system.
 * 
 * Command line arguments:
 * - Optional: Path to configuration file (defaults to /etc/fan_control_system/config.yaml)
 * 
 * The application will exit with error code 1 if:
 * - The configuration file doesn't exist
 * - CLI initialization fails
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

    cli::CLI cli;
    if (!cli.initialize(config_file)) {
        std::cerr << "Failed to initialize CLI" << std::endl;
        return 1;
    }

    cli.run();
    return 0;
} 