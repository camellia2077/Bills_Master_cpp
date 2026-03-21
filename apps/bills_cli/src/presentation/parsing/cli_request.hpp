#ifndef PRESENTATION_PARSING_CLI_REQUEST_HPP_
#define PRESENTATION_PARSING_CLI_REQUEST_HPP_

#include <filesystem>
#include <optional>
#include <string>
#include <variant>

namespace bills::cli {

enum class WorkspaceAction {
  kValidate,
  kConvert,
  kIngest,
  kImportJson,
};

struct WorkspaceRequest {
  WorkspaceAction action = WorkspaceAction::kValidate;
  std::filesystem::path input_path;
  std::optional<std::filesystem::path> db_path;
  bool write_json_cache = false;
};

enum class ReportAction {
  kShowYear,
  kShowMonth,
  kExportYear,
  kExportMonth,
  kExportRange,
  kExportAllMonths,
  kExportAllYears,
  kExportAll,
};

struct ReportRequest {
  ReportAction action = ReportAction::kShowYear;
  std::string primary_value;
  std::string secondary_value;
  std::string format = "md";
};

enum class TemplateAction {
  kGenerate,
  kPreview,
  kListPeriods,
};

struct TemplateRequest {
  TemplateAction action = TemplateAction::kGenerate;
  std::filesystem::path input_path;
  std::string period;
  std::string start_period;
  std::string end_period;
  std::string start_year;
  std::string end_year;
  std::optional<std::filesystem::path> output_dir;
};

enum class ConfigAction {
  kInspect,
  kFormats,
};

struct ConfigRequest {
  ConfigAction action = ConfigAction::kInspect;
};

enum class MetaAction {
  kVersion,
  kNotices,
};

struct MetaRequest {
  MetaAction action = MetaAction::kVersion;
  bool raw_json = false;
};

struct HelpRequest {};

using CliRequest = std::variant<HelpRequest, WorkspaceRequest, ReportRequest,
                                TemplateRequest, ConfigRequest, MetaRequest>;

}  // namespace bills::cli

#endif  // PRESENTATION_PARSING_CLI_REQUEST_HPP_
