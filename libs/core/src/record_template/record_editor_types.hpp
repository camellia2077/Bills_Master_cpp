#ifndef RECORD_TEMPLATE_RECORD_EDITOR_TYPES_HPP_
#define RECORD_TEMPLATE_RECORD_EDITOR_TYPES_HPP_

#include <expected>
#include <string>
#include <vector>

enum class RecordEditorErrorCategory {
  kParse,
  kSerialize,
};

struct RecordEditorError {
  RecordEditorErrorCategory category = RecordEditorErrorCategory::kParse;
  std::string message;
  int line = 0;
};

template <typename T>
using RecordEditorResult = std::expected<T, RecordEditorError>;

// Shared document shape for the structure-locked editor. The UI may build
// richer draft-only state on top of this, but title/section persistence should
// round-trip through this model.
struct RecordEditorEntry {
  std::string amount_expression;
  std::string description;
  std::string comment;
};

struct RecordEditorSubSection {
  std::string title;
  std::vector<RecordEditorEntry> entries;
};

struct RecordEditorParentSection {
  std::string title;
  std::vector<RecordEditorSubSection> sub_sections;
};

struct RecordEditorDocument {
  std::string date_line;
  std::vector<std::string> remark_lines;
  std::vector<RecordEditorParentSection> sections;
};

#endif  // RECORD_TEMPLATE_RECORD_EDITOR_TYPES_HPP_
