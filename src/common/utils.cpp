#include "common/utils.hpp"
#include <iomanip>
#include <sstream>

namespace common {
namespace utils {

/**
 * @brief Formats a system clock time point to a human-readable timestamp string
 * 
 * Converts a std::chrono::system_clock::time_point to a string in the format
 * "YYYY-MM-DD HH:MM:SS" using the local timezone.
 * 
 * @param tp The system clock time point to format
 * @return Formatted timestamp string in "YYYY-MM-DD HH:MM:SS" format
 */
std::string formatTimestamp(const std::chrono::system_clock::time_point& tp) {
    auto time = std::chrono::system_clock::to_time_t(tp);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

/**
 * @brief Gets the current system time as a formatted timestamp string
 * 
 * Retrieves the current system time and formats it as a human-readable
 * timestamp string in "YYYY-MM-DD HH:MM:SS" format.
 * 
 * @return Current timestamp string in "YYYY-MM-DD HH:MM:SS" format
 */
std::string getCurrentTimestamp() {
    return formatTimestamp(std::chrono::system_clock::now());
}

} // namespace utils
} // namespace common 