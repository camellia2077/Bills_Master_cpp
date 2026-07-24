#include "ingest/transform/content_line_support.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <string>
#include <string_view>

namespace {
constexpr std::string_view kDoubleSlashCommentDelimiter = "//";
constexpr unsigned char kUtf8TimesByte1 = 0xC3;
constexpr unsigned char kUtf8TimesByte2 = 0x97;

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

auto find_comment_start(std::string_view text)
    -> std::pair<std::size_t, std::size_t> {
  for (std::size_t index = 0; index < text.size(); ++index) {
    if (text.substr(index).starts_with(kDoubleSlashCommentDelimiter)) {
      return {index, kDoubleSlashCommentDelimiter.size()};
    }
    const char current = text[index];
    if ((current == '#' || current == ';') && index > 0U &&
        std::isspace(static_cast<unsigned char>(text[index - 1U])) != 0) {
      return {index, 1U};
    }
  }
  return {std::string_view::npos, 0U};
}

auto parse_description_and_comment(std::string_view text,
                                   std::string& description,
                                   std::string& comment) -> void {
  const auto [comment_pos, delimiter_length] = find_comment_start(text);
  if (comment_pos != std::string_view::npos) {
    description = trim_copy(text.substr(0U, comment_pos));
    comment = trim_copy(text.substr(comment_pos + delimiter_length));
    return;
  }
  description = trim_copy(text);
  comment.clear();
}

auto is_income_parent(std::string_view parent_category) -> bool {
  std::string parent_lower(parent_category);
  std::transform(parent_lower.begin(), parent_lower.end(), parent_lower.begin(),
                 [](unsigned char character) -> char {
                   return static_cast<char>(std::tolower(character));
                 });
  return parent_lower == "income";
}

class ExpressionParser {
 public:
  explicit ExpressionParser(std::string_view source) : source_(source) {}

  auto parse(double& value, std::size_t& expression_end) -> bool {
    index_ = 0U;
    first_non_space_char_ = '\0';
    saw_first_non_space_ = false;
    has_error_ = false;

    if (!parse_expression(value)) {
      return false;
    }
    skip_spaces();
    expression_end = index_;
    return expression_end > 0U;
  }

  [[nodiscard]] auto first_non_space_char() const -> char {
    return first_non_space_char_;
  }

 private:
  auto parse_expression(double& value) -> bool {
    if (!parse_term(value)) {
      return false;
    }

    while (true) {
      skip_spaces();
      if (peek() == '+' || peek() == '-') {
        const char op = source_[index_++];
        double rhs = 0.0;
        if (!parse_term(rhs)) {
          return false;
        }
        value = (op == '+') ? value + rhs : value - rhs;
        continue;
      }
      break;
    }

    return !has_error_;
  }

  auto parse_term(double& value) -> bool {
    if (!parse_factor(value)) {
      return false;
    }

    while (true) {
      skip_spaces();
      if (peek() == '*' || peek() == '/' || starts_with_utf8_times()) {
        const char op = starts_with_utf8_times() ? '*' : source_[index_];
        advance_operator(op);
        double rhs = 0.0;
        if (!parse_factor(rhs)) {
          return false;
        }
        if (op == '*') {
          value *= rhs;
        } else {
          if (std::abs(rhs) <= std::numeric_limits<double>::epsilon()) {
            has_error_ = true;
            return false;
          }
          value /= rhs;
        }
        continue;
      }
      break;
    }

    return !has_error_;
  }

  auto parse_factor(double& value) -> bool {
    skip_spaces();
    if (!record_first_non_space()) {
      return false;
    }

    double sign = 1.0;
    while (peek() == '+' || peek() == '-') {
      if (source_[index_] == '-') {
        sign = -sign;
      }
      ++index_;
      skip_spaces();
      if (peek() == '\0') {
        return false;
      }
    }

    if (peek() == '(') {
      ++index_;
      double nested = 0.0;
      if (!parse_expression(nested)) {
        return false;
      }
      skip_spaces();
      if (peek() != ')') {
        return false;
      }
      ++index_;
      value = sign * nested;
      return true;
    }

    return parse_number(value, sign);
  }

  auto parse_number(double& value, double sign) -> bool {
    skip_spaces();
    const std::size_t start = index_;
    bool saw_digit = false;
    bool saw_dot = false;

    while (index_ < source_.size()) {
      const char current = source_[index_];
      if (std::isdigit(static_cast<unsigned char>(current)) != 0) {
        saw_digit = true;
        ++index_;
        continue;
      }
      if (current == '.' && !saw_dot) {
        saw_dot = true;
        ++index_;
        continue;
      }
      break;
    }

    if (!saw_digit) {
      index_ = start;
      return false;
    }

    try {
      value = sign * std::stod(std::string(source_.substr(start, index_ - start)));
      return true;
    } catch (...) {
      index_ = start;
      return false;
    }
  }

  auto record_first_non_space() -> bool {
    if (saw_first_non_space_) {
      return true;
    }
    skip_spaces();
    if (peek() == '\0') {
      return false;
    }
    first_non_space_char_ = source_[index_];
    saw_first_non_space_ = true;
    return true;
  }

  auto starts_with_utf8_times() const -> bool {
    return index_ + 1U < source_.size() &&
           static_cast<unsigned char>(source_[index_]) == kUtf8TimesByte1 &&
           static_cast<unsigned char>(source_[index_ + 1U]) == kUtf8TimesByte2;
  }

  auto advance_operator(char op) -> void {
    if (op == '*' && starts_with_utf8_times()) {
      index_ += 2U;
      return;
    }
    ++index_;
  }

  auto skip_spaces() -> void {
    while (index_ < source_.size() &&
           std::isspace(static_cast<unsigned char>(source_[index_])) != 0) {
      ++index_;
    }
  }

  [[nodiscard]] auto peek() const -> char {
    if (index_ >= source_.size()) {
      return '\0';
    }
    return source_[index_];
  }

  std::string_view source_;
  std::size_t index_ = 0U;
  char first_non_space_char_ = '\0';
  bool saw_first_non_space_ = false;
  bool has_error_ = false;
};

}  // namespace

namespace bills::core::ingest::content_line {

auto EvaluateAmountExpression(std::string_view parent_category,
                              std::string_view math_expr) -> double {
  ExpressionParser parser(math_expr);
  double parsed_value = 0.0;
  std::size_t expression_end = 0U;
  if (!parser.parse(parsed_value, expression_end)) {
    return 0.0;
  }

  if (expression_end != math_expr.size()) {
    return 0.0;
  }

  const char first_char = parser.first_non_space_char();
  const bool has_explicit_sign = first_char == '+' || first_char == '-';
  if (has_explicit_sign || is_income_parent(parent_category)) {
    return parsed_value;
  }
  return -parsed_value;
}

auto ParseStructuredEntryLine(std::string_view parent_category,
                              std::string_view line)
    -> std::optional<ParsedContentLine> {
  ExpressionParser parser(line);
  double parsed_value = 0.0;
  std::size_t expression_end = 0U;
  if (!parser.parse(parsed_value, expression_end)) {
    return std::nullopt;
  }

  ParsedContentLine parsed;
  parsed.amount_expression = trim_copy(line.substr(0U, expression_end));
  if (parsed.amount_expression.empty()) {
    return std::nullopt;
  }
  parsed.evaluated_amount = EvaluateAmountExpression(parent_category, parsed.amount_expression);
  parse_description_and_comment(line.substr(expression_end), parsed.description,
                                parsed.comment);
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
