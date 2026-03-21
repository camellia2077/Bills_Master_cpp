// reporting/standard_report/standard_report_json_serializer.cpp
#include "reporting/standard_report/standard_report_json_serializer.hpp"

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

auto StandardReportJsonSerializer::FromJson(
    const nlohmann::ordered_json& report_json) -> StandardReport {
  StandardReport report;

  if (const auto meta_it = report_json.find("meta");
      meta_it != report_json.end() && meta_it->is_object()) {
    const auto& meta = *meta_it;
    report.schema_version =
        meta.value("schema_version", report.schema_version);
    report.report_type = meta.value("report_type", "");
    report.generated_at_utc = meta.value("generated_at_utc", "");
    report.source = meta.value("source", report.source);
  }

  if (const auto scope_it = report_json.find("scope");
      scope_it != report_json.end() && scope_it->is_object()) {
    const auto& scope = *scope_it;
    report.period_start = scope.value("period_start", "");
    report.period_end = scope.value("period_end", "");
    report.remark = scope.value("remark", "");
    report.data_found = scope.value("data_found", false);
  }

  if (const auto summary_it = report_json.find("summary");
      summary_it != report_json.end() && summary_it->is_object()) {
    const auto& summary = *summary_it;
    report.total_income = summary.value("total_income", 0.0);
    report.total_expense = summary.value("total_expense", 0.0);
    report.balance = summary.value("balance", 0.0);
  }

  if (const auto items_it = report_json.find("items");
      items_it != report_json.end() && items_it->is_object()) {
    const auto& items = *items_it;

    if (const auto categories_it = items.find("categories");
        categories_it != items.end() && categories_it->is_array()) {
      for (const auto& category_json : *categories_it) {
        if (!category_json.is_object()) {
          continue;
        }

        StandardCategoryItem category;
        category.name = category_json.value("name", "");
        category.total = category_json.value("total", 0.0);

        if (const auto sub_categories_it = category_json.find("sub_categories");
            sub_categories_it != category_json.end() &&
            sub_categories_it->is_array()) {
          for (const auto& sub_category_json : *sub_categories_it) {
            if (!sub_category_json.is_object()) {
              continue;
            }

            StandardSubCategoryItem sub_category;
            sub_category.name = sub_category_json.value("name", "");
            sub_category.subtotal = sub_category_json.value("subtotal", 0.0);

            if (const auto transactions_it =
                    sub_category_json.find("transactions");
                transactions_it != sub_category_json.end() &&
                transactions_it->is_array()) {
              for (const auto& transaction_json : *transactions_it) {
                if (!transaction_json.is_object()) {
                  continue;
                }

                StandardTransactionItem transaction;
                transaction.parent_category =
                    transaction_json.value("parent_category", "");
                transaction.sub_category =
                    transaction_json.value("sub_category", "");
                transaction.transaction_type =
                    transaction_json.value("transaction_type", "");
                transaction.description =
                    transaction_json.value("description", "");
                transaction.source = transaction_json.value("source", "");
                transaction.comment = transaction_json.value("comment", "");
                transaction.amount = transaction_json.value("amount", 0.0);
                sub_category.transactions.push_back(std::move(transaction));
              }
            }

            category.sub_categories.push_back(std::move(sub_category));
          }
        }

        report.categories.push_back(std::move(category));
      }
    }

    if (const auto monthly_summary_it = items.find("monthly_summary");
        monthly_summary_it != items.end() && monthly_summary_it->is_array()) {
      for (const auto& month_json : *monthly_summary_it) {
        if (!month_json.is_object()) {
          continue;
        }

        StandardMonthlySummaryItem month_item;
        month_item.month = month_json.value("month", 0);
        month_item.income = month_json.value("income", 0.0);
        month_item.expense = month_json.value("expense", 0.0);
        month_item.balance = month_json.value("balance", 0.0);
        report.monthly_summary.push_back(std::move(month_item));
      }
    }
  }

  return report;
}

auto StandardReportJsonSerializer::FromString(const std::string& report_json)
    -> StandardReport {
  return FromJson(nlohmann::ordered_json::parse(report_json));
}
