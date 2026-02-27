#include "abi/internal/abi_shared.hpp"

#include <algorithm>
#include <cctype>

#include "serialization/bills_json_serializer.hpp"

namespace bills::core::abi {

namespace {

auto to_ascii_lower(std::string value) -> std::string {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char ch) -> char {
                   return static_cast<char>(std::tolower(ch));
                 });
  return value;
}

auto parse_year_string(const std::string& raw, int& year) -> bool {
  if (raw.size() != 4U) {
    return false;
  }
  try {
    year = std::stoi(raw);
  } catch (...) {
    return false;
  }
  return year >= 1900 && year <= 9999;
}

auto parse_month_string(const std::string& raw, int& year, int& month) -> bool {
  if (raw.size() != 6U) {
    return false;
  }

  try {
    year = std::stoi(raw.substr(0U, 4U));
    month = std::stoi(raw.substr(4U, 2U));
  } catch (...) {
    return false;
  }

  return year >= 1900 && year <= 9999 && month >= 1 && month <= 12;
}

struct QueryMonthTotals {
  double income = 0.0;
  double expense = 0.0;
};

}  // namespace

auto handle_query_command(const Json& request) -> std::string {
  const Json params = request.value("params", Json::object());
  if (!params.is_object()) {
    return make_response(false, error_code::kParamInvalidRequest,
                         "'params' must be a JSON object.");
  }

  const std::string query_type_raw = params.value("type", "");
  const std::string query_value = params.value("value", "");
  const std::string input_path =
      params.value("input_path", std::string(constants::kDefaultConvertOutputDir));

  if (query_type_raw.empty() || query_value.empty()) {
    return make_response(
        false, error_code::kParamInvalidRequest,
        "Query requires non-empty 'params.type' and 'params.value'.");
  }

  const std::string query_type = to_ascii_lower(query_type_raw);
  const bool is_year_query = (query_type == "year" || query_type == "y");
  const bool is_month_query = (query_type == "month" || query_type == "m");
  if (!is_year_query && !is_month_query) {
    Json data;
    data["type"] = query_type_raw;
    data["supported_types"] = {"year", "month"};
    return make_response(false, error_code::kParamInvalidRequest,
                         "Unsupported query type.", std::move(data));
  }

  int query_year = 0;
  int query_month = 0;
  bool parsed = false;
  if (is_year_query) {
    parsed = parse_year_string(query_value, query_year);
  } else {
    parsed = parse_month_string(query_value, query_year, query_month);
  }
  if (!parsed) {
    Json data;
    data["type"] = query_type_raw;
    data["value"] = query_value;
    return make_response(false, error_code::kParamInvalidRequest,
                         "Invalid query value format.", std::move(data));
  }

  std::vector<fs::path> files;
  try {
    files = list_json_files(fs::path(input_path));
  } catch (const std::exception& ex) {
    Json data;
    data["detail"] = ex.what();
    return make_response(false, error_code::kParamInvalidInputPath,
                         "Failed to enumerate input files.", std::move(data));
  }

  if (files.empty()) {
    Json data;
    data["input_path"] = input_path;
    return make_response(false, error_code::kBusinessNoInputFiles,
                         "No .json files found under input_path.",
                         std::move(data));
  }

  std::size_t parse_failures = 0;
  std::size_t matched_bills = 0;
  std::size_t transaction_count = 0;
  double total_income = 0.0;
  double total_expense = 0.0;
  double total_balance = 0.0;
  std::map<int, QueryMonthTotals> monthly_summary;
  std::map<std::string, double> income_by_parent;
  std::map<std::string, double> expense_by_parent;
  Json file_results = Json::array();

  for (const auto& file : files) {
    Json item;
    item["path"] = file.string();
    try {
      const ParsedBill bill_data = BillJsonSerializer::read_from_file(file.string());
      const bool matched =
          is_year_query
              ? (bill_data.year == query_year)
              : (bill_data.year == query_year && bill_data.month == query_month);
      item["ok"] = true;
      item["matched"] = matched;

      if (matched) {
        ++matched_bills;
        transaction_count += bill_data.transactions.size();
        total_income += bill_data.total_income;
        total_expense += bill_data.total_expense;
        total_balance += bill_data.balance;

        item["date"] = bill_data.date;
        item["year"] = bill_data.year;
        item["month"] = bill_data.month;
        item["transaction_count"] = bill_data.transactions.size();

        if (is_year_query) {
          auto& summary = monthly_summary[bill_data.month];
          summary.income += bill_data.total_income;
          summary.expense += bill_data.total_expense;
        }

        for (const auto& transaction : bill_data.transactions) {
          const std::string txn_type = to_ascii_lower(transaction.transaction_type);
          if (txn_type == "income") {
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
  data["input_path"] = input_path;
  data["query_type"] = is_year_query ? "year" : "month";
  data["query_value"] = query_value;
  data["year"] = query_year;
  if (is_month_query) {
    data["month"] = query_month;
  }
  data["processed"] = files.size();
  data["parse_failures"] = parse_failures;
  data["matched_bills"] = matched_bills;
  data["transaction_count"] = transaction_count;
  data["total_income"] = total_income;
  data["total_expense"] = total_expense;
  data["balance"] = total_balance;

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

  if (is_year_query) {
    Json monthly = Json::array();
    for (const auto& [month, summary] : monthly_summary) {
      Json month_item;
      month_item["month"] = month;
      month_item["income"] = summary.income;
      month_item["expense"] = summary.expense;
      month_item["balance"] = summary.income - summary.expense;
      monthly.push_back(std::move(month_item));
    }
    data["monthly_summary"] = std::move(monthly);
  }

  data["files"] = std::move(file_results);

  if (matched_bills == 0U) {
    return make_response(false, error_code::kBusinessQueryNotFound,
                         "No data matched the query.", std::move(data));
  }

  if (parse_failures == 0U) {
    return make_response(true, error_code::kOk, "Query completed successfully.",
                         std::move(data));
  }

  return make_response(false, error_code::kBusinessQueryFailed,
                       "Query completed with partial failures.",
                       std::move(data));
}

}  // namespace bills::core::abi
