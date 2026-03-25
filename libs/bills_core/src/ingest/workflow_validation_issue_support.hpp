#ifndef INGEST_WORKFLOW_VALIDATION_ISSUE_SUPPORT_HPP_
#define INGEST_WORKFLOW_VALIDATION_ISSUE_SUPPORT_HPP_

#include <string>
#include <string_view>
#include <vector>

#include "common/validation_issue.hpp"

namespace bills::core::ingest {

inline auto MapWorkflowIssueStage(std::string_view detailed_stage)
    -> std::string_view {
  if (detailed_stage == "validate_bill") {
    return "business";
  }
  if (detailed_stage == "insert_repository") {
    return "system";
  }
  return "parse";
}

inline auto BuildWorkflowIssueCode(std::string_view detailed_stage)
    -> std::string {
  const std::string normalized_stage =
      detailed_stage.empty() ? "validation_failed" : std::string(detailed_stage);
  return "bill." + std::string(MapWorkflowIssueStage(detailed_stage)) + "." +
         normalized_stage;
}

inline auto BuildWorkflowIssues(
    std::string_view detailed_stage, std::string_view fallback_message,
    const std::vector<std::string>& messages, const std::string& display_path,
    std::string_view source_kind = "record_txt") -> std::vector<ValidationIssue> {
  const std::string normalized_stage =
      detailed_stage.empty() ? "validation_failed" : std::string(detailed_stage);
  const std::string_view mapped_stage = MapWorkflowIssueStage(normalized_stage);

  std::vector<ValidationIssue> issues;
  if (messages.empty()) {
    issues.push_back(MakeValidationIssue(
        std::string(source_kind), std::string(mapped_stage),
        BuildWorkflowIssueCode(normalized_stage),
        fallback_message.empty() ? "Validation failed."
                                 : std::string(fallback_message),
        display_path));
    return issues;
  }

  issues.reserve(messages.size());
  for (const auto& message : messages) {
    issues.push_back(MakeValidationIssue(
        std::string(source_kind), std::string(mapped_stage),
        BuildWorkflowIssueCode(normalized_stage), message, display_path));
  }
  return issues;
}

}  // namespace bills::core::ingest

#endif  // INGEST_WORKFLOW_VALIDATION_ISSUE_SUPPORT_HPP_
