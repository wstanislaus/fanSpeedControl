#include "mcu_simulator/mcu_simulator.hpp"
#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>

namespace mcu_simulator {

volatile bool running = true;

void signalHandler(int) {
    running = false;
}

} // namespace mcu_simulator

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

    // Set up signal handler
    signal(SIGINT, mcu_simulator::signalHandler);
    signal(SIGTERM, mcu_simulator::signalHandler);

    try {
        // Create and initialize MCU simulator
        mcu_simulator::MCUSimulator simulator(config_file);
        if (!simulator.initialize()) {
            std::cerr << "Failed to initialize MCU simulator" << std::endl;
            return 1;
        }

        // Start the simulator
        simulator.start();
        std::cout << "MCU Simulator started. Press Ctrl+C to stop." << std::endl;

        // Main loop
        while (mcu_simulator::running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Stop the simulator
        simulator.stop();
        std::cout << "MCU Simulator stopped." << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
} 