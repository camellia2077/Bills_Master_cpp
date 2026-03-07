// serialization/bills_json_serializer.cpp

#include "bills_json_serializer.hpp"

#include <cctype>
#include <fstream>
#include <iomanip>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace {
constexpr int kIndentSpaces = 4;
constexpr int kMoneyPrecision = 2;
constexpr std::size_t kIsoMonthLength = 7U;
constexpr std::size_t kYearDigits = 4U;
constexpr std::size_t kMonthStart = 5U;
constexpr std::size_t kMonthDigits = 2U;
constexpr int kMinMonth = 1;
constexpr int kMaxMonth = 12;

struct IsoMonth {
  int year = 0;
  int month = 0;
};

auto FormatMoney(double value) -> nlohmann::json {
  std::stringstream money_stream;
  money_stream << std::fixed << std::setprecision(kMoneyPrecision) << value;
  return nlohmann::json::parse(money_stream.str());
}

auto IsAsciiDigit(char character) -> bool {
  return std::isdigit(static_cast<unsigned char>(character)) != 0;
}

auto ParseIsoMonth(const std::string& date) -> std::optional<IsoMonth> {
  if (date.size() != kIsoMonthLength || date[kYearDigits] != '-') {
    return std::nullopt;
  }
  if (!IsAsciiDigit(date[0]) || !IsAsciiDigit(date[1]) ||
      !IsAsciiDigit(date[2]) || !IsAsciiDigit(date[3]) ||
      !IsAsciiDigit(date[kMonthStart]) ||
      !IsAsciiDigit(date[kMonthStart + 1U])) {
    return std::nullopt;
  }

  IsoMonth parsed_date;
  try {
    parsed_date.year = std::stoi(date.substr(0U, kYearDigits));
    parsed_date.month = std::stoi(date.substr(kMonthStart, kMonthDigits));
  } catch (...) {
    return std::nullopt;
  }

  if (parsed_date.month < kMinMonth || parsed_date.month > kMaxMonth) {
    return std::nullopt;
  }

  return parsed_date;
}
}  // namespace

auto BillJsonSerializer::read_from_file(const std::string& file_path)
    -> ParsedBill {
  std::ifstream file(file_path);
  if (!file.is_open()) {
    throw std::runtime_error("无法打开JSON文件: " + file_path);
  }

  nlohmann::json data;
  try {
    file >> data;
  } catch (const nlohmann::json::parse_error& e) {
    throw std::runtime_error("解析JSON失败: " + std::string(e.what()));
  }

  return deserialize(data);
}

auto BillJsonSerializer::serialize(const ParsedBill& bill_data) -> std::string {
  nlohmann::ordered_json root = to_json(bill_data);
  return root.dump(kIndentSpaces);
}

void BillJsonSerializer::write_to_file(const ParsedBill& bill_data,
                                       const std::string& file_path) {
  std::ofstream output_file(file_path);
  if (!output_file.is_open()) {
    throw std::runtime_error("无法打开输出账单文件 '" + file_path +
                             "' 进行写入。");
  }
  output_file << serialize(bill_data);
}

auto BillJsonSerializer::deserialize(const nlohmann::json& data) -> ParsedBill {
  ParsedBill bill_data{};

  try {
    const std::string kDateStr = data.at("date").get<std::string>();
    bill_data.date = kDateStr;
    bill_data.remark = data.at("remark").get<std::string>();

    bill_data.total_income = data.at("total_income").get<double>();
    bill_data.total_expense = data.at("total_expense").get<double>();
    bill_data.balance = data.at("balance").get<double>();

    const auto kParsedMonth = ParseIsoMonth(kDateStr);
    if (!kParsedMonth.has_value()) {
      throw std::runtime_error("JSON中的日期格式无效，必须为 YYYY-MM 格式。");
    }
    bill_data.year = kParsedMonth->year;
    bill_data.month = kParsedMonth->month;

    const auto& categories = data.at("categories");
    for (const auto& parent_item : categories.items()) {
      const std::string& parent_category = parent_item.key();
      const auto& parent_data = parent_item.value();

      if (!parent_data.contains("transactions") ||
          !parent_data.at("transactions").is_array()) {
        continue;
      }

      const auto& transactions_json = parent_data.at("transactions");
      for (const auto& item : transactions_json) {
        Transaction transaction{};
        transaction.parent_category = parent_category;
        transaction.sub_category = item.at("sub_category").get<std::string>();
        transaction.description = item.at("description").get<std::string>();
        transaction.amount = item.at("amount").get<double>();
        transaction.source = item.value("source", "manually_add");

        if (item.contains("comment") && !item.at("comment").is_null()) {
          transaction.comment = item.at("comment").get<std::string>();
        } else {
          transaction.comment = "";
        }

        transaction.transaction_type =
            item.value("transaction_type", "Expense");

        bill_data.transactions.push_back(transaction);
      }
    }
  } catch (const nlohmann::json::exception& e) {
    throw std::runtime_error("JSON 数据结构不符合预期: " +
                             std::string(e.what()));
  }

  return bill_data;
}

auto BillJsonSerializer::to_json(const ParsedBill& bill_data)
    -> nlohmann::ordered_json {
  nlohmann::ordered_json root;

  root["date"] = bill_data.date;
  root["remark"] = bill_data.remark;
  root["total_income"] = FormatMoney(bill_data.total_income);
  root["total_expense"] = FormatMoney(bill_data.total_expense);
  root["balance"] = FormatMoney(bill_data.balance);

  struct ParentAggregate {
    nlohmann::ordered_json transactions = nlohmann::ordered_json::array();
    double sub_total = 0.0;
  };

  std::unordered_map<std::string, ParentAggregate> parents;
  std::vector<std::string> parent_order;

  for (const auto& transaction : bill_data.transactions) {
    auto parent_it = parents.find(transaction.parent_category);
    if (parent_it == parents.end()) {
      parent_order.push_back(transaction.parent_category);
      parent_it =
          parents.emplace(transaction.parent_category, ParentAggregate{}).first;
    }

    nlohmann::ordered_json transaction_node;
    transaction_node["sub_category"] = transaction.sub_category;
    transaction_node["description"] = transaction.description;
    transaction_node["amount"] = transaction.amount;
    transaction_node["source"] = transaction.source;
    transaction_node["transaction_type"] = transaction.transaction_type;
    if (transaction.comment.empty()) {
      transaction_node["comment"] = nullptr;
    } else {
      transaction_node["comment"] = transaction.comment;
    }

    parent_it->second.transactions.push_back(transaction_node);
    parent_it->second.sub_total += transaction.amount;
  }

  nlohmann::ordered_json categories_obj = nlohmann::ordered_json::object();
  for (const auto& parent_title : parent_order) {
    const auto& aggregate = parents.at(parent_title);
    nlohmann::ordered_json parent_node;
    parent_node["display_name"] = parent_title;
    parent_node["sub_total"] = FormatMoney(aggregate.sub_total);
    parent_node["transactions"] = aggregate.transactions;
    categories_obj[parent_title] = parent_node;
  }

  root["categories"] = categories_obj;
  return root;
}
