#include <jni.h>
#include <sqlite3.h>

#include <algorithm>
#include <cctype>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

#include "bills_io/io_factory.hpp"
#include "jni_common.hpp"
#include "query/query_service.hpp"
#include "reporting/renderers/standard_report_renderer_registry.hpp"
#include "reporting/report_render_service.hpp"

namespace {

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

auto count_matching_year_bills(sqlite3* db_connection, std::string_view iso_year)
    -> int {
  const char* sql = "SELECT COUNT(*) FROM bills WHERE substr(bill_date, 1, 4) = ?;";
  sqlite3_stmt* statement = nullptr;
  if (sqlite3_prepare_v2(db_connection, sql, -1, &statement, nullptr) !=
      SQLITE_OK) {
    throw std::runtime_error("Failed to prepare bill count query.");
  }

  sqlite3_bind_text(statement, 1, iso_year.data(),
                    static_cast<int>(iso_year.size()), SQLITE_TRANSIENT);

  int result = 0;
  if (sqlite3_step(statement) == SQLITE_ROW) {
    result = sqlite3_column_int(statement, 0);
  }
  sqlite3_finalize(statement);
  return result;
}

auto count_matching_month_bills(sqlite3* db_connection,
                                std::string_view iso_month) -> int {
  const char* sql = "SELECT COUNT(*) FROM bills WHERE bill_date = ?;";
  sqlite3_stmt* statement = nullptr;
  if (sqlite3_prepare_v2(db_connection, sql, -1, &statement, nullptr) !=
      SQLITE_OK) {
    throw std::runtime_error("Failed to prepare bill count query.");
  }

  sqlite3_bind_text(statement, 1, iso_month.data(),
                    static_cast<int>(iso_month.size()), SQLITE_TRANSIENT);

  int result = 0;
  if (sqlite3_step(statement) == SQLITE_ROW) {
    result = sqlite3_column_int(statement, 0);
  }
  sqlite3_finalize(statement);
  return result;
}

auto count_transactions(const MonthlyReportData& report) -> std::size_t {
  std::size_t count = 0;
  for (const auto& [_, parent] : report.aggregated_data) {
    for (const auto& [__, sub] : parent.sub_categories) {
      count += sub.transactions.size();
    }
  }
  return count;
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
                                    const StandardReport& standard_report) -> void {
  if (StandardReportRendererRegistry::IsFormatAvailable("json")) {
    data["standard_report"] = Json::parse(
        StandardReportRendererRegistry::Render(standard_report, "json"));
  }
  if (StandardReportRendererRegistry::IsFormatAvailable("md")) {
    data["report_markdown"] =
        StandardReportRendererRegistry::Render(standard_report, "md");
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

  auto db_session = bills::io::CreateReportDbSession(db_path);
  auto report_data_gateway =
      bills::io::CreateReportDataGateway(db_session->GetConnectionHandle());
  const auto query_result = QueryService::QueryYear(*report_data_gateway, iso_year);
  if (!query_result.data_found) {
    Json data;
    data["db_path"] = db_path;
    data["iso_year"] = iso_year;
    return bills::android::jni::MakeResponse(
        false, "business.query_not_found",
        "No data matched the requested year.", std::move(data));
  }

  const auto standard_report = ReportRenderService::BuildStandardReport(query_result);
  Json data;
  data["query_type"] = "year";
  data["query_value"] = iso_year;
  data["year"] = query_result.year;
  data["matched_bills"] =
      count_matching_year_bills(db_session->GetConnectionHandle(), iso_year);
  data["total_income"] = query_result.yearly_data.total_income;
  data["total_expense"] = query_result.yearly_data.total_expense;
  data["balance"] = query_result.yearly_data.balance;
  data["monthly_summary"] =
      json_for_monthly_summary(query_result.yearly_data.monthly_summary);
  attach_rendered_report_payload(data, standard_report);
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

  auto db_session = bills::io::CreateReportDbSession(db_path);
  auto report_data_gateway =
      bills::io::CreateReportDataGateway(db_session->GetConnectionHandle());
  const auto query_result = QueryService::QueryMonth(*report_data_gateway, iso_month);
  if (!query_result.data_found) {
    Json data;
    data["db_path"] = db_path;
    data["iso_month"] = iso_month;
    return bills::android::jni::MakeResponse(
        false, "business.query_not_found",
        "No data matched the requested month.", std::move(data));
  }

  const auto standard_report = ReportRenderService::BuildStandardReport(query_result);
  Json data;
  data["query_type"] = "month";
  data["query_value"] = iso_month;
  data["year"] = query_result.year;
  data["month"] = *query_result.month;
  data["matched_bills"] =
      count_matching_month_bills(db_session->GetConnectionHandle(), iso_month);
  data["transaction_count"] = count_transactions(query_result.monthly_data);
  data["total_income"] = query_result.monthly_data.total_income;
  data["total_expense"] = query_result.monthly_data.total_expense;
  data["balance"] = query_result.monthly_data.balance;
  data["remark"] = query_result.monthly_data.remark;
  attach_rendered_report_payload(data, standard_report);
  return bills::android::jni::MakeResponse(
      true, "ok", "Month query completed successfully.", std::move(data));
}

}  // namespace

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
