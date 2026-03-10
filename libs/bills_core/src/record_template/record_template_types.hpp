#ifndef RECORD_TEMPLATE_RECORD_TEMPLATE_TYPES_HPP_
#define RECORD_TEMPLATE_RECORD_TEMPLATE_TYPES_HPP_

#include <cstddef>
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

#include "common/Result.hpp"

enum class RecordTemplateErrorCategory {
  kRequest,
  kConfig,
  kInputPath,
  kOutputPath,
  kSystem,
};

struct RecordTemplateError {
  RecordTemplateErrorCategory category = RecordTemplateErrorCategory::kSystem;
  std::string message;
  std::string detail;
};

template <typename T>
using RecordTemplateResult = std::expected<T, RecordTemplateError>;

inline auto MakeRecordTemplateError(RecordTemplateErrorCategory category,
                                    std::string message,
                                    std::string detail = {})
    -> RecordTemplateError {
  return RecordTemplateError{
      .category = category,
      .message = std::move(message),
      .detail = std::move(detail),
  };
}

inline auto MakeRecordTemplateErrorFromCommon(
    const Error& error, RecordTemplateErrorCategory category)
    -> RecordTemplateError {
  return MakeRecordTemplateError(category, error.message_, error.context_);
}

inline auto FormatRecordTemplateError(const RecordTemplateError& error)
    -> std::string {
  if (error.detail.empty()) {
    return error.message;
  }
  return error.message + " (" + error.detail + ")";
}

struct OrderedTemplateCategory {
  std::string parent_item;
  std::string description;
  std::vector<std::string> sub_items;
};

struct OrderedTemplateLayout {
  std::vector<OrderedTemplateCategory> categories;
};

struct TemplateGenerationRequest {
  std::string period;
  std::string start_period;
  std::string end_period;
  std::string start_year;
  std::string end_year;
  std::filesystem::path config_dir;
  std::filesystem::path validator_config_path;
  bool write_files = false;
  std::filesystem::path output_dir;
};

struct GeneratedTemplateFile {
  std::string period;
  std::filesystem::path relative_path;
  std::string text;
  std::filesystem::path output_path;
};

struct TemplateGenerationResult {
  bool write_files = false;
  std::filesystem::path output_dir;
  std::vector<GeneratedTemplateFile> templates;
};

struct RecordPreviewFile {
  std::filesystem::path path;
  bool ok = false;
  std::string period;
  int year = 0;
  int month = 0;
  std::size_t transaction_count = 0;
  double total_income = 0.0;
  double total_expense = 0.0;
  double balance = 0.0;
  std::string error;
};

struct RecordPreviewResult {
  std::filesystem::path input_path;
  std::size_t processed = 0;
  std::size_t success = 0;
  std::size_t failure = 0;
  std::vector<std::string> periods;
  std::vector<RecordPreviewFile> files;
};

struct ConfigInspectResult {
  int schema_version = 1;
  std::string date_format = "YYYY-MM";
  std::vector<std::string> metadata_headers = {"date", "remark"};
  std::vector<OrderedTemplateCategory> categories;
};

struct InvalidPeriodFile {
  std::filesystem::path path;
  std::string error;
};

struct ListedPeriodsResult {
  std::filesystem::path input_path;
  std::size_t processed = 0;
  std::size_t valid = 0;
  std::size_t invalid = 0;
  std::vector<std::string> periods;
  std::vector<InvalidPeriodFile> invalid_files;
};

#endif  // RECORD_TEMPLATE_RECORD_TEMPLATE_TYPES_HPP_
