#ifndef REPORTS_STANDARD_JSON_STANDARD_REPORT_JSON_SERIALIZER_HPP
#define REPORTS_STANDARD_JSON_STANDARD_REPORT_JSON_SERIALIZER_HPP

#include <string>

#include "nlohmann/json.hpp"
#include "reports/standard_json/standard_report_dto.hpp"

class StandardReportJsonSerializer {
 public:
  [[nodiscard]] static auto ToJson(const StandardReport& report)
      -> nlohmann::ordered_json;
  [[nodiscard]] static auto ToString(const StandardReport& report) -> std::string;
};

#endif  // REPORTS_STANDARD_JSON_STANDARD_REPORT_JSON_SERIALIZER_HPP
