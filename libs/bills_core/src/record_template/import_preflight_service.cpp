#include "record_template/import_preflight_service.hpp"

#include <map>
#include <set>

#include "common/iso_period.hpp"
#include "record_template/record_template_service.hpp"

namespace {
auto ParsePeriodFromFileName(const std::string& display_path) -> std::string {
  const std::size_t slash = display_path.find_last_of("/\\");
  const std::size_t stem_begin = slash == std::string::npos ? 0 : slash + 1;
  const std::size_t dot = display_path.find_last_of('.');
  const std::size_t stem_end =
      (dot == std::string::npos || dot < stem_begin) ? display_path.size() : dot;
  const std::string stem = display_path.substr(stem_begin, stem_end - stem_begin);
  const auto parsed =
      bills::core::common::iso_period::parse_year_month(stem);
  if (!parsed.has_value()) {
    return {};
  }
  return bills::core::common::iso_period::format_year_month(parsed->year,
                                                            parsed->month);
}

auto AttachIssue(RecordPreviewFile& file, const ValidationIssue& issue,
                 ImportPreflightResult& result) -> void {
  file.issues.push_back(issue);
  result.issues.push_back(issue);
}

auto CollectSorted(const std::set<std::string>& values) -> std::vector<std::string> {
  return {values.begin(), values.end()};
}

}  // namespace

auto ImportPreflightService::Run(const ImportPreflightRequest& request)
    -> RecordTemplateResult<ImportPreflightResult> {
  ImportPreflightResult result;
  result.config_validation = request.config_validation;
  if (!result.config_validation.ok) {
    return result;
  }

  const auto preview_result = RecordTemplateService::ValidateRecordBatch(
      request.documents, request.config_bundle, request.input_label);
  if (!preview_result) {
    return std::unexpected(preview_result.error());
  }

  result.record_validation = *preview_result;
  std::set<std::string> periods;
  std::map<std::string, std::vector<std::size_t>> file_indexes_by_period;
  std::set<std::string> workspace_periods(request.existing_workspace_periods.begin(),
                                          request.existing_workspace_periods.end());
  std::set<std::string> db_periods(request.existing_db_periods.begin(),
                                   request.existing_db_periods.end());
  std::set<std::string> duplicate_periods;
  std::set<std::string> workspace_conflicts;
  std::set<std::string> db_conflicts;

  for (std::size_t index = 0; index < result.record_validation.files.size();
       ++index) {
    auto& file = result.record_validation.files[index];
    if (!file.period.empty()) {
      periods.insert(file.period);
      file_indexes_by_period[file.period].push_back(index);
    }

    file.file_name_period = ParsePeriodFromFileName(file.path);
    file.file_name_matches_period =
        file.file_name_period.empty() || file.file_name_period == file.period;
    if (file.ok && !file.file_name_period.empty() &&
        file.file_name_period != file.period) {
      AttachIssue(
          file,
          MakeValidationIssue(
              "record_txt", "cross_file", "import.filename_period_mismatch",
              "File name period '" + file.file_name_period +
                  "' does not match content period '" + file.period + "'.",
              file.path),
          result);
    }
  }

  for (const auto& [period, indexes] : file_indexes_by_period) {
    if (indexes.size() <= 1U) {
      continue;
    }
    duplicate_periods.insert(period);
    for (const std::size_t index : indexes) {
      AttachIssue(
          result.record_validation.files[index],
          MakeValidationIssue(
              "record_txt", "cross_file", "import.duplicate_period_in_batch",
              "Period '" + period + "' appears multiple times in the import batch.",
              result.record_validation.files[index].path),
          result);
    }
  }

  for (auto& file : result.record_validation.files) {
    if (!file.period.empty() && workspace_periods.contains(file.period)) {
      workspace_conflicts.insert(file.period);
      AttachIssue(
          file,
          MakeValidationIssue(
              "record_txt", "cross_file", "import.workspace_period_conflict",
              "Period '" + file.period + "' already exists in the workspace.",
              file.path),
          result);
    }
    if (!file.period.empty() && db_periods.contains(file.period)) {
      db_conflicts.insert(file.period);
      AttachIssue(
          file,
          MakeValidationIssue("record_txt", "cross_file",
                              "import.db_period_conflict",
                              "Period '" + file.period +
                                  "' already exists in the database.",
                              file.path),
          result);
    }
    file.ok = file.ok && file.issues.empty();
  }

  result.periods = CollectSorted(periods);
  result.duplicate_periods = CollectSorted(duplicate_periods);
  result.workspace_conflict_periods = CollectSorted(workspace_conflicts);
  result.db_conflict_periods = CollectSorted(db_conflicts);

  result.success = 0;
  result.failure = 0;
  for (const auto& file : result.record_validation.files) {
    if (file.ok) {
      ++result.success;
    } else {
      ++result.failure;
    }
  }
  result.record_validation.success = result.success;
  result.record_validation.failure = result.failure;
  result.all_clear = result.config_validation.ok && result.failure == 0U;
  return result;
}
