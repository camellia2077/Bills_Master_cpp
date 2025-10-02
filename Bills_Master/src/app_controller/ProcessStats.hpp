// app_controller/ProcessStats.hpp
#ifndef PROCESS_STATS_HPP
#define PROCESS_STATS_HPP

#include <iostream>
#include <string>
#include "common/common_utils.hpp" // For color definitions

/**
 * @struct ProcessStats
 * @brief A simple helper to track and summarize the count of successful and failed operations.
 */
struct ProcessStats {
    int success = 0;
    int failure = 0;

    /**
     * @brief Prints a formatted summary of the tracked operations.
     * @param process_name The name of the process being summarized (e.g., "Validation").
     */
    void print_summary(const std::string& process_name) const {
        std::cout << "\n--- " << process_name << " Summary ---\n";
        std::cout << GREEN_COLOR << "Successful operations: " << RESET_COLOR << success << "\n";
        std::cout << RED_COLOR << "Failed operations:     " << RESET_COLOR << failure << "\n";
        std::cout << "--------------------------------\n";
    }
};

#endif // PROCESS_STATS_HPP