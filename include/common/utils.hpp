#pragma once

#include <chrono>
#include <string>

namespace common {

/**
 * @brief Common utility functions used across the project
 */
namespace utils {

/**
 * @brief Formats a timestamp to a human-readable string
 * 
 * Converts a system clock time point to a string in the format
 * "YYYY-MM-DD HH:MM:SS".
 * 
 * @param tp The timestamp to format
 * @return Formatted timestamp string
 */
std::string formatTimestamp(const std::chrono::system_clock::time_point& tp);

/**
 * @brief Gets the current timestamp in human-readable format
 * 
 * Formats the current system time as "YYYY-MM-DD HH:MM:SS"
 * 
 * @return Timestamp string in the format "YYYY-MM-DD HH:MM:SS"
 */
std::string getCurrentTimestamp();

} // namespace utils
} // namespace common 