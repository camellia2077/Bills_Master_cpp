// windows/presentation/cli/controllers/app_controller.cpp
#include "app_controller.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <system_error>
#include <utility>

#include "common/cli_version.hpp"
#include "common/common_utils.hpp"
#include "record_template/record_template_service.hpp"
#include "reports/core/export_format_config.hpp"
#include "reports/core/standard_report_renderer_registry.hpp"
#include "nlohmann/json.hpp"

namespace terminal = common::terminal;
#include "export/export_controller.hpp"
#include "workflow/workflow_controller.hpp"
#if BILLS_CORE_MODULES_ENABLED
import bill.core.common.version;
#else
#include "common/version.hpp"
#endif
#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;
namespace {
constexpr const char* kExportFormatConfigName = "export_formats.toml";
constexpr const char* kNoticesMarkdownPath = "notices/NOTICE.md";
constexpr const char* kNoticesJsonPath = "notices/notices.json";
constexpr const char* kLegacyPipelineDeprecatedSince = "2026-03-05";
constexpr const char* kLegacyPipelineRemovalTarget = "2026-06-30";
constexpr const char* kRuntimeWorkspaceOverrideEnv =
    "BILLS_TRACER_RUNTIME_WORKSPACE_DIR";
constexpr const char* kProjectName = "bills_tracer";

auto GetExecutableDirectory() -> fs::path {
#ifdef _WIN32
  wchar_t path[MAX_PATH] = {0};
  GetModuleFileNameW(nullptr, path, MAX_PATH);
  return fs::path(path).parent_path();
#else
  return fs::current_path();
#endif
}

auto HasRepoMarkers(const fs::path& candidate) -> bool {
  return fs::exists(candidate / "apps") && fs::exists(candidate / "libs") &&
         fs::exists(candidate / "tools") && fs::exists(candidate / "config");
}

auto FindRepoRoot(const fs::path& start_dir) -> fs::path {
  for (fs::path current = start_dir; !current.empty(); current = current.parent_path()) {
    if (HasRepoMarkers(current)) {
      return current;
    }
    if (current == current.root_path()) {
      break;
    }
  }
  return {};
}

auto ResolveRepoRoot() -> fs::path {
  const fs::path exe_dir = GetExecutableDirectory();
  if (const fs::path from_exe = FindRepoRoot(exe_dir); !from_exe.empty()) {
    return from_exe;
  }
  if (const fs::path from_cwd = FindRepoRoot(fs::current_path()); !from_cwd.empty()) {
    return from_cwd;
  }
  return exe_dir;
}

auto ResolveRuntimeWorkspace(const fs::path& repo_root) -> fs::path {
  if (const char* override_value = std::getenv(kRuntimeWorkspaceOverrideEnv);
      override_value != nullptr && override_value[0] != '\0') {
    fs::path override_path(override_value);
    if (override_path.is_relative()) {
      return (repo_root / override_path).lexically_normal();
    }
    return override_path.lexically_normal();
  }
  return (repo_root / "dist" / "runtime" / kProjectName / "workspace")
      .lexically_normal();
}

auto JoinFormats(const std::set<std::string>& formats) -> std::string {
  if (formats.empty()) {
    return "(none)";
  }

  std::ostringstream out;
  bool first = true;
  for (const auto& format : formats) {
    if (!first) {
      out << ", ";
    }
    out << format;
    first = false;
  }
  return out.str();
}

auto AvailableBuildFormats() -> std::set<std::string> {
  const auto formats = StandardReportRendererRegistry::ListAvailableFormats();
  return {formats.begin(), formats.end()};
}

auto ReadTextFile(const fs::path& file_path) -> std::string {
  std::ifstream input(file_path, std::ios::binary);
  if (!input) {
    throw std::runtime_error("Failed to open file: " + file_path.string());
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

auto FormatValidationIssueForLog(const ValidationIssue& issue) -> std::string {
  std::ostringstream out;
  out << issue.code << ": " << issue.message;
  if (!issue.field_path.empty()) {
    out << " [field=" << issue.field_path << "]";
  }
  if (issue.line > 0) {
    out << " [line=" << issue.line;
    if (issue.column > 0) {
      out << ", column=" << issue.column;
    }
    out << "]";
  }
  return out.str();
}

auto JoinPeriods(const std::vector<std::string>& periods) -> std::string {
  if (periods.empty()) {
    return "(none)";
  }

  std::ostringstream out;
  for (std::size_t index = 0; index < periods.size(); ++index) {
    if (index != 0U) {
      out << ", ";
    }
    out << periods[index];
  }
  return out.str();
}

auto ToJson(const ConfigInspectResult& inspect_result) -> nlohmann::json {
  nlohmann::json json;
  json["schema_version"] = inspect_result.schema_version;
  json["date_format"] = inspect_result.date_format;
  json["metadata_headers"] = inspect_result.metadata_headers;

  nlohmann::json categories = nlohmann::json::array();
  for (const auto& category : inspect_result.categories) {
    nlohmann::json item;
    item["parent_item"] = category.parent_item;
    item["description"] = category.description;
    item["sub_items"] = category.sub_items;
    categories.push_back(std::move(item));
  }
  json["categories"] = std::move(categories);
  return json;
}
}  // namespace

AppController::AppController(std::string db_path,
                             const std::string& config_path,
                             std::string modified_output_dir)
    : m_db_path(std::move(db_path)),
      m_config_path(config_path),
      m_modified_output_dir(std::move(modified_output_dir)) {
  const fs::path repo_root = ResolveRepoRoot();
  fs::path exe_dir = GetExecutableDirectory();
  const fs::path runtime_workspace_dir = ResolveRuntimeWorkspace(repo_root);

  if (m_db_path.empty()) {
    m_db_path = (runtime_workspace_dir / "db" / "bills.sqlite3").string();
  }
  if (m_modified_output_dir.empty()) {
    m_modified_output_dir = (runtime_workspace_dir / "cache" / "txt2json").string();
  }
  m_export_base_dir = (runtime_workspace_dir / "exports").string();

  for (const fs::path& dir_path :
       {runtime_workspace_dir / "db", runtime_workspace_dir / "cache" / "txt2json",
        runtime_workspace_dir / "exports"}) {
    std::error_code create_error;
    fs::create_directories(dir_path, create_error);
    if (create_error) {
      std::cerr << terminal::kYellow << "Warning: " << terminal::kReset
                << "Failed to ensure runtime directory exists: "
                << dir_path.string() << ". Reason: " << create_error.message()
                << std::endl;
    }
  }

  fs::path config_path_resolved = config_path;
  if (config_path_resolved.is_relative()) {
    config_path_resolved = exe_dir / config_path_resolved;
  }
  m_config_path = config_path_resolved.string();
  m_enabled_formats = load_enabled_formats(m_config_path);

  m_format_folder_names = {{"json", "JSON_bills"},
                           {"md", "Markdown_bills"},
                           {"rst", "reST_bills"},
                           {"tex", "LaTeX_bills"},
                           {"typ", "Typst_bills"}};
  m_workflow_controller =
      std::make_unique<WorkflowController>(m_config_path, m_modified_output_dir);
  m_export_controller = std::make_unique<ExportController>(
      m_db_path, m_export_base_dir, m_format_folder_names);
}

AppController::~AppController() = default;

auto AppController::load_enabled_formats(const std::string& config_path)
    -> std::set<std::string> {
  std::set<std::string> enabled_formats;
  const fs::path config_file = fs::path(config_path) / kExportFormatConfigName;
  const auto validation = ExportFormatConfig::ValidateFile(config_file);
  std::set<std::string> available_formats(validation.available_formats.begin(),
                                          validation.available_formats.end());
  if (available_formats.empty()) {
    available_formats = AvailableBuildFormats();
  }

  if (!validation.ok) {
    const std::string detail =
        validation.issues.empty()
            ? "Unknown export format validation error."
            : FormatValidationIssueForLog(validation.issues.front());
    std::cerr << terminal::kYellow << "Warning: " << terminal::kReset
              << "Failed to load export formats from " << config_file.string()
              << ". Reason: " << detail
              << ". Falling back to build format set: "
              << JoinFormats(available_formats) << "." << std::endl;
    return available_formats;
  }

  enabled_formats.insert(validation.enabled_formats.begin(),
                         validation.enabled_formats.end());
  if (enabled_formats.empty()) {
    enabled_formats = available_formats;
    std::cerr << terminal::kYellow << "Warning: " << terminal::kReset
              << "No formats configured in " << config_file.string()
              << ". Falling back to build format set: "
              << JoinFormats(enabled_formats) << "." << std::endl;
  }
  return enabled_formats;
}

auto AppController::normalize_format(std::string format_name) const
    -> std::string {
  return StandardReportRendererRegistry::NormalizeFormat(format_name);
}

auto AppController::normalize_export_pipeline(std::string pipeline_name) const
    -> std::string {
  std::transform(pipeline_name.begin(), pipeline_name.end(),
                 pipeline_name.begin(), [](unsigned char ch) -> char {
                   return static_cast<char>(std::tolower(ch));
                 });
  std::ranges::replace(pipeline_name, '_', '-');
  if (pipeline_name.empty()) {
    return "model-first";
  }
  if (pipeline_name == "jsonfirst") {
    return "json-first";
  }
  if (pipeline_name == "modelfirst") {
    return "model-first";
  }
  return pipeline_name;
}

auto AppController::is_export_format_available(
    const std::string& type, const std::vector<std::string>& values,
    const std::string& format_str,
    const std::string& export_pipeline) const -> bool {
  (void)type;
  (void)values;
  (void)export_pipeline;
  const std::string format = normalize_format(format_str);
  if (m_enabled_formats.count(format) == 0U) {
    std::cerr << terminal::kRed << "Error: " << terminal::kReset
              << "Format '" << format
              << "' is not enabled. Enabled formats: "
              << JoinFormats(m_enabled_formats)
              << ". Please update config/" << kExportFormatConfigName << "."
              << std::endl;
    return false;
  }
  if (!StandardReportRendererRegistry::IsFormatAvailable(format)) {
    std::cerr << terminal::kRed << "Error: " << terminal::kReset
              << "Format '" << format
              << "' is enabled in config but not available in this build."
              << std::endl;
    return false;
  }

  return true;
}

auto AppController::list_enabled_export_formats() const
    -> std::vector<std::string> {
  std::vector<std::string> formats;
  const auto available_formats = StandardReportRendererRegistry::ListAvailableFormats();
  formats.reserve(m_enabled_formats.size());
  for (const auto& format : available_formats) {
    if (m_enabled_formats.count(format) != 0U) {
      formats.push_back(format);
    }
  }
  for (const auto& format : m_enabled_formats) {
    if (std::ranges::find(formats, format) == formats.end()) {
      formats.push_back(format);
    }
  }
  return formats;
}

// ... handle_validation, handle_convert 等其他方法保持不变 ...
auto AppController::handle_validation(const std::string& path) -> bool {
  return m_workflow_controller->handle_validation(path);
}

auto AppController::handle_convert(const std::string& path) -> bool {
  return m_workflow_controller->handle_convert(path);
}

auto AppController::handle_ingest(const std::string& path, bool write_json)
    -> bool {
  return m_workflow_controller->handle_ingest(path, m_db_path, write_json);
}

auto AppController::handle_import(const std::string& path) -> bool {
  return m_workflow_controller->handle_import(path, m_db_path);
}

auto AppController::handle_full_workflow(const std::string& path) -> bool {
  return m_workflow_controller->handle_full_workflow(path, m_db_path);
}

auto AppController::handle_record_template(const RecordTemplateOptions& options)
    -> bool {
  TemplateGenerationRequest request;
  request.period = options.period;
  request.start_period = options.start_period;
  request.end_period = options.end_period;
  request.start_year = options.start_year;
  request.end_year = options.end_year;
  request.config_dir = m_config_path;
  request.write_files = !options.output_dir.empty();
  request.output_dir = options.output_dir;

  const auto result = RecordTemplateService::GenerateTemplates(request);
  if (!result) {
    std::cerr << terminal::kRed << "Error: " << terminal::kReset
              << FormatRecordTemplateError(result.error()) << std::endl;
    return false;
  }

  if (result->templates.empty()) {
    std::cerr << terminal::kYellow << "Warning: " << terminal::kReset
              << "No templates were generated." << std::endl;
    return false;
  }

  if (result->write_files) {
    std::cout << "Generated " << result->templates.size()
              << " template file(s):" << std::endl;
    for (const auto& generated_template : result->templates) {
      std::cout << "  " << generated_template.output_path.string() << std::endl;
    }
    return true;
  }

  for (std::size_t index = 0; index < result->templates.size(); ++index) {
    const auto& generated_template = result->templates[index];
    std::cout << "=== " << generated_template.period << " ("
              << generated_template.relative_path.generic_string() << ") ==="
              << std::endl;
    std::cout << generated_template.text << std::endl;
    if (index + 1U < result->templates.size()) {
      std::cout << std::endl;
    }
  }
  return true;
}

auto AppController::handle_record_preview(const std::string& path) -> bool {
  const auto result = RecordTemplateService::PreviewRecords(path, m_config_path);
  if (!result) {
    std::cerr << terminal::kRed << "Error: " << terminal::kReset
              << FormatRecordTemplateError(result.error()) << std::endl;
    return false;
  }

  if (result->processed == 0U) {
    std::cerr << terminal::kRed << "Error: " << terminal::kReset
              << "No .txt files found under input_path." << std::endl;
    return false;
  }

  for (const auto& preview_file : result->files) {
    if (preview_file.ok) {
      std::cout << "[OK] " << preview_file.path.string() << " | period="
                << preview_file.period << " | txns="
                << preview_file.transaction_count << " | income="
                << preview_file.total_income << " | expense="
                << preview_file.total_expense << " | balance="
                << preview_file.balance << std::endl;
      continue;
    }

    std::cerr << "[FAIL] " << preview_file.path.string() << " | "
              << preview_file.error << std::endl;
  }

  std::cout << "Processed: " << result->processed
            << ", Success: " << result->success
            << ", Failure: " << result->failure << std::endl;
  std::cout << "Periods: " << JoinPeriods(result->periods) << std::endl;
  return result->failure == 0U;
}

auto AppController::handle_record_list(const std::string& path) -> bool {
  const auto result = RecordTemplateService::ListPeriods(path);
  if (!result) {
    std::cerr << terminal::kRed << "Error: " << terminal::kReset
              << FormatRecordTemplateError(result.error()) << std::endl;
    return false;
  }

  if (result->processed == 0U) {
    std::cerr << terminal::kRed << "Error: " << terminal::kReset
              << "No .txt files found under input_path." << std::endl;
    return false;
  }

  for (const auto& period : result->periods) {
    std::cout << period << std::endl;
  }

  if (result->invalid != 0U) {
    for (const auto& invalid_file : result->invalid_files) {
      std::cerr << "[INVALID] " << invalid_file.path.string() << " | "
                << invalid_file.error << std::endl;
    }
  }

  return result->invalid == 0U;
}

auto AppController::handle_config_inspect() -> bool {
  const auto result = RecordTemplateService::InspectConfig(m_config_path);
  if (!result) {
    std::cerr << terminal::kRed << "Error: " << terminal::kReset
              << FormatRecordTemplateError(result.error()) << std::endl;
    return false;
  }

  std::cout << ToJson(*result).dump(2) << std::endl;
  return true;
}

auto AppController::handle_export(const std::string& type,
                                  const std::vector<std::string>& values,
                                  const std::string& format_str,
                                  const std::string& export_pipeline) -> bool {
  const std::string normalized_format = normalize_format(format_str);
  const std::string normalized_pipeline =
      normalize_export_pipeline(export_pipeline);
  if (normalized_pipeline != "legacy" &&
      normalized_pipeline != "model-first" &&
      normalized_pipeline != "json-first") {
    std::cerr << terminal::kRed << "Error: " << terminal::kReset
              << "Unknown value for --export-pipeline: '" << export_pipeline
              << "'. Use 'legacy', 'model-first', or 'json-first'."
              << std::endl;
    return false;
  }

  if (!is_export_format_available(type, values, normalized_format,
                                  normalized_pipeline)) {
    return false;
  }

  if (normalized_pipeline == "legacy") {
    static bool legacy_warning_emitted = false;
    if (!legacy_warning_emitted) {
      std::cerr
          << terminal::kYellow << "Warning: " << terminal::kReset
          << "Export pipeline 'legacy' is transitional and deprecated since "
          << kLegacyPipelineDeprecatedSince << ". Planned removal target is "
          << kLegacyPipelineRemovalTarget
          << ". Prefer 'model-first' (default) or 'json-first'." << std::endl;
      legacy_warning_emitted = true;
    }
  }

  return m_export_controller->handle_export(type, values, normalized_format,
                                            normalized_pipeline);
}

void AppController::display_version() {
#if BILLS_CORE_MODULES_ENABLED
  constexpr auto kCoreVersion = bills::core::modules::common_version::kVersion;
  constexpr auto kCoreLastUpdated =
      bills::core::modules::common_version::kLastUpdated;
#else
  constexpr auto kCoreVersion = bills::core::version::kVersion;
  constexpr auto kCoreLastUpdated = bills::core::version::kLastUpdated;
#endif
  std::cout << "Core Version: " << kCoreVersion << std::endl;
  std::cout << "Core Last Updated: " << kCoreLastUpdated << std::endl;
  std::cout << "CLI Version: " << bills::cli::version::kVersion << std::endl;
  std::cout << "CLI Last Updated: " << bills::cli::version::kLastUpdated
            << std::endl;
}

auto AppController::display_notices(bool raw_json) -> bool {
  const fs::path notices_path =
      ResolveRuntimeWorkspace(ResolveRepoRoot()) /
      (raw_json ? kNoticesJsonPath : kNoticesMarkdownPath);
  try {
    std::cout << ReadTextFile(notices_path);
    return true;
  } catch (const std::exception& error) {
    std::cerr << terminal::kRed << "Error: " << terminal::kReset
              << "Failed to load bundled notices from " << notices_path.string()
              << ". Reason: " << error.what() << std::endl;
    return false;
  }
}

