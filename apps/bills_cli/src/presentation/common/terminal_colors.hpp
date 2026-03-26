#ifndef PRESENTATION_COMMON_TERMINAL_COLORS_HPP_
#define PRESENTATION_COMMON_TERMINAL_COLORS_HPP_

#include <string_view>

namespace bills::cli::terminal {

inline constexpr std::string_view kReset = "\033[0m";
inline constexpr std::string_view kRed = "\033[31m";
inline constexpr std::string_view kGreen = "\033[32m";
inline constexpr std::string_view kYellow = "\033[33m";
inline constexpr std::string_view kCyan = "\033[36m";

}  // namespace bills::cli::terminal

#endif  // PRESENTATION_COMMON_TERMINAL_COLORS_HPP_
