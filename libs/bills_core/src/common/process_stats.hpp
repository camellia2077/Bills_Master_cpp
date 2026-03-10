// common/process_stats.hpp
#ifndef COMMON_PROCESS_STATS_H_
#define COMMON_PROCESS_STATS_H_

#include <string>
#include <utility>
#include <vector>

struct ProcessFailureDetail {
  std::string path;
  std::string stage;
  std::string message;
};

/**
 * @struct ProcessStats
 * @brief A simple helper to track and summarize the count of successful and
 * failed operations.
 */
struct ProcessStats {
  int success = 0;
  int failure = 0;
  std::vector<ProcessFailureDetail> failure_details;

  void add_failure(std::string path, std::string stage, std::string message) {
    failure_details.push_back(ProcessFailureDetail{
        .path = std::move(path),
        .stage = std::move(stage),
        .message = std::move(message),
    });
  }

  /**
   * @brief Compatibility no-op. Core keeps stats but does not print.
   * @param process_name Unused.
   */
  void print_summary(const std::string& process_name) const {
    (void)process_name;
  }
};

#endif  // COMMON_PROCESS_STATS_H_
