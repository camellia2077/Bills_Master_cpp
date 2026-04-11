#include "ingest/transform/content_line_support.hpp"

#include <algorithm>
#include <cctype>
#include <regex>
#include <string>
#include <string_view>

namespace {
constexpr std::size_t kExpectedMatchSize = 3U;
constexpr std::string_view kCommentDelimiter = "//";

auto trim_copy(std::string_view raw) -> std::string {
  std::size_t start = 0;
  while (start < raw.size() &&
         std::isspace(static_cast<unsigned char>(raw[start])) != 0) {
    ++start;
  }
  std::size_t end = raw.size();
  while (end > start &&
         std::isspace(static_cast<unsigned char>(raw[end - 1U])) != 0) {
    --end;
  }
  return std::string(raw.substr(start, end - start));
}

auto parse_description_and_comment(std::string_view text,
                                   std::string& description,
                                   std::string& comment) -> void {
  const auto comment_pos = text.find(kCommentDelimiter);
  if (comment_pos != std::string_view::npos) {
    description = trim_copy(text.substr(0U, comment_pos));
    comment = trim_copy(
        text.substr(comment_pos + kCommentDelimiter.size()));
    return;
  }
  description = trim_copy(text);
  comment.clear();
}
}  // namespace

namespace bills::core::ingest::content_line {

auto EvaluateAmountExpression(std::string_view parent_category,
                              std::string_view math_expr) -> double {
  std::string expression(math_expr);
  expression.erase(
      std::remove_if(expression.begin(), expression.end(),
                     [](unsigned char character) -> bool {
                       return std::isspace(character) != 0;
                     }),
      expression.end());

  if (expression.empty()) {
    return 0.0;
  }

  bool has_explicit_plus = false;
  if (expression[0] == '+') {
    has_explicit_plus = true;
    expression = expression.substr(1U);
  } else if (expression[0] == '-') {
    expression = expression.substr(1U);
  }

  double sum = 0.0;
  while (!expression.empty()) {
    std::size_t consumed = 0;
    try {
      sum += std::stod(expression, &consumed);
      if (consumed == 0U) {
        break;
      }
      expression = expression.substr(consumed);
    } catch (...) {
      break;
    }
  }

  std::string parent_lower(parent_category);
  std::transform(
      parent_lower.begin(), parent_lower.end(), parent_lower.begin(),
      [](unsigned char character) -> char {
        return static_cast<char>(std::tolower(character));
      });

  if (parent_lower == "income" || has_explicit_plus) {
    return sum;
  }
  return -sum;
}

auto ParseStructuredEntryLine(std::string_view parent_category,
                              std::string_view line)
    -> std::optional<ParsedContentLine> {
  static const std::regex content_regex(
      R"(^([+-]?\s*\d+(?:\.\d+)?(?:\s*[+-]\s*\d+(?:\.\d+)?)*)\s*(.*))");

  std::smatch match;
  const std::string owned_line(line);
  if (!std::regex_match(owned_line, match, content_regex) ||
      match.size() != kExpectedMatchSize) {
    return std::nullopt;
  }

  ParsedContentLine parsed;
  parsed.amount_expression = trim_copy(match[1].str());
  parsed.amount = EvaluateAmountExpression(parent_category, parsed.amount_expression);
  parse_description_and_comment(match[2].str(), parsed.description, parsed.comment);
  return parsed;
}

auto SerializeStructuredEntryLine(std::string_view amount_expression,
                                  std::string_view description,
                                  std::string_view comment) -> std::string {
  const auto trimmed_amount = trim_copy(amount_expression);
  const auto trimmed_description = trim_copy(description);
  const auto trimmed_comment = trim_copy(comment);

  std::string line = trimmed_amount;
  if (!trimmed_description.empty()) {
    line += " " + trimmed_description;
  }
  if (!trimmed_comment.empty()) {
    line += " // " + trimmed_comment;
  }
  return line;
}

}  // namespace bills::core::ingest::content_line
