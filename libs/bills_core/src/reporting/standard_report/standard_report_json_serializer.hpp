// reporting/standard_report/standard_report_json_serializer.hpp
#ifndef REPORTING_STANDARD_REPORT_STANDARD_REPORT_JSON_SERIALIZER_H_
#define REPORTING_STANDARD_REPORT_STANDARD_REPORT_JSON_SERIALIZER_H_

#include <string>

#include "nlohmann/json.hpp"
#include "reporting/standard_report/standard_report_dto.hpp"

class StandardReportJsonSerializer {
 public:
  [[nodiscard]] static auto ToJson(const StandardReport& report)
      -> nlohmann::ordered_json;
  [[nodiscard]] static auto ToString(const StandardReport& report) -> std::string;
  [[nodiscard]] static auto FromJson(
      const nlohmann::ordered_json& report_json) -> StandardReport;
  [[nodiscard]] static auto FromString(const std::string& report_json)
      -> StandardReport;
};

#endif  // REPORTING_STANDARD_REPORT_STANDARD_REPORT_JSON_SERIALIZER_H_
