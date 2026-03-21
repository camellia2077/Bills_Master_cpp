// ingest/transform/bills_parser.cpp

#include "bills_parser.hpp"

#include <algorithm>
#include <cctype>
#include <ranges>
#include <regex>
#include <stdexcept>
#include <string_view>

#include "common/iso_period.hpp"

namespace {
constexpr std::string_view kDatePrefix = "date:";
constexpr std::string_view kRemarkPrefix = "remark:";
constexpr std::string_view kDefaultParentTitle = "Default Parent";
constexpr std::string_view kDefaultSource = "manually_add";
constexpr std::string_view kIncomeType = "Income";
constexpr std::string_view kExpenseType = "Expense";
constexpr std::size_t kExpectedMatchSize = 3U;
}  // namespace

BillParser::BillParser(const Config& config) : m_config(config) {}

// NOLINTNEXTLINE(readability-function-cognitive-complexity) -- parser keeps the bill text-to-structure flow in one place.
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
    std::erase_if(parent.sub_items, [](const SubGroup& sub) -> bool {
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
          [](const std::string& left_value,
             const std::string& right_value) -> bool {
            const double kLeftAmount =
                _get_numeric_value_from_content(left_value);
            const double kRightAmount =
                _get_numeric_value_from_content(right_value);
            if (kLeftAmount > kRightAmount) {
              return true;
            }
            if (kLeftAmount < kRightAmount) {
              return false;
            }
            return left_value < right_value;
          });

      for (const auto& content_line : sub_item.contents) {
        Transaction transaction{};
        transaction.parent_category = parent.title;
        transaction.sub_category = sub_item.title;

        _parse_content_line(content_line, transaction.amount,
                            transaction.description, transaction.comment);

        transaction.source = std::string(kDefaultSource);
        transaction.transaction_type = (transaction.amount >= 0.0)
                                           ? std::string(kIncomeType)
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
    const auto kParsedDate =
        bills::core::common::iso_period::parse_year_month(bill_data.date);
    if (!kParsedDate.has_value()) {
      throw std::runtime_error("账单日期格式无效，必须为 YYYY-MM。");
    }
    bill_data.year = kParsedDate->year;
    bill_data.month = kParsedDate->month;
    bill_data.date = bills::core::common::iso_period::format_year_month(
        bill_data.year, bill_data.month);
  }

  return bill_data;
}

auto BillParser::_is_metadata_line(const std::string& line) const -> bool {
  return std::ranges::any_of(
      m_config.metadata_prefixes,
      [&line](const std::string& prefix) -> bool {
        return line.starts_with(prefix);
      });
}

auto BillParser::_is_parent_title(const std::string& line) -> bool {
  return line.find('_') == std::string::npos;
}

auto BillParser::_is_title(const std::string& line) -> bool {
  if (line.empty()) {
    return false;
  }
  for (char character : line) {
    const auto kUnsignedCharacter = static_cast<unsigned char>(character);
    if (std::isspace(kUnsignedCharacter) == 0) {
      return std::isalpha(kUnsignedCharacter) != 0;
    }
  }
  return false;
}

auto BillParser::_trim(std::string& text) -> std::string& {
  text.erase(
      text.begin(),
      std::ranges::find_if(text, [](unsigned char character) -> bool {
        return !std::isspace(character);
      }));
  text.erase(std::ranges::find_if(
                 std::ranges::reverse_view(text),
                 [](unsigned char character) -> bool {
                   return !std::isspace(character);
                 })
                 .base(),
             text.end());
  return text;
}

void BillParser::_parse_content_line(const std::string& line, double& amount,
                                     std::string& description,
                                     std::string& comment) {
  std::smatch match;
  std::regex content_regex(R"(^(-?\d+(?:\.\d+)?)\s*(.*))");

  std::string full_description_part;

  if (std::regex_match(line, match, content_regex) &&
      match.size() == kExpectedMatchSize) {
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

  const std::string_view kCommentDelimiter = "//";
  const std::size_t kCommentPos = full_description_part.find(kCommentDelimiter);
  if (kCommentPos != std::string::npos) {
    comment =
        full_description_part.substr(kCommentPos + kCommentDelimiter.size());
    description = full_description_part.substr(0, kCommentPos);
  } else {
    description = full_description_part;
    comment = "";
  }

  if (!description.empty()) {
    const auto kEndPos = description.find_last_not_of(" \t\n\r");
    if (kEndPos != std::string::npos) {
      description.erase(kEndPos + 1U);
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
