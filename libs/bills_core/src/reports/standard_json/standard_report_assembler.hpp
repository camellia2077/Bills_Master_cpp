// reports/standard_json/standard_report_assembler.hpp
#ifndef REPORTS_STANDARD_JSON_STANDARD_REPORT_ASSEMBLER_H_
#define REPORTS_STANDARD_JSON_STANDARD_REPORT_ASSEMBLER_H_

#include "ports/contracts/reports/monthly/monthly_report_data.hpp"
#include "ports/contracts/reports/yearly/yearly_report_data.hpp"
#include "reports/standard_json/standard_report_dto.hpp"

class StandardReportAssembler {
 public:
  [[nodiscard]] static auto FromMonthly(const MonthlyReportData& data)
      -> StandardReport;
  [[nodiscard]] static auto FromYearly(const YearlyReportData& data)
      -> StandardReport;
};

#endif  // REPORTS_STANDARD_JSON_STANDARD_REPORT_ASSEMBLER_H_
