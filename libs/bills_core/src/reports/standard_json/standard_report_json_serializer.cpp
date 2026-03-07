// reports/standard_json/standard_report_json_serializer.cpp
#include "reports/standard_json/standard_report_json_serializer.hpp"

auto StandardReportJsonSerializer::ToJson(const StandardReport& report)
    -> nlohmann::ordered_json {
  nlohmann::ordered_json root;

  root["meta"] = {
      {"schema_version", report.schema_version},
      {"report_type", report.report_type},
      {"generated_at_utc", report.generated_at_utc},
      {"source", report.source},
  };

  root["scope"] = {
      {"period_start", report.period_start},
      {"period_end", report.period_end},
      {"remark", report.remark},
      {"data_found", report.data_found},
  };

  root["summary"] = {
      {"total_income", report.total_income},
      {"total_expense", report.total_expense},
      {"balance", report.balance},
  };

  nlohmann::ordered_json categories_json = nlohmann::ordered_json::array();
  for (const auto& category : report.categories) {
    nlohmann::ordered_json sub_categories_json = nlohmann::ordered_json::array();
    for (const auto& sub_category : category.sub_categories) {
      nlohmann::ordered_json transactions_json = nlohmann::ordered_json::array();
      for (const auto& tx : sub_category.transactions) {
        transactions_json.push_back({
            {"parent_category", tx.parent_category},
            {"sub_category", tx.sub_category},
            {"transaction_type", tx.transaction_type},
            {"description", tx.description},
            {"source", tx.source},
            {"comment", tx.comment},
            {"amount", tx.amount},
        });
      }

      sub_categories_json.push_back({
          {"name", sub_category.name},
          {"subtotal", sub_category.subtotal},
          {"transactions", std::move(transactions_json)},
      });
    }

    categories_json.push_back({
        {"name", category.name},
        {"total", category.total},
        {"sub_categories", std::move(sub_categories_json)},
    });
  }

  nlohmann::ordered_json monthly_summary_json = nlohmann::ordered_json::array();
  for (const auto& month_item : report.monthly_summary) {
    monthly_summary_json.push_back({
        {"month", month_item.month},
        {"income", month_item.income},
        {"expense", month_item.expense},
        {"balance", month_item.balance},
    });
  }

  root["items"] = {
      {"categories", std::move(categories_json)},
      {"monthly_summary", std::move(monthly_summary_json)},
  };

  root["extensions"] = nlohmann::ordered_json::object();
  return root;
}

auto StandardReportJsonSerializer::ToString(const StandardReport& report)
    -> std::string {
  return ToJson(report).dump(2);
}
