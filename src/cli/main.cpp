#include "cli/cli.hpp"
#include <iostream>
#include <string>

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