#include "common/iso_period.hpp"

#include <cctype>
#include <iomanip>
#include <regex>
#include <sstream>

namespace bills::core::common::iso_period {
namespace {

constexpr int kMinSupportedYear = 1900;
constexpr int kMaxSupportedYear = 9999;
constexpr int kMinSupportedMonth = 1;
constexpr int kMaxSupportedMonth = 12;
constexpr std::size_t kYearDigitCount = 4U;
constexpr const char* kDatePrefix = "date:";

auto IsAsciiDigit(unsigned char character) -> bool {
  return std::isdigit(character) != 0;
}

}  // namespace

auto parse_year(std::string_view raw) -> std::optional<int> {
  if (raw.size() != kYearDigitCount) {
    return std::nullopt;
  }
  for (const unsigned char character : raw) {
    if (!IsAsciiDigit(character)) {
      return std::nullopt;
    }
  }

  int year = 0;
  try {
    year = std::stoi(std::string(raw));
  } catch (...) {
    return std::nullopt;
  }

  if (year < kMinSupportedYear || year > kMaxSupportedYear) {
    return std::nullopt;
  }
  return year;
}

auto parse_year_month(std::string_view raw) -> std::optional<IsoYearMonth> {
  static const std::regex kIsoMonthRegex(R"(^(\d{4})-(0[1-9]|1[0-2])$)");
  std::smatch match;
  const std::string raw_string(raw);
  if (!std::regex_match(raw_string, match, kIsoMonthRegex) ||
      match.size() != 3U) {
    return std::nullopt;
  }

  const auto year = parse_year(match[1].str());
  if (!year.has_value()) {
    return std::nullopt;
  }

  int month = 0;
  try {
    month = std::stoi(match[2].str());
  } catch (...) {
    return std::nullopt;
  }

  if (month < kMinSupportedMonth || month > kMaxSupportedMonth) {
    return std::nullopt;
  }

  return IsoYearMonth{.year = *year, .month = month};
}

auto format_year_month(int year, int month) -> std::string {
  std::ostringstream buffer;
  buffer << std::setw(4) << std::setfill('0') << year << "-" << std::setw(2)
         << std::setfill('0') << month;
  return buffer.str();
}

auto extract_year_month_from_date_header(std::string_view line)
    -> std::optional<std::string> {
  if (!line.starts_with(kDatePrefix)) {
    return std::nullopt;
  }

  const auto parsed =
      parse_year_month(line.substr(std::string_view(kDatePrefix).size()));
  if (!parsed.has_value()) {
    return std::nullopt;
  }
  return format_year_month(parsed->year, parsed->month);
}

}  // namespace bills::core::common::iso_period
