#include "record_template/period_support.hpp"

#include <sstream>

#include "common/iso_period.hpp"
#include "common/text_normalizer.hpp"

namespace {

auto IsReverseRange(const bills::core::common::iso_period::IsoYearMonth& start_period,
                    const bills::core::common::iso_period::IsoYearMonth& end_period)
    -> bool {
  if (start_period.year != end_period.year) {
    return start_period.year > end_period.year;
  }
  return start_period.month > end_period.month;
}

}  // namespace

auto RecordTemplatePeriodSupport::ExpandTemplatePeriods(
    const TemplateGenerationRequest& request)
    -> RecordTemplateResult<std::vector<std::string>> {
  const bool has_single_period = !request.period.empty();
  const bool has_month_range =
      !request.start_period.empty() || !request.end_period.empty();
  const bool has_year_range =
      !request.start_year.empty() || !request.end_year.empty();
  const int mode_count = static_cast<int>(has_single_period) +
                         static_cast<int>(has_month_range) +
                         static_cast<int>(has_year_range);

  if (mode_count != 1) {
    return std::unexpected(MakeRecordTemplateError(
        RecordTemplateErrorCategory::kRequest,
        "Provide exactly one period spec: period, start_period/end_period, or "
        "start_year/end_year."));
  }

  if (has_single_period) {
    const auto parsed_period =
        bills::core::common::iso_period::parse_year_month(request.period);
    if (!parsed_period.has_value()) {
      return std::unexpected(MakeRecordTemplateError(
          RecordTemplateErrorCategory::kRequest,
          "period must be YYYY-MM."));
    }
    return std::vector<std::string>{bills::core::common::iso_period::format_year_month(
        parsed_period->year, parsed_period->month)};
  }

  std::vector<std::string> periods;
  if (has_month_range) {
    if (request.start_period.empty() || request.end_period.empty()) {
      return std::unexpected(MakeRecordTemplateError(
          RecordTemplateErrorCategory::kRequest,
          "Month range requires both start_period and end_period."));
    }

    const auto start_period =
        bills::core::common::iso_period::parse_year_month(request.start_period);
    const auto end_period =
        bills::core::common::iso_period::parse_year_month(request.end_period);
    if (!start_period.has_value() || !end_period.has_value()) {
      return std::unexpected(MakeRecordTemplateError(
          RecordTemplateErrorCategory::kRequest,
          "Month range requires YYYY-MM to YYYY-MM."));
    }
    if (IsReverseRange(*start_period, *end_period)) {
      return std::unexpected(MakeRecordTemplateError(
          RecordTemplateErrorCategory::kRequest,
          "Month range must not be reversed."));
    }

    auto current_period = *start_period;
    while (!IsReverseRange(current_period, *end_period)) {
      periods.push_back(bills::core::common::iso_period::format_year_month(
          current_period.year, current_period.month));
      ++current_period.month;
      if (current_period.month > 12) {
        current_period.month = 1;
        ++current_period.year;
      }
    }
    return periods;
  }

  if (request.start_year.empty() || request.end_year.empty()) {
    return std::unexpected(MakeRecordTemplateError(
        RecordTemplateErrorCategory::kRequest,
        "Year range requires both start_year and end_year."));
  }

  const auto start_year =
      bills::core::common::iso_period::parse_year(request.start_year);
  const auto end_year =
      bills::core::common::iso_period::parse_year(request.end_year);
  if (!start_year.has_value() || !end_year.has_value()) {
    return std::unexpected(MakeRecordTemplateError(
        RecordTemplateErrorCategory::kRequest,
        "Year range requires YYYY to YYYY."));
  }
  if (*start_year > *end_year) {
    return std::unexpected(MakeRecordTemplateError(
        RecordTemplateErrorCategory::kRequest,
        "Year range must not be reversed."));
  }

  periods.reserve(static_cast<std::size_t>((*end_year - *start_year + 1) * 12));
  for (int year = *start_year; year <= *end_year; ++year) {
    for (int month = 1; month <= 12; ++month) {
      periods.push_back(
          bills::core::common::iso_period::format_year_month(year, month));
    }
  }
  return periods;
}

auto RecordTemplatePeriodSupport::ExtractPeriodFromNormalizedText(
    const std::string& normalized_text) -> RecordTemplateResult<std::string> {
  std::istringstream stream(normalized_text);
  std::string first_line;
  if (!std::getline(stream, first_line)) {
    return std::unexpected(MakeRecordTemplateError(
        RecordTemplateErrorCategory::kInputData, "Bill file is empty."));
  }

  const auto period =
      bills::core::common::iso_period::extract_year_month_from_date_header(first_line);
  if (!period.has_value()) {
    return std::unexpected(MakeRecordTemplateError(
        RecordTemplateErrorCategory::kInputData,
        "The first line must be 'date:YYYY-MM'."));
  }
  return *period;
}
