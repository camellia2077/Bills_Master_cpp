// abi/internal/handlers/query_handler.cpp
#include <algorithm>
#include <cstddef>
#include <cctype>
#include <exception>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "common/iso_period.hpp"

#if BILLS_CORE_MODULES_ENABLED
import bill.core.abi;
import bill.core.domain.bill_record;
import bill.core.reports.standard_report_assembler;
import bill.core.reports.standard_report_json_serializer;
import bill.core.serialization.bill_json_serializer;
namespace core_abi = bills::core::modules::abi;
using bills::core::modules::domain_bill_record::ParsedBill;
using bills::core::modules::reports::MonthlyReportData;
using bills::core::modules::reports::MonthlySummary;
using bills::core::modules::reports::StandardReportAssembler;
using bills::core::modules::reports::StandardReportJsonSerializer;
using bills::core::modules::reports::YearlyReportData;
using bills::core::modules::serialization::BillJsonSerializer;
#else
#include "abi/internal/abi_shared.hpp"
namespace core_abi = bills::core::abi;
#endif

namespace bills::core::abi {

#if BILLS_CORE_MODULES_ENABLED
using Json = core_abi::Json;
namespace fs = core_abi::fs;
#endif

namespace {
auto ToAsciiLower(std::string value) -> std::string {
  std::ranges::transform(value, value.begin(),
                         [](unsigned char character) -> char {
                           return static_cast<char>(std::tolower(character));
                         });
  return value;
}

auto ParseYearString(const std::string& raw, int& year) -> bool {
  const auto parsed_year = bills::core::common::iso_period::parse_year(raw);
  if (!parsed_year.has_value()) {
    return false;
  }
  year = *parsed_year;
  return true;
}

auto ParseMonthString(const std::string& raw, int& year, int& month) -> bool {
  const auto parsed_period =
      bills::core::common::iso_period::parse_year_month(raw);
  if (!parsed_period.has_value()) {
    return false;
  }
  year = parsed_period->year;
  month = parsed_period->month;
  return true;
}

}  // namespace

// NOLINTNEXTLINE(readability-function-cognitive-complexity) -- query assembly keeps validation, aggregation, and response shaping together.
auto handle_query_command(const Json& request) -> std::string {
  const Json kParams = request.value("params", Json::object());
  if (!kParams.is_object()) {
    return core_abi::make_response(false, core_abi::error_code::kParamInvalidRequest,
                                   "'params' must be a JSON object.");
  }

  const std::string kQueryTypeRaw = kParams.value("type", "");
  const std::string kQueryValue = kParams.value("value", "");
  const std::string kInputPath =
      kParams.value("input_path",
                   std::string(core_abi::constants::kDefaultConvertOutputDir));

  if (kQueryTypeRaw.empty() || kQueryValue.empty()) {
    return core_abi::make_response(
        false, core_abi::error_code::kParamInvalidRequest,
        "Query requires non-empty 'params.type' and 'params.value'.");
  }

  const std::string kQueryType = ToAsciiLower(kQueryTypeRaw);
  const bool kIsYearQuery = (kQueryType == "year" || kQueryType == "y");
  const bool kIsMonthQuery = (kQueryType == "month" || kQueryType == "m");
  if (!kIsYearQuery && !kIsMonthQuery) {
    Json data;
    data["type"] = kQueryTypeRaw;
    data["supported_types"] = {"year", "month"};
    return core_abi::make_response(
        false, core_abi::error_code::kParamInvalidRequest,
        "Unsupported query type.", std::move(data));
  }

  int query_year = 0;
  int query_month = 0;
  bool parsed = false;
  if (kIsYearQuery) {
    parsed = ParseYearString(kQueryValue, query_year);
  } else {
    parsed = ParseMonthString(kQueryValue, query_year, query_month);
  }
  if (!parsed) {
    Json data;
    data["type"] = kQueryTypeRaw;
    data["value"] = kQueryValue;
    data["expected_format"] = kIsYearQuery ? "YYYY" : "YYYY-MM";
    return core_abi::make_response(
        false, core_abi::error_code::kParamInvalidRequest,
        kIsYearQuery ? "Year query requires YYYY."
                     : "Month query requires YYYY-MM.",
        std::move(data));
  }

  std::vector<fs::path> files;
  try {
    files = core_abi::list_json_files(fs::path(kInputPath));
  } catch (const std::exception& ex) {
    Json data;
    data["detail"] = ex.what();
    return core_abi::make_response(
        false, core_abi::error_code::kParamInvalidInputPath,
        "Failed to enumerate input files.", std::move(data));
  }

  if (files.empty()) {
    Json data;
    data["input_path"] = kInputPath;
    return core_abi::make_response(
        false, core_abi::error_code::kBusinessNoInputFiles,
        "No .json files found under input_path.", std::move(data));
  }

  std::size_t parse_failures = 0;
  std::size_t matched_bills = 0;
  std::size_t transaction_count = 0;
  double total_income = 0.0;
  double total_expense = 0.0;
  double total_balance = 0.0;
  std::map<int, MonthlySummary> yearly_monthly_summary;
  std::map<std::string, double> income_by_parent;
  std::map<std::string, double> expense_by_parent;
  MonthlyReportData monthly_report_data;
  Json file_results = Json::array();

  monthly_report_data.year = query_year;
  monthly_report_data.month = query_month;
  monthly_report_data.data_found = false;

  for (const auto& file : files) {
    Json item;
    item["path"] = file.string();
    try {
      const ParsedBill kBillData = BillJsonSerializer::read_from_file(file.string());
      const bool kMatched =
          kIsYearQuery
              ? (kBillData.year == query_year)
              : (kBillData.year == query_year && kBillData.month == query_month);
      item["ok"] = true;
      item["matched"] = kMatched;

      if (kMatched) {
        ++matched_bills;
        transaction_count += kBillData.transactions.size();
        total_income += kBillData.total_income;
        total_expense += kBillData.total_expense;
        total_balance += kBillData.balance;
        monthly_report_data.data_found = true;

        item["date"] = kBillData.date;
        item["year"] = kBillData.year;
        item["month"] = kBillData.month;
        item["transaction_count"] = kBillData.transactions.size();

        if (kIsYearQuery) {
          auto& summary = yearly_monthly_summary[kBillData.month];
          summary.income += kBillData.total_income;
          summary.expense += kBillData.total_expense;
        }

        for (const auto& transaction : kBillData.transactions) {
          if (kIsMonthQuery) {
            auto& parent_data = monthly_report_data.aggregated_data[transaction.parent_category];
            parent_data.parent_total += transaction.amount;
            auto& sub_data = parent_data.sub_categories[transaction.sub_category];
            sub_data.sub_total += transaction.amount;
            sub_data.transactions.push_back(transaction);
          }

          const std::string kTxnType = ToAsciiLower(transaction.transaction_type);
          if (kTxnType == "income") {
            income_by_parent[transaction.parent_category] += transaction.amount;
          } else {
            expense_by_parent[transaction.parent_category] += transaction.amount;
          }
        }
      }
    } catch (const std::exception& ex) {
      item["ok"] = false;
      item["matched"] = false;
      item["error"] = ex.what();
      ++parse_failures;
    }

    file_results.push_back(std::move(item));
  }

  Json data;
  data["input_path"] = kInputPath;
  data["query_type"] = kIsYearQuery ? "year" : "month";
  data["query_value"] = kQueryValue;
  data["year"] = query_year;
  if (kIsMonthQuery) {
    data["month"] = query_month;
  }
  data["processed"] = files.size();
  data["parse_failures"] = parse_failures;
  data["matched_bills"] = matched_bills;
  data["transaction_count"] = transaction_count;
  data["total_income"] = total_income;
  data["total_expense"] = total_expense;
  data["balance"] = total_balance;

  monthly_report_data.total_income = total_income;
  monthly_report_data.total_expense = total_expense;
  monthly_report_data.balance = total_balance;

  Json category_totals = Json::object();
  Json income_categories = Json::object();
  for (const auto& [category, amount] : income_by_parent) {
    income_categories[category] = amount;
  }
  Json expense_categories = Json::object();
  for (const auto& [category, amount] : expense_by_parent) {
    expense_categories[category] = amount;
  }
  category_totals["income"] = std::move(income_categories);
  category_totals["expense"] = std::move(expense_categories);
  data["category_totals"] = std::move(category_totals);

  if (kIsYearQuery) {
    Json monthly = Json::array();
    for (const auto& [month, summary] : yearly_monthly_summary) {
      Json month_item;
      month_item["month"] = month;
      month_item["income"] = summary.income;
      month_item["expense"] = summary.expense;
      month_item["balance"] = summary.income - summary.expense;
      monthly.push_back(std::move(month_item));
    }
    data["monthly_summary"] = std::move(monthly);
  }

  if (kIsMonthQuery) {
    const auto kStandardReport =
        StandardReportAssembler::FromMonthly(monthly_report_data);
    data["standard_report"] = StandardReportJsonSerializer::ToJson(kStandardReport);
  } else {
    YearlyReportData yearly_report_data;
    yearly_report_data.year = query_year;
    yearly_report_data.data_found = (matched_bills > 0U);
    yearly_report_data.total_income = total_income;
    yearly_report_data.total_expense = total_expense;
    yearly_report_data.balance = total_balance;
    yearly_report_data.monthly_summary = std::move(yearly_monthly_summary);

    const auto kStandardReport =
        StandardReportAssembler::FromYearly(yearly_report_data);
    data["standard_report"] = StandardReportJsonSerializer::ToJson(kStandardReport);
  }

  data["files"] = std::move(file_results);

  if (matched_bills == 0U) {
    return core_abi::make_response(
        false, core_abi::error_code::kBusinessQueryNotFound,
        "No data matched the query.", std::move(data));
  }

  if (parse_failures == 0U) {
    return core_abi::make_response(
        true, core_abi::error_code::kOk, "Query completed successfully.",
        std::move(data));
  }

  return core_abi::make_response(
      false, core_abi::error_code::kBusinessQueryFailed,
      "Query completed with partial failures.", std::move(data));
}

}  // namespace bills::core::abi
