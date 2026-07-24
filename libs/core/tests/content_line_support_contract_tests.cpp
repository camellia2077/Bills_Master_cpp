#include <cmath>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "config/modifier_data.hpp"
#include "domain/bill/bill_record.hpp"
#include "ingest/transform/bills_parser.hpp"
#include "ingest/transform/content_line_support.hpp"
#include "ports/report_data_gateway.hpp"
#include "query/query_service.hpp"
#include "record_template/record_editor_service.hpp"
#include "reporting/report_render_service.hpp"

namespace {

constexpr double kEpsilon = 1e-9;

auto nearly_equal(double left, double right) -> bool {
  return std::abs(left - right) <= kEpsilon;
}

auto expect(bool condition, std::string_view message) -> bool {
  if (!condition) {
    std::cerr << "[FAIL] " << message << '\n';
    return false;
  }
  return true;
}

auto find_transaction(const ParsedBill& bill, std::string_view description)
    -> const Transaction* {
  for (const auto& transaction : bill.transactions) {
    if (transaction.description == description) {
      return &transaction;
    }
  }
  return nullptr;
}

class FakeRangeGateway final : public ReportDataGateway {
 public:
  explicit FakeRangeGateway(RangeReportData range_data)
      : range_data_(std::move(range_data)) {}

  auto ReadRangeData(std::string_view start_iso_month,
                     std::string_view end_iso_month) -> RangeReportData override {
    last_start_ = std::string(start_iso_month);
    last_end_ = std::string(end_iso_month);
    return range_data_;
  }

  auto ListAvailableMonths() -> std::vector<std::string> override { return {}; }

  [[nodiscard]] auto last_start() const -> const std::string& { return last_start_; }
  [[nodiscard]] auto last_end() const -> const std::string& { return last_end_; }

