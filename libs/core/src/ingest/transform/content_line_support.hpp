#ifndef INGEST_TRANSFORM_CONTENT_LINE_SUPPORT_HPP_
#define INGEST_TRANSFORM_CONTENT_LINE_SUPPORT_HPP_

#include <optional>
#include <string>
#include <string_view>

namespace bills::core::ingest::content_line {

struct ParsedContentLine {
  std::string amount_expression;
  double evaluated_amount = 0.0;
  std::string description;
  std::string comment;
};

[[nodiscard]] auto EvaluateAmountExpression(std::string_view parent_category,
                                            std::string_view math_expr) -> double;

[[nodiscard]] auto ParseStructuredEntryLine(std::string_view parent_category,
                                            std::string_view line)
    -> std::optional<ParsedContentLine>;

[[nodiscard]] auto SerializeStructuredEntryLine(std::string_view amount_expression,
                                                std::string_view description,
                                                std::string_view comment)
    -> std::string;

}  // namespace bills::core::ingest::content_line

#endif  // INGEST_TRANSFORM_CONTENT_LINE_SUPPORT_HPP_
