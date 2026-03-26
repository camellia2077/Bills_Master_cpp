#ifndef COMMON_VALIDATION_ISSUE_HPP_
#define COMMON_VALIDATION_ISSUE_HPP_

#include <string>

struct ValidationIssue {
  std::string source_kind;
  std::string stage;
  std::string code;
  std::string message;
  std::string path;
  int line = 0;
  int column = 0;
  std::string field_path;
  std::string severity = "error";
};

inline auto MakeValidationIssue(std::string source_kind, std::string stage,
                                std::string code, std::string message,
                                std::string path = {},
                                int line = 0, int column = 0,
                                std::string field_path = {},
                                std::string severity = "error")
    -> ValidationIssue {
  return ValidationIssue{
      .source_kind = std::move(source_kind),
      .stage = std::move(stage),
      .code = std::move(code),
      .message = std::move(message),
      .path = std::move(path),
      .line = line,
      .column = column,
      .field_path = std::move(field_path),
      .severity = std::move(severity),
  };
}

#endif  // COMMON_VALIDATION_ISSUE_HPP_
