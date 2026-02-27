// common/process_stats.hpp
#ifndef PROCESS_STATS_HPP
#define PROCESS_STATS_HPP

#include <string>

/**
 * @struct ProcessStats
 * @brief A simple helper to track and summarize the count of successful and
 * failed operations.
 */
struct ProcessStats {
  int success = 0;
  int failure = 0;

  /**
   * @brief Compatibility no-op. Core keeps stats but does not print.
   * @param process_name Unused.
   */
  void print_summary(const std::string& process_name) const {
    (void)process_name;
  }
};

#endif  // PROCESS_STATS_HPP
