#include <jni.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "io/host_flow_support.hpp"
#include "jni_common.hpp"

namespace {

namespace fs = std::filesystem;

using bills::android::jni::Json;

struct QueryMonth {
  int year = 0;
  int month = 0;
};

auto is_ascii_digits(std::string_view value) -> bool {
  return !value.empty() &&
         std::all_of(value.begin(), value.end(), [](unsigned char ch) -> bool {
           return std::isdigit(ch) != 0;
         });
}

auto parse_iso_year(const std::string& value) -> std::optional<int> {
  if (value.size() != 4U || !is_ascii_digits(value)) {
    return std::nullopt;
  }
  const int year = std::stoi(value);
  if (year < 1900 || year > 9999) {
    return std::nullopt;
  }
  return year;
}

auto parse_iso_month(const std::string& value) -> std::optional<QueryMonth> {
  if (value.size() != 7U || value[4] != '-' ||
      !is_ascii_digits(std::string_view(value).substr(0, 4)) ||
      !is_ascii_digits(std::string_view(value).substr(5, 2))) {
    return std::nullopt;
  }
  QueryMonth parsed;
  parsed.year = std::stoi(value.substr(0, 4));
  parsed.month = std::stoi(value.substr(5, 2));
  if (parsed.year < 1900 || parsed.year > 9999 ||
      parsed.month < 1 || parsed.month > 12) {
    return std::nullopt;
  }
  return parsed;
}

auto json_for_monthly_summary(const std::map<int, MonthlySummary>& monthly_summary)
    -> Json {
  Json summary = Json::array();
  for (const auto& [month, value] : monthly_summary) {
    Json entry;
    entry["month"] = month;
    entry["income"] = value.income;
    entry["expense"] = value.expense;
    entry["balance"] = value.income + value.expense;
    summary.push_back(std::move(entry));
  }
  return summary;
}

auto attach_rendered_report_payload(Json& data,
                                    const bills::io::HostQueryResult& query_result) -> void {
  if (!query_result.standard_report_json.empty()) {
    data["standard_report"] = Json::parse(query_result.standard_report_json);
  }
  if (!query_result.report_markdown.empty()) {
    data["report_markdown"] = query_result.report_markdown;
  }
}

auto is_missing_bills_table_error(std::string_view message) -> bool {
  return message.find("no such table: bills") != std::string_view::npos;
}

auto list_available_periods(const std::string& db_path) -> std::string {
  if (db_path.empty()) {
    return bills::android::jni::MakeResponse(false, "param.invalid_argument",
                                             "dbPath must be non-empty.");
  }

  Json data;
  data["db_path"] = db_path;
  data["periods"] = Json::array();
  data["count"] = 0;

  if (!fs::exists(fs::path(db_path))) {
    return bills::android::jni::MakeResponse(
        true, "ok", "No imported months found in database.", std::move(data));
  }

  try {
    const auto periods = bills::io::ListAvailableMonths(db_path);
    if (!periods) {
      throw std::runtime_error(FormatError(periods.error()));
    }
    data["periods"] = *periods;
    data["count"] = periods->size();
    return bills::android::jni::MakeResponse(
        true, "ok",
        periods->empty() ? "No imported months found in database."
                         : "Available query periods loaded successfully.",
        std::move(data));
  } catch (const std::exception& error) {
    if (is_missing_bills_table_error(error.what())) {
      return bills::android::jni::MakeResponse(
          true, "ok", "No imported months found in database.", std::move(data));
    }
    throw;
  }
}

auto query_year(const std::string& db_path, const std::string& iso_year)
    -> std::string {
  if (db_path.empty()) {
    return bills::android::jni::MakeResponse(false, "param.invalid_argument",
                                             "dbPath must be non-empty.");
  }
  if (!parse_iso_year(iso_year).has_value()) {
    return bills::android::jni::MakeResponse(false, "param.invalid_argument",
                                             "isoYear must use YYYY.");
  }

  const auto query_result = bills::io::QueryYearReport(db_path, iso_year);
  if (!query_result) {
    Json data;
    data["db_path"] = db_path;
    data["iso_year"] = iso_year;
    return bills::android::jni::MakeResponse(
        false, "system.native_failure", FormatError(query_result.error()),
        std::move(data));
  }
  if (!query_result->execution.data_found) {
    Json data;
    data["db_path"] = db_path;
    data["iso_year"] = iso_year;
    return bills::android::jni::MakeResponse(
        false, "business.query_not_found",
        "No data matched the requested year.", std::move(data));
  }

  Json data;
  data["query_type"] = "year";
  data["query_value"] = iso_year;
  data["year"] = query_result->execution.year;
  data["matched_bills"] = query_result->matched_bills;
  data["total_income"] = query_result->execution.yearly_data.total_income;
  data["total_expense"] = query_result->execution.yearly_data.total_expense;
  data["balance"] = query_result->execution.yearly_data.balance;
  data["monthly_summary"] =
      json_for_monthly_summary(query_result->execution.yearly_data.monthly_summary);
  attach_rendered_report_payload(data, *query_result);
  return bills::android::jni::MakeResponse(
      true, "ok", "Year query completed successfully.", std::move(data));
}

auto query_month(const std::string& db_path, const std::string& iso_month)
    -> std::string {
  if (db_path.empty()) {
    return bills::android::jni::MakeResponse(false, "param.invalid_argument",
                                             "dbPath must be non-empty.");
  }
  if (!parse_iso_month(iso_month).has_value()) {
    return bills::android::jni::MakeResponse(false, "param.invalid_argument",
                                             "isoMonth must use YYYY-MM.");
  }

  const auto query_result = bills::io::QueryMonthReport(db_path, iso_month);
  if (!query_result) {
    Json data;
    data["db_path"] = db_path;
    data["iso_month"] = iso_month;
    return bills::android::jni::MakeResponse(
        false, "system.native_failure", FormatError(query_result.error()),
        std::move(data));
  }
  if (!query_result->execution.data_found) {
    Json data;
    data["db_path"] = db_path;
    data["iso_month"] = iso_month;
    return bills::android::jni::MakeResponse(
        false, "business.query_not_found",
        "No data matched the requested month.", std::move(data));
  }

  Json data;
  data["query_type"] = "month";
  data["query_value"] = iso_month;
  data["year"] = query_result->execution.year;
  data["month"] = *query_result->execution.month;
  data["matched_bills"] = query_result->matched_bills;
  data["transaction_count"] = query_result->transaction_count;
  data["total_income"] = query_result->execution.monthly_data.total_income;
  data["total_expense"] = query_result->execution.monthly_data.total_expense;
  data["balance"] = query_result->execution.monthly_data.balance;
  data["remark"] = query_result->execution.monthly_data.remark;
  attach_rendered_report_payload(data, *query_result);
  return bills::android::jni::MakeResponse(
      true, "ok", "Month query completed successfully.", std::move(data));
}

}  // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_com_billstracer_android_data_nativebridge_QueryNativeBindings_listAvailablePeriodsNative(
    JNIEnv* env, jclass, jstring db_path) {
  return bills::android::jni::SafeCall(env, [&]() -> std::string {
    return list_available_periods(
        bills::android::jni::FromJString(env, db_path));
  });
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_billstracer_android_data_nativebridge_QueryNativeBindings_queryYearNative(
    JNIEnv* env, jclass, jstring db_path, jstring iso_year) {
  return bills::android::jni::SafeCall(env, [&]() -> std::string {
    return query_year(bills::android::jni::FromJString(env, db_path),
                      bills::android::jni::FromJString(env, iso_year));
  });
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_billstracer_android_data_nativebridge_QueryNativeBindings_queryMonthNative(
    JNIEnv* env, jclass, jstring db_path, jstring iso_month) {
  return bills::android::jni::SafeCall(env, [&]() -> std::string {
    return query_month(bills::android::jni::FromJString(env, db_path),
                       bills::android::jni::FromJString(env, iso_month));
  });
}
