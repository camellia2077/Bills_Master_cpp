#include "record_template/record_editor_service.hpp"

#include <cctype>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "common/iso_period.hpp"
#include "ingest/transform/content_line_support.hpp"

namespace {

struct ParsedTitleLine {
  std::string first_token;
  std::string remainder;
  bool starts_with_letter = false;
};

auto Trim(std::string_view raw) -> std::string {
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

auto TrimEndCarriageReturn(std::string_view raw) -> std::string_view {
  if (!raw.empty() && raw.back() == '\r') {
    return raw.substr(0U, raw.size() - 1U);
  }
  return raw;
}

auto ParseTitleLine(std::string_view line) -> ParsedTitleLine {
  const auto first_whitespace_index = line.find_first_of(" \t");
  const auto first_token = std::string(
      line.substr(0U, first_whitespace_index == std::string_view::npos
                          ? line.size()
                          : first_whitespace_index));
  std::string remainder;
  if (first_whitespace_index != std::string_view::npos) {
    remainder = Trim(line.substr(first_whitespace_index));
  }
  return ParsedTitleLine{
      .first_token = first_token,
      .remainder = std::move(remainder),
      .starts_with_letter =
          !line.empty() &&
          std::isalpha(static_cast<unsigned char>(line.front())) != 0,
  };
}

auto MakeParseError(std::string message, int line) -> RecordEditorError {
  return RecordEditorError{
      .category = RecordEditorErrorCategory::kParse,
      .message = std::move(message),
      .line = line,
  };
}

}  // namespace

auto RecordEditorService::Parse(std::string_view raw_text)
    -> RecordEditorResult<RecordEditorDocument> {
  RecordEditorDocument document;
  RecordEditorParentSection* current_parent = nullptr;
  RecordEditorSubSection* current_sub_section = nullptr;

  std::istringstream stream{std::string(raw_text)};
  std::string raw_line;
  int line_number = 0;
  while (std::getline(stream, raw_line)) {
    ++line_number;
    const auto normalized_line = TrimEndCarriageReturn(raw_line);
    const auto trimmed_line = Trim(normalized_line);

    if (trimmed_line.starts_with("date:")) {
      if (!bills::core::common::iso_period::extract_year_month_from_date_header(
               trimmed_line)
               .has_value()) {
        return std::unexpected(
            MakeParseError("Invalid date header. Expected 'date:YYYY-MM'.",
                           line_number));
      }
      document.date_line = trimmed_line;
      continue;
    }

    if (trimmed_line.starts_with("remark:")) {
      // Repeated remark headers are preserved as separate lines so the editor
      // can keep user-authored header text stable across round-trips.
      document.remark_lines.emplace_back(trimmed_line.substr(7U));
      continue;
    }

    if (trimmed_line.empty()) {
      continue;
    }

    const auto parsed_title_line = ParseTitleLine(trimmed_line);
    const bool is_pure_parent_title =
        parsed_title_line.starts_with_letter &&
        parsed_title_line.remainder.empty() &&
        !parsed_title_line.first_token.contains('_');
    const bool is_pure_sub_title =
        parsed_title_line.starts_with_letter &&
        parsed_title_line.remainder.empty() &&
        parsed_title_line.first_token.contains('_');
    const bool is_inline_sub_title =
        parsed_title_line.starts_with_letter &&
        !parsed_title_line.remainder.empty() &&
        parsed_title_line.first_token.contains('_');

    // Unlike the ingest parser, this service is intentionally conservative:
    // it keeps the title skeleton explicit for the editor and reports
    // structure errors instead of auto-healing them.
    if (is_pure_parent_title) {
      document.sections.push_back(RecordEditorParentSection{
          .title = parsed_title_line.first_token,
      });
      current_parent = &document.sections.back();
      current_sub_section = nullptr;
      continue;
    }

    if (is_pure_sub_title) {
      if (current_parent == nullptr) {
        return std::unexpected(MakeParseError(
            "A sub-title appeared before any parent title.", line_number));
      }
      current_parent->sub_sections.push_back(RecordEditorSubSection{
          .title = parsed_title_line.first_token,
      });
      current_sub_section = &current_parent->sub_sections.back();
      continue;
    }

    if (is_inline_sub_title) {
      if (current_parent == nullptr) {
        return std::unexpected(MakeParseError(
            "A sub-title with inline content appeared before any parent title.",
            line_number));
      }
      current_parent->sub_sections.push_back(RecordEditorSubSection{
          .title = parsed_title_line.first_token,
          .entries = {},
      });
      const auto parsed_entry =
          bills::core::ingest::content_line::ParseStructuredEntryLine(
              current_parent->title, parsed_title_line.remainder);
      if (!parsed_entry) {
        return std::unexpected(MakeParseError(
            "A sub-title inline content line is not supported by the structured editor.",
            line_number));
      }
      current_parent->sub_sections.back().entries.push_back(RecordEditorEntry{
          .amount_expression = parsed_entry->amount_expression,
          .description = parsed_entry->description,
          .comment = parsed_entry->comment,
      });
      current_sub_section = &current_parent->sub_sections.back();
      continue;
    }

    if (current_sub_section != nullptr) {
      // Once a sub-title is open, subsequent non-title lines belong to that
      // sub-section's content body.
      const auto parsed_entry =
          bills::core::ingest::content_line::ParseStructuredEntryLine(
              current_parent->title, trimmed_line);
      if (!parsed_entry) {
        return std::unexpected(MakeParseError(
            "A content line is not supported by the structured editor.",
            line_number));
      }
      current_sub_section->entries.push_back(RecordEditorEntry{
          .amount_expression = parsed_entry->amount_expression,
          .description = parsed_entry->description,
          .comment = parsed_entry->comment,
      });
      continue;
    }

    return std::unexpected(MakeParseError(
        "Content appeared before a recognized sub-title.", line_number));
  }

  return document;
}

auto RecordEditorService::Serialize(const RecordEditorDocument& document)
    -> RecordEditorResult<std::string> {
  std::vector<std::string> lines;

  if (!document.date_line.empty()) {
    const auto trimmed_date_line = Trim(document.date_line);
    if (!bills::core::common::iso_period::extract_year_month_from_date_header(
             trimmed_date_line)
             .has_value()) {
      return std::unexpected(RecordEditorError{
          .category = RecordEditorErrorCategory::kSerialize,
          .message = "Invalid date header while serializing.",
      });
    }
    lines.push_back(trimmed_date_line);
  }

  if (document.remark_lines.empty()) {
    lines.push_back("remark:");
  } else {
    for (const auto& remark_line : document.remark_lines) {
      lines.push_back("remark:" + remark_line);
    }
  }

  if (!document.sections.empty()) {
    lines.push_back("");
  }

  for (std::size_t parent_index = 0; parent_index < document.sections.size();
       ++parent_index) {
    const auto& parent = document.sections[parent_index];
    lines.push_back(Trim(parent.title));
    lines.push_back("");

    for (std::size_t sub_index = 0; sub_index < parent.sub_sections.size();
         ++sub_index) {
      const auto& sub_section = parent.sub_sections[sub_index];
      lines.push_back(Trim(sub_section.title));
      for (const auto& entry : sub_section.entries) {
        if (Trim(entry.amount_expression).empty()) {
          return std::unexpected(RecordEditorError{
              .category = RecordEditorErrorCategory::kSerialize,
              .message = "Structured editor entries must include an amount expression.",
          });
        }
        const auto serialized_entry =
            bills::core::ingest::content_line::SerializeStructuredEntryLine(
                entry.amount_expression, entry.description, entry.comment);
        if (!serialized_entry.empty()) {
          lines.push_back(serialized_entry);
        }
      }
      if (sub_index + 1U < parent.sub_sections.size()) {
        lines.push_back("");
      }
    }

    if (parent_index + 1U < document.sections.size()) {
      lines.push_back("");
    }
  }

  std::ostringstream buffer;
  for (std::size_t index = 0; index < lines.size(); ++index) {
    buffer << lines[index];
    if (index + 1U < lines.size()) {
      buffer << '\n';
    }
  }
  return buffer.str();
}
