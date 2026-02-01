// conversion/modifier/processor/BillParser.cpp

#include "BillParser.hpp"

#include <algorithm>
#include <cctype>
#include <ranges>
#include <regex>
#include <stdexcept>
#include <string_view>

namespace {
constexpr std::string_view kDatePrefix = "date:";
constexpr std::string_view kRemarkPrefix = "remark:";
constexpr std::string_view kDefaultParentTitle = "Default Parent";
constexpr std::string_view kDefaultSource = "manually_add";
constexpr std::string_view kIncomeType = "Income";
constexpr std::string_view kExpenseType = "Expense";
constexpr std::size_t kExpectedMatchSize = 3U;
constexpr std::size_t kYearLength = 4U;
constexpr std::size_t kMonthLength = 2U;
constexpr std::size_t kDateLength = kYearLength + kMonthLength;
}  // namespace

BillParser::BillParser(const Config& config) : m_config(config) {}

auto BillParser::parse(const std::vector<std::string>& lines) const
    -> ParsedBill {
  ParsedBill bill_data{};

  std::vector<std::string> temp_lines;
  temp_lines.reserve(lines.size());

  for (const auto& line : lines) {
    if (_is_metadata_line(line)) {
      if (line.starts_with(kDatePrefix)) {
        bill_data.date = line.substr(kDatePrefix.size());
      } else if (line.starts_with(kRemarkPrefix)) {
        bill_data.remark = line.substr(kRemarkPrefix.size());
      }
      continue;
    }

    std::string temp = line;
    if (!_trim(temp).empty()) {
      temp_lines.push_back(temp);
    }
  }

  struct SubGroup {
    std::string title;
    std::vector<std::string> contents;
  };

  struct ParentGroup {
    std::string title;
    std::vector<SubGroup> sub_items;
  };

  std::vector<ParentGroup> structure;
  ParentGroup* current_parent = nullptr;
  SubGroup* current_sub_item = nullptr;

  for (const std::string& line : temp_lines) {
    if (_is_title(line)) {
      if (_is_parent_title(line)) {
        structure.emplace_back();
        current_parent = &structure.back();
        current_parent->title = line;
        current_sub_item = nullptr;
      } else {
        if (current_parent == nullptr) {
          structure.emplace_back();
          current_parent = &structure.back();
          current_parent->title = std::string(kDefaultParentTitle);
        }
        current_parent->sub_items.emplace_back();
        current_sub_item = &current_parent->sub_items.back();
        current_sub_item->title = line;
      }
    } else if (current_sub_item != nullptr) {
      current_sub_item->contents.push_back(line);
    }
  }

  for (auto& parent : structure) {
    std::erase_if(parent.sub_items,
                  [](const SubGroup& sub) -> bool {
                    return sub.contents.empty();
                  });
  }

  std::erase_if(structure, [](const ParentGroup& parent) -> bool {
    return parent.sub_items.empty();
  });

  for (auto& parent : structure) {
    for (auto& sub_item : parent.sub_items) {
      std::ranges::sort(
          sub_item.contents,
          [](const std::string& a, const std::string& b) -> bool {
            double val_a = _get_numeric_value_from_content(a);
            double val_b = _get_numeric_value_from_content(b);
            if (val_a != val_b) {
              return val_a > val_b;
            }
            return a < b;
          });

      for (const auto& content_line : sub_item.contents) {
        Transaction transaction{};
        transaction.parent_category = parent.title;
        transaction.sub_category = sub_item.title;

        _parse_content_line(content_line, transaction.amount,
                            transaction.description, transaction.comment);

        transaction.source = std::string(kDefaultSource);
        transaction.transaction_type =
            (transaction.amount >= 0.0) ? std::string(kIncomeType)
                                        : std::string(kExpenseType);

        bill_data.transactions.push_back(transaction);

        if (transaction.amount >= 0.0) {
          bill_data.total_income += transaction.amount;
        } else {
          bill_data.total_expense += transaction.amount;
        }
      }
    }
  }

  bill_data.balance = bill_data.total_income + bill_data.total_expense;

  if (!bill_data.date.empty()) {
    if (bill_data.date.size() != kDateLength) {
      throw std::runtime_error("账单日期格式无效，必须为 YYYYMM。");
    }
    bill_data.year =
        std::stoi(bill_data.date.substr(0, kYearLength));
    bill_data.month =
        std::stoi(bill_data.date.substr(kYearLength, kMonthLength));
  }

  return bill_data;
}

auto BillParser::_is_metadata_line(const std::string& line) const -> bool {
  for (const auto& prefix : m_config.metadata_prefixes) {
    if (line.starts_with(prefix)) {
      return true;
    }
  }
  return false;
}

auto BillParser::_is_parent_title(const std::string& line) -> bool {
  return line.find('_') == std::string::npos;
}

auto BillParser::_is_title(const std::string& line) -> bool {
  if (line.empty()) {
    return false;
  }
  for (char c : line) {
    if (isspace(c) == 0) {
      return std::isalpha(static_cast<unsigned char>(c)) != 0;
    }
  }
  return false;
}

auto BillParser::_trim(std::string& s) -> std::string& {
  s.erase(s.begin(), std::ranges::find_if(s, [](unsigned char ch) -> bool {
            return !std::isspace(ch);
          }));
  s.erase(std::ranges::find_if(
              std::ranges::reverse_view(s),
              [](unsigned char ch) -> bool { return !std::isspace(ch); })
              .base(),
          s.end());
  return s;
}

void BillParser::_parse_content_line(const std::string& line, double& amount,
                                     std::string& description,
                                     std::string& comment) {
  std::smatch match;
  std::regex re(R"(^(-?\d+(?:\.\d+)?)\s*(.*))");

  std::string full_description_part;

  if (std::regex_match(line, match, re) && match.size() == kExpectedMatchSize) {
    try {
      amount = std::stod(match[1].str());
      full_description_part = match[2].str();
    } catch (const std::exception&) {
      amount = 0.0;
      full_description_part = line;
    }
  } else {
    amount = 0.0;
    full_description_part = line;
  }

  const std::string_view comment_delimiter = "//";
  const std::size_t comment_pos =
      full_description_part.find(comment_delimiter);
  if (comment_pos != std::string::npos) {
    comment = full_description_part.substr(comment_pos + comment_delimiter.size());
    description = full_description_part.substr(0, comment_pos);
  } else {
    description = full_description_part;
    comment = "";
  }

  if (!description.empty()) {
    const auto end_pos = description.find_last_not_of(" \t\n\r");
    if (end_pos != std::string::npos) {
      description.erase(end_pos + 1U);
    } else {
      description.clear();
    }
  }
  comment.erase(0, comment.find_first_not_of(" \t\n\r"));
}

auto BillParser::_get_numeric_value_from_content(
    const std::string& content_line) -> double {
  try {
    size_t pos = 0U;
    double val = std::stod(content_line, &pos);
    return (pos == 0U) ? 0.0 : val;
  } catch (const std::invalid_argument&) {
    return 0.0;
  }
}
