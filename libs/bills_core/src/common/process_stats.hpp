// common/process_stats.hpp
#ifndef COMMON_PROCESS_STATS_H_
#define COMMON_PROCESS_STATS_H_

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

#endif  // COMMON_PROCESS_STATS_H_