 private:
  RangeReportData range_data_;
  std::string last_start_;
  std::string last_end_;
};

auto make_monthly_report(int year, int month, double income, double expense)
    -> MonthlyReportData {
  MonthlyReportData report;
  report.year = year;
  report.month = month;
  report.data_found = true;
  report.total_income = income;
  report.total_expense = expense;
  report.balance = income + expense;
  return report;
}

auto test_content_line_parser() -> bool {
  using bills::core::ingest::content_line::EvaluateAmountExpression;
  using bills::core::ingest::content_line::ParseStructuredEntryLine;

  const auto meal =
      ParseStructuredEntryLine("meal", "103.60*5+6.03 饭 // 有优惠买的");
  if (!expect(meal.has_value(), "meal expression should parse")) {
    return false;
  }
  if (!expect(meal->amount_expression == "103.60*5+6.03",
              "amount_expression should preserve original input")) {
    return false;
  }
  if (!expect(nearly_equal(meal->evaluated_amount, -524.03),
              "expense expression should evaluate to signed amount")) {
    return false;
  }
  if (!expect(meal->description == "饭", "description should stop before comment")) {
    return false;
  }
  if (!expect(meal->comment == "有优惠买的", "double slash comment should parse")) {
    return false;
  }

  const auto group_buy = ParseStructuredEntryLine("meal", "10/(2+3) 半价 # 团购");
  if (!expect(group_buy.has_value(), "division and parentheses should parse")) {
    return false;
  }
  if (!expect(group_buy->amount_expression == "10/(2+3)",
              "division expression should preserve original input")) {
    return false;
  }
  if (!expect(nearly_equal(group_buy->evaluated_amount, -2.0),
              "division expression should evaluate correctly")) {
    return false;
  }
  if (!expect(group_buy->description == "半价", "# comment should split description")) {
    return false;
  }
  if (!expect(group_buy->comment == "团购", "# comment should parse")) {
    return false;
  }

  const auto stationery = ParseStructuredEntryLine("meal", "2×3+4 文具 ; 备用");
  if (!expect(stationery.has_value(), "utf8 times symbol should parse")) {
    return false;
  }
  if (!expect(stationery->amount_expression == "2×3+4",
              "utf8 times symbol should be preserved")) {
    return false;
  }
  if (!expect(nearly_equal(stationery->evaluated_amount, -10.0),
              "utf8 times expression should evaluate correctly")) {
    return false;
  }
  if (!expect(stationery->description == "文具", "; comment should split description")) {
    return false;
  }
  if (!expect(stationery->comment == "备用", "; comment should parse")) {
    return false;
  }

  const auto no_comment = ParseStructuredEntryLine("meal", "12 饭#优惠");
  if (!expect(no_comment.has_value(), "description containing # should still parse")) {
    return false;
  }
  if (!expect(no_comment->description == "饭#优惠",
              "# without preceding whitespace should stay in description")) {
    return false;
  }
  if (!expect(no_comment->comment.empty(),
              "# without preceding whitespace should not start a comment")) {
    return false;
  }

  if (!expect(!ParseStructuredEntryLine("meal", "10/(2+3 饭").has_value(),
              "unterminated parentheses should fail the whole line")) {
    return false;
  }
  if (!expect(!ParseStructuredEntryLine("meal", "10/0 饭").has_value(),
              "division by zero should fail the whole line")) {
    return false;
  }
  if (!expect(nearly_equal(EvaluateAmountExpression("income", "12*2"), 24.0),
              "income without explicit sign should stay positive")) {
    return false;
  }
  if (!expect(nearly_equal(EvaluateAmountExpression("meal", "+12"), 12.0),
              "explicit positive sign should override expense default")) {
    return false;
  }
  return true;
}

auto test_record_editor_contract() -> bool {
  const std::string raw_text =
      "date:2026-06\n"
      "remark:expression regression\n"
      "\n"
      "meal\n"
      "\n"
      "meal_low\n"
      "103.60*5+6.03 饭 // 有优惠买的\n"
      "10/(2+3) 半价 # 团购\n"
      "2×3+4 文具 ; 备用\n";

  const auto parsed = RecordEditorService::Parse(raw_text);
  if (!expect(parsed.has_value(), "record editor should parse supported expressions")) {
    return false;
  }
  if (!expect(parsed->sections.size() == 1U, "record editor should keep parent section")) {
    return false;
  }
  if (!expect(parsed->sections.front().sub_sections.size() == 1U,
              "record editor should keep sub section")) {
    return false;
  }
  const auto& entries = parsed->sections.front().sub_sections.front().entries;
  if (!expect(entries.size() == 3U, "record editor should parse all content lines")) {
    return false;
  }
  if (!expect(entries[0].amount_expression == "103.60*5+6.03",
              "record editor should preserve amount expression")) {
    return false;
  }
  if (!expect(entries[1].comment == "团购",
              "record editor should expose # comment")) {
    return false;
  }
  if (!expect(entries[2].comment == "备用",
              "record editor should expose ; comment")) {
    return false;
  }

  const auto serialized = RecordEditorService::Serialize(*parsed);
  if (!expect(serialized.has_value(), "record editor should serialize parsed document")) {
    return false;
  }
  if (!expect(serialized->find("103.60*5+6.03 饭 // 有优惠买的") != std::string::npos,
              "serialized text should keep original amount expression")) {
    return false;
  }
  if (!expect(serialized->find("10/(2+3) 半价 // 团购") != std::string::npos,
              "serialized text should normalize parsed comments via //")) {
    return false;
  }
  return true;
}

auto test_bill_parser_contract() -> bool {
  Config config;
  config.metadata_prefixes = {"date:", "remark:"};

  BillParser parser(config);
  const ParsedBill bill = parser.parse({
      "date:2026-06",
      "remark:expression regression",
      "",
      "meal",
      "",
      "meal_low",
      "103.60*5+6.03 饭 // 有优惠买的",
      "10/(2+3) 半价 # 团购",
      "2×3+4 文具 ; 备用",
  });

  if (!expect(nearly_equal(bill.total_income, 0.0), "bill should have no income")) {
    return false;
  }
  if (!expect(nearly_equal(bill.total_expense, -536.03),
              "bill total expense should use evaluated signed amounts")) {
    return false;
  }
  if (!expect(nearly_equal(bill.balance, -536.03),
              "bill balance should match signed totals")) {
    return false;
  }

  const Transaction* meal = find_transaction(bill, "饭");
  const Transaction* group_buy = find_transaction(bill, "半价");
  const Transaction* stationery = find_transaction(bill, "文具");
  if (!expect(meal != nullptr, "bill parser should emit meal transaction")) {
    return false;
  }
  if (!expect(group_buy != nullptr, "bill parser should emit group-buy transaction")) {
    return false;
  }
  if (!expect(stationery != nullptr, "bill parser should emit stationery transaction")) {
    return false;
  }
  if (!expect(nearly_equal(meal->amount, -524.03),
              "bill parser should use evaluated signed amount for meal")) {
    return false;
  }
  if (!expect(meal->comment == "有优惠买的",
              "bill parser should keep // comment")) {
    return false;
  }
  if (!expect(nearly_equal(group_buy->amount, -2.0),
              "bill parser should use evaluated signed amount for division")) {
    return false;
  }
  if (!expect(group_buy->comment == "团购", "bill parser should keep # comment")) {
    return false;
  }
  if (!expect(nearly_equal(stationery->amount, -10.0),
              "bill parser should use evaluated signed amount for × expression")) {
    return false;
  }
  if (!expect(stationery->comment == "备用", "bill parser should keep ; comment")) {
    return false;
  }
  return true;
}

auto test_query_range_and_presentations() -> bool {
  RangeReportData range_data;
  range_data.period_start = "2025-03";
  range_data.period_end = "2025-04";
  range_data.data_found = true;
  range_data.total_income = 300.0;
  range_data.total_expense = -120.0;
  range_data.balance = 180.0;
  range_data.months.push_back(make_monthly_report(2025, 3, 100.0, -40.0));
  range_data.months.push_back(make_monthly_report(2025, 4, 200.0, -80.0));

  FakeRangeGateway gateway(range_data);
  const auto query_result = QueryService::QueryRange(gateway, "2025-03", "2025-04");
  if (!expect(query_result.period_start == "2025-03",
              "query range should preserve start period")) {
    return false;
  }
  if (!expect(query_result.period_end == "2025-04",
              "query range should preserve end period")) {
    return false;
  }
  if (!expect(gateway.last_start() == "2025-03" && gateway.last_end() == "2025-04",
              "query range should call gateway with normalized bounds")) {
    return false;
  }
  if (!expect(query_result.range_data.months.size() == 2U,
              "query range should return ordered months")) {
    return false;
  }

  const auto yearly_report = ReportRenderService::BuildStandardReport(
      QueryExecutionResult{
          .period_start = "2025-01",
          .period_end = "2025-12",
          .data_found = true,
          .range_data =
              RangeReportData{
                  .period_start = "2025-01",
                  .period_end = "2025-12",
                  .data_found = true,
                  .total_income = range_data.total_income,
                  .total_expense = range_data.total_expense,
                  .balance = range_data.balance,
                  .months = range_data.months,
              },
      },
      ReportPresentationKind::kYearly);
  if (!expect(yearly_report.report_type == "yearly",
              "yearly projection should mark report type")) {
    return false;
  }
  if (!expect(yearly_report.monthly_summary.size() == 2U,
              "yearly projection should emit monthly summary rows")) {
    return false;
  }
  if (!expect(yearly_report.monthly_summary.front().period == "2025-03",
              "yearly projection should preserve period labels")) {
    return false;
  }

  const auto range_report = ReportRenderService::BuildStandardReport(
      query_result, ReportPresentationKind::kRange);
  if (!expect(range_report.report_type == "range",
              "range projection should mark report type")) {
    return false;
  }
  if (!expect(range_report.categories.empty(),
              "range projection should not emit monthly detail categories")) {
    return false;
  }
  if (!expect(range_report.monthly_summary.size() == 2U,
              "range projection should emit monthly summary rows")) {
    return false;
  }

  return true;
}

}  // namespace

auto main() -> int {
  if (!test_content_line_parser()) {
    return EXIT_FAILURE;
  }
  if (!test_record_editor_contract()) {
    return EXIT_FAILURE;
  }
  if (!test_bill_parser_contract()) {
    return EXIT_FAILURE;
  }
  if (!test_query_range_and_presentations()) {
    return EXIT_FAILURE;
  }
  std::cout << "[OK] bills_core content-line contract tests passed.\n";
  return EXIT_SUCCESS;
}
