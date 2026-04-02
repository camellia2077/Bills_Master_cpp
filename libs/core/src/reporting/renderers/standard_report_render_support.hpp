#pragma once

#include <cctype>
#include <string>
#include <vector>

namespace bills::core::reporting::render_support {

inline auto StripMonthHyphen(const std::string& period_start) -> std::string {
  std::string output;
  output.reserve(period_start.size());
  for (const char ch : period_start) {
    if (ch != '-') {
      output.push_back(ch);
    }
  }
  return output;
}

inline auto FormatMonthlyPeriodLabel(const std::string& period_start)
    -> std::string {
  if (period_start.size() >= 7U) {
    return period_start.substr(0U, 7U);
  }
  if (period_start.size() == 6U) {
    return period_start.substr(0U, 4U) + "-" + period_start.substr(4U, 2U);
  }
  const std::string compact = StripMonthHyphen(period_start);
  if (compact.size() >= 6U) {
    return compact.substr(0U, 4U) + "-" + compact.substr(4U, 2U);
  }
  return period_start;
}

inline auto ParsePeriodMonth(const std::string& period_start, int& year, int& month)
    -> bool {
  if (period_start.size() < 7U || period_start[4] != '-') {
    return false;
  }
  try {
    year = std::stoi(period_start.substr(0U, 4U));
    month = std::stoi(period_start.substr(5U, 2U));
  } catch (...) {
    return false;
  }
  return month >= 1 && month <= 12;
}

inline auto MonthlyRemarkOrDash(const std::string& remark) -> std::string {
  for (const char ch : remark) {
    if (!std::isspace(static_cast<unsigned char>(ch))) {
      return remark;
    }
  }
  return "-";
}

inline auto SplitRemarkLinesOrDash(const std::string& remark)
    -> std::vector<std::string> {
  const std::string normalized_remark = MonthlyRemarkOrDash(remark);
  std::vector<std::string> lines;
  std::string current_line;

  for (const char ch : normalized_remark) {
    if (ch == '\n') {
      lines.push_back(current_line);
      current_line.clear();
      continue;
    }
    if (ch != '\r') {
      current_line.push_back(ch);
    }
  }

  lines.push_back(current_line);
  return lines;
}

inline auto MonthlyTitleText(const std::string& period_start) -> std::string {
  return FormatMonthlyPeriodLabel(period_start) + " 月报";
}

}  // namespace bills::core::reporting::render_support
