#ifndef COMMON_ISO_PERIOD_HPP_
#define COMMON_ISO_PERIOD_HPP_

#include <optional>
#include <string>
#include <string_view>

namespace bills::core::common::iso_period {

struct IsoYearMonth {
  int year = 0;
  int month = 0;
};

[[nodiscard]] auto parse_year(std::string_view raw) -> std::optional<int>;
[[nodiscard]] auto parse_year_month(std::string_view raw)
    -> std::optional<IsoYearMonth>;
[[nodiscard]] auto format_year_month(int year, int month) -> std::string;
[[nodiscard]] auto extract_year_month_from_date_header(std::string_view line)
    -> std::optional<std::string>;

}  // namespace bills::core::common::iso_period

#endif  // COMMON_ISO_PERIOD_HPP_
