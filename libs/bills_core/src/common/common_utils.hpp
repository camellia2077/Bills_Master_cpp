// common/common_utils.hpp
#ifndef COMMON_COMMON_UTILS_H_
#define COMMON_COMMON_UTILS_H_

#include <string_view>

namespace common::terminal {

inline constexpr std::string_view kReset = "\033[0m";
inline constexpr std::string_view kRed = "\033[31m";
inline constexpr std::string_view kGreen = "\033[32m";
inline constexpr std::string_view kYellow = "\033[33m";
inline constexpr std::string_view kCyan = "\033[36m";

}  // namespace common::terminal

#endif  // COMMON_COMMON_UTILS_H_
