#include "io/host_flow_support.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

#include "io/adapters/reports/report_export_service.hpp"
#include "common/iso_period.hpp"
#include "io/adapters/config/config_document_parser.hpp"
#include "io/adapters/io/source_document_io.hpp"
#include "io/adapters/io/year_partition_output_path_builder.hpp"
#include "io/adapters/io/zip_archive_io.hpp"
#include "io/io_factory.hpp"
#include "nlohmann/json.hpp"
#include "query/query_service.hpp"
#include "record_template/record_template_service.hpp"
#include "reporting/renderers/standard_report_renderer_registry.hpp"
#include "reporting/report_render_service.hpp"

namespace bills::io {
namespace {

constexpr const char* kContext = "HostFlowSupport";
constexpr std::string_view kManifestPath = "manifest.json";
constexpr std::string_view kConfigPrefix = "config/";
constexpr std::string_view kRecordsPrefix = "records/";
constexpr int kParseBundleVersion = 1;
constexpr int kBackupBundleVersion = 1;
constexpr std::string_view kParseBundleKind = "parse_bundle";
constexpr std::string_view kBackupBundleKind = "backup_bundle";
constexpr std::array<std::string_view, 3U> kConfigFileNames = {
    "validator_config.toml",
    "modifier_config.toml",
    "export_formats.toml",
};
constexpr std::array<std::string_view, 2U> kBackupConfigFileNames = {
    "validator_config.toml",
    "modifier_config.toml",
};

struct ConfigTexts {
  std::string validator_text;
  std::string modifier_text;
  std::string export_formats_text;
};

struct ValidatedConfigTextContext {
  ConfigTexts texts;
  ConfigDocumentBundle documents;
  ValidatedConfigBundle validated;
};

struct BundleArchiveContents {
  std::string manifest_text;
  ConfigTexts config_texts;
  SourceDocumentBatch records;
};

struct BackupBundleArchiveContents {
  std::string manifest_text;
  std::string validator_text;
  std::string modifier_text;
  SourceDocumentBatch records;
};

struct FileSnapshot {
  std::filesystem::path relative_path;
  std::optional<std::string> previous_text;
};

struct SourceDocumentView {
  std::string relative_path;
  std::string text;
};

struct RecordImportCandidate {
  std::string absolute_path;
  std::string relative_path;
  std::string period;
  std::string text;
};

auto MakeConfigValidationError(const ConfigBundleValidationReport& report) -> Error {
  for (const auto& file : report.files) {
    if (file.issues.empty()) {
      continue;
    }
    const auto& issue = file.issues.front();
    std::string message = issue.message;
    if (!issue.path.empty()) {
      message += " [" + issue.path + "]";
    }
    if (!issue.field_path.empty()) {
      message += " field=" + issue.field_path;
    }
    return MakeError(std::move(message), kContext);
  }
  return MakeError("Config validation failed.", kContext);
}

auto ReadConfigTextsFromDirectory(const std::filesystem::path& config_dir)
    -> Result<ConfigTexts> {
  const auto validator_text =
      SourceDocumentIo::ReadText(config_dir / kConfigFileNames[0]);
  if (!validator_text) {
    return std::unexpected(validator_text.error());
  }
  const auto modifier_text =
      SourceDocumentIo::ReadText(config_dir / kConfigFileNames[1]);
  if (!modifier_text) {
    return std::unexpected(modifier_text.error());
  }
  const auto export_formats_text =
      SourceDocumentIo::ReadText(config_dir / kConfigFileNames[2]);
  if (!export_formats_text) {
    return std::unexpected(export_formats_text.error());
  }

  return ConfigTexts{
      .validator_text = *validator_text,
      .modifier_text = *modifier_text,
      .export_formats_text = *export_formats_text,
  };
}

auto ParseAndValidateConfigTexts(
    ConfigTexts texts, const std::string& validator_display_path,
    const std::string& modifier_display_path,
    const std::string& export_formats_display_path)
    -> Result<ValidatedConfigTextContext> {
  const ConfigDocumentBundle documents = ConfigDocumentParser::ParseTexts(
      texts.validator_text, texts.modifier_text, texts.export_formats_text,
      validator_display_path, modifier_display_path, export_formats_display_path);
  const auto validated = ConfigBundleService::Validate(documents);
  if (!validated) {
    return std::unexpected(MakeConfigValidationError(validated.error()));
  }

  return ValidatedConfigTextContext{
      .texts = std::move(texts),
      .documents = documents,
      .validated = *validated,
  };
}

auto LoadValidatedConfigTextContext(const std::filesystem::path& config_dir)
    -> Result<ValidatedConfigTextContext> {
  auto texts = ReadConfigTextsFromDirectory(config_dir);
  if (!texts) {
    return std::unexpected(texts.error());
  }
  return ParseAndValidateConfigTexts(
      *texts, (config_dir / kConfigFileNames[0]).string(),
      (config_dir / kConfigFileNames[1]).string(),
      (config_dir / kConfigFileNames[2]).string());
}

auto BuildValidationError(const BillWorkflowBatchResult& result,
                          std::string_view prefix) -> Error {
  for (const auto& file : result.files) {
    if (file.ok) {
      continue;
    }
    std::string detail = std::string(prefix) + ": " + file.display_path;
    if (!file.stage.empty()) {
      detail += " | stage=" + file.stage;
    }
    if (!file.error.empty()) {
      detail += " | " + file.error;
    }
    return MakeError(std::move(detail), kContext);
  }
  return MakeError(std::string(prefix), kContext);
}

auto ValidateRecordDocuments(const SourceDocumentBatch& documents,
                             const RuntimeConfigBundle& runtime_config,
                             std::string_view failure_prefix)
    -> Result<void> {
  const auto result = BillWorkflowService::Validate(documents, runtime_config);
  if (result.failure > 0U) {
    return std::unexpected(BuildValidationError(result, failure_prefix));
  }
  return {};
}

auto ToTm(std::time_t now) -> std::tm {
  std::tm tm_value{};
#ifdef _WIN32
  localtime_s(&tm_value, &now);
#else
  localtime_r(&now, &tm_value);
#endif
  return tm_value;
}

auto FormatLocalTimestamp(std::string_view format) -> std::string {
  const auto now =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  const std::tm tm_value = ToTm(now);
  std::ostringstream stream;
  stream << std::put_time(&tm_value, format.data());
  return stream.str();
}

auto BuildManifestText(std::size_t record_count) -> std::string {
  nlohmann::json manifest = {
      {"bundle_version", kParseBundleVersion},
      {"exported_at", FormatLocalTimestamp("%Y-%m-%dT%H:%M:%S")},
      {"record_count", record_count},
      {"config_files",
       {std::string(kConfigFileNames[0]), std::string(kConfigFileNames[1]),
        std::string(kConfigFileNames[2])}},
  };
  return manifest.dump(2) + "\n";
}

auto ValidateManifest(const std::string& manifest_text,
                      std::size_t extracted_record_count) -> Result<void> {
  try {
    const nlohmann::json manifest = nlohmann::json::parse(manifest_text);
    if (!manifest.is_object()) {
      return std::unexpected(
          MakeError("manifest.json must contain a JSON object.", kContext));
    }
    if (manifest.contains("bundle_kind") &&
        (!manifest["bundle_kind"].is_string() ||
         manifest["bundle_kind"].get<std::string>() != kParseBundleKind)) {
      return std::unexpected(MakeError(
          "manifest.json bundle_kind must be parse_bundle when present.",
          kContext));
    }
    if (!manifest.contains("bundle_version") ||
        !manifest["bundle_version"].is_number_integer() ||
        manifest["bundle_version"].get<int>() != kParseBundleVersion) {
      return std::unexpected(MakeError(
          "manifest.json must contain bundle_version=1.", kContext));
    }
    if (!manifest.contains("record_count") ||
        !manifest["record_count"].is_number_unsigned()) {
      return std::unexpected(MakeError(
          "manifest.json must contain an unsigned record_count.", kContext));
    }
    if (manifest["record_count"].get<std::size_t>() != extracted_record_count) {
      return std::unexpected(MakeError(
          "manifest.json record_count does not match archive contents.",
          kContext));
    }
    if (!manifest.contains("config_files") || !manifest["config_files"].is_array()) {
      return std::unexpected(MakeError(
          "manifest.json must contain a config_files array.", kContext));
    }
    std::set<std::string> config_files;
    for (const auto& item : manifest["config_files"]) {
      if (!item.is_string()) {
        return std::unexpected(MakeError(
            "manifest.json config_files must contain strings.", kContext));
      }
      config_files.insert(item.get<std::string>());
    }
    const std::set<std::string> expected = {
        std::string(kConfigFileNames[0]), std::string(kConfigFileNames[1]),
        std::string(kConfigFileNames[2]),
    };
    if (config_files != expected) {
      return std::unexpected(MakeError(
          "manifest.json config_files must list the canonical TOML files.",
          kContext));
    }
  } catch (const nlohmann::json::exception& error) {
    return std::unexpected(MakeError(
        "Failed to parse manifest.json: " + std::string(error.what()),
        kContext));
  }
  return {};
}

auto BuildBackupManifestText(std::size_t record_count) -> std::string {
  nlohmann::json manifest = {
      {"bundle_kind", kBackupBundleKind},
      {"bundle_version", kBackupBundleVersion},
      {"exported_at", FormatLocalTimestamp("%Y-%m-%dT%H:%M:%S")},
      {"record_count", record_count},
      {"config_files",
       {std::string(kBackupConfigFileNames[0]),
        std::string(kBackupConfigFileNames[1])}},
  };
  return manifest.dump(2) + "\n";
}

auto ValidateBackupManifest(const std::string& manifest_text,
                            std::size_t extracted_record_count) -> Result<void> {
  try {
    const nlohmann::json manifest = nlohmann::json::parse(manifest_text);
    if (!manifest.is_object()) {
      return std::unexpected(
          MakeError("manifest.json must contain a JSON object.", kContext));
    }
    if (!manifest.contains("bundle_kind") || !manifest["bundle_kind"].is_string() ||
        manifest["bundle_kind"].get<std::string>() != kBackupBundleKind) {
      return std::unexpected(MakeError(
          "manifest.json must contain bundle_kind=backup_bundle.", kContext));
    }
    if (!manifest.contains("bundle_version") ||
        !manifest["bundle_version"].is_number_integer() ||
        manifest["bundle_version"].get<int>() != kBackupBundleVersion) {
      return std::unexpected(MakeError(
          "manifest.json must contain bundle_version=1.", kContext));
    }
    if (!manifest.contains("record_count") ||
        !manifest["record_count"].is_number_unsigned()) {
      return std::unexpected(MakeError(
          "manifest.json must contain an unsigned record_count.", kContext));
    }
    if (manifest["record_count"].get<std::size_t>() != extracted_record_count) {
      return std::unexpected(MakeError(
          "manifest.json record_count does not match archive contents.",
          kContext));
    }
    if (!manifest.contains("config_files") || !manifest["config_files"].is_array()) {
      return std::unexpected(MakeError(
          "manifest.json must contain a config_files array.", kContext));
    }
    std::set<std::string> config_files;
    for (const auto& item : manifest["config_files"]) {
      if (!item.is_string()) {
        return std::unexpected(MakeError(
            "manifest.json config_files must contain strings.", kContext));
      }
      config_files.insert(item.get<std::string>());
    }
    const std::set<std::string> expected = {
        std::string(kBackupConfigFileNames[0]),
        std::string(kBackupConfigFileNames[1]),
    };
    if (config_files != expected) {
      return std::unexpected(MakeError(
          "manifest.json config_files must list validator_config.toml and "
          "modifier_config.toml.",
          kContext));
    }
  } catch (const nlohmann::json::exception& error) {
    return std::unexpected(MakeError(
        "Failed to parse manifest.json: " + std::string(error.what()),
        kContext));
  }
  return {};
}

auto HasTxtExtension(const std::filesystem::path& path) -> bool {
  std::string extension = path.extension().string();
  std::transform(extension.begin(), extension.end(), extension.begin(),
                 [](unsigned char character) -> char {
                   return static_cast<char>(std::tolower(character));
                 });
  return extension == ".txt";
}

auto LoadBundleArchiveContents(const std::filesystem::path& bundle_zip)
    -> Result<BundleArchiveContents> {
  const auto entries = ZipArchiveIo::ReadTextEntries(bundle_zip);
  if (!entries) {
    return std::unexpected(entries.error());
  }

  BundleArchiveContents contents;
  bool has_manifest = false;
  bool has_validator = false;
  bool has_modifier = false;
  bool has_export_formats = false;

  for (const auto& entry : *entries) {
    if (entry.archive_path == kManifestPath) {
      has_manifest = true;
      contents.manifest_text = entry.text;
      continue;
    }

    if (entry.archive_path.starts_with(kConfigPrefix)) {
      const std::string relative_path =
          entry.archive_path.substr(kConfigPrefix.size());
      if (relative_path == kConfigFileNames[0]) {
        has_validator = true;
        contents.config_texts.validator_text = entry.text;
        continue;
      }
      if (relative_path == kConfigFileNames[1]) {
        has_modifier = true;
        contents.config_texts.modifier_text = entry.text;
        continue;
      }
      if (relative_path == kConfigFileNames[2]) {
        has_export_formats = true;
        contents.config_texts.export_formats_text = entry.text;
        continue;
      }
      return std::unexpected(MakeError(
          "ZIP archive contains unsupported config entry: " + entry.archive_path,
          kContext));
    }

    if (entry.archive_path.starts_with(kRecordsPrefix)) {
      const std::string relative_path =
          entry.archive_path.substr(kRecordsPrefix.size());
      if (relative_path.empty()) {
        return std::unexpected(MakeError(
            "ZIP archive record path must not be empty.", kContext));
      }
      const std::filesystem::path record_path(relative_path);
      if (!HasTxtExtension(record_path)) {
        return std::unexpected(MakeError(
            "ZIP archive record entries must use the .txt extension: " +
                entry.archive_path,
            kContext));
      }
      contents.records.push_back(SourceDocument{
          .display_path = std::filesystem::path(relative_path).generic_string(),
          .text = entry.text,
      });
      continue;
    }

    return std::unexpected(MakeError(
        "ZIP archive contains unsupported top-level entry: " + entry.archive_path,
        kContext));
  }

  if (!has_manifest) {
    return std::unexpected(
        MakeError("ZIP archive is missing manifest.json.", kContext));
  }
  if (!has_validator || !has_modifier || !has_export_formats) {
    return std::unexpected(MakeError(
        "ZIP archive must contain validator_config.toml, modifier_config.toml, "
        "and export_formats.toml under config/.",
        kContext));
  }

  const auto manifest_validation =
      ValidateManifest(contents.manifest_text, contents.records.size());
  if (!manifest_validation) {
    return std::unexpected(manifest_validation.error());
  }
  return contents;
}

auto LoadBackupArchiveContents(const std::filesystem::path& bundle_zip)
    -> Result<BackupBundleArchiveContents> {
  const auto entries = ZipArchiveIo::ReadTextEntries(bundle_zip);
  if (!entries) {
    return std::unexpected(entries.error());
  }

  BackupBundleArchiveContents contents;
  bool has_manifest = false;
  bool has_validator = false;
  bool has_modifier = false;

  for (const auto& entry : *entries) {
    if (entry.archive_path == kManifestPath) {
      has_manifest = true;
      contents.manifest_text = entry.text;
      continue;
    }

    if (entry.archive_path.starts_with(kConfigPrefix)) {
      const std::string relative_path =
          entry.archive_path.substr(kConfigPrefix.size());
      if (relative_path == kBackupConfigFileNames[0]) {
        has_validator = true;
        contents.validator_text = entry.text;
        continue;
      }
      if (relative_path == kBackupConfigFileNames[1]) {
        has_modifier = true;
        contents.modifier_text = entry.text;
        continue;
      }
      return std::unexpected(MakeError(
          "Backup ZIP archive contains unsupported config entry: " +
              entry.archive_path,
          kContext));
    }

    if (entry.archive_path.starts_with(kRecordsPrefix)) {
      const std::string relative_path =
          entry.archive_path.substr(kRecordsPrefix.size());
      if (relative_path.empty()) {
        return std::unexpected(MakeError(
            "Backup ZIP archive record path must not be empty.", kContext));
      }
      const std::filesystem::path record_path(relative_path);
      if (!HasTxtExtension(record_path)) {
        return std::unexpected(MakeError(
            "Backup ZIP archive record entries must use the .txt extension: " +
                entry.archive_path,
            kContext));
      }
      contents.records.push_back(SourceDocument{
          .display_path = std::filesystem::path(relative_path).generic_string(),
          .text = entry.text,
      });
      continue;
    }

    return std::unexpected(MakeError(
        "Backup ZIP archive contains unsupported top-level entry: " +
            entry.archive_path,
        kContext));
  }

  if (!has_manifest) {
    return std::unexpected(
        MakeError("Backup ZIP archive is missing manifest.json.", kContext));
  }
  if (!has_validator || !has_modifier) {
    return std::unexpected(MakeError(
        "Backup ZIP archive must contain validator_config.toml and "
        "modifier_config.toml under config/.",
        kContext));
  }

  const auto manifest_validation =
      ValidateBackupManifest(contents.manifest_text, contents.records.size());
  if (!manifest_validation) {
    return std::unexpected(manifest_validation.error());
  }
  return contents;
}

auto BuildConfigDocumentsForWrite(const ConfigTexts& texts) -> SourceDocumentBatch {
  return SourceDocumentBatch{
      SourceDocument{.display_path = std::string(kConfigFileNames[0]),
                     .text = texts.validator_text},
      SourceDocument{.display_path = std::string(kConfigFileNames[1]),
                     .text = texts.modifier_text},
      SourceDocument{.display_path = std::string(kConfigFileNames[2]),
                     .text = texts.export_formats_text},
  };
}

auto BuildBackupConfigDocumentsForWrite(std::string validator_text,
                                        std::string modifier_text)
    -> SourceDocumentBatch {
  return SourceDocumentBatch{
      SourceDocument{.display_path = std::string(kBackupConfigFileNames[0]),
                     .text = std::move(validator_text)},
      SourceDocument{.display_path = std::string(kBackupConfigFileNames[1]),
                     .text = std::move(modifier_text)},
  };
}

auto CaptureSnapshots(const std::filesystem::path& root_path,
                      const SourceDocumentBatch& documents)
    -> Result<std::vector<FileSnapshot>> {
  std::vector<FileSnapshot> snapshots;
  snapshots.reserve(documents.size());
  for (const auto& document : documents) {
    const std::filesystem::path absolute_path =
        root_path / std::filesystem::path(document.display_path);
    if (std::filesystem::exists(absolute_path)) {
      if (!std::filesystem::is_regular_file(absolute_path)) {
        return std::unexpected(MakeError(
            "Import target must be a file path: " + absolute_path.string(),
            kContext));
      }
      const auto current_text = SourceDocumentIo::ReadText(absolute_path);
      if (!current_text) {
        return std::unexpected(current_text.error());
      }
      snapshots.push_back(FileSnapshot{
          .relative_path = std::filesystem::path(document.display_path),
          .previous_text = *current_text,
      });
      continue;
    }
    snapshots.push_back(FileSnapshot{
        .relative_path = std::filesystem::path(document.display_path),
        .previous_text = std::nullopt,
    });
  }
  return snapshots;
}

auto PruneEmptyDirectories(const std::filesystem::path& file_path,
                           const std::filesystem::path& root_path) -> void {
  std::error_code error;
  for (std::filesystem::path current = file_path.parent_path();
       !current.empty() && current != root_path;
       current = current.parent_path()) {
    if (!std::filesystem::exists(current) || !std::filesystem::is_directory(current)) {
      break;
    }
    if (!std::filesystem::is_empty(current, error) || error) {
      break;
    }
    std::filesystem::remove(current, error);
    if (error) {
      break;
    }
  }
}

auto RestoreSnapshots(const std::filesystem::path& root_path,
                      const std::vector<FileSnapshot>& snapshots) -> Result<void> {
  for (const auto& snapshot : snapshots) {
    const std::filesystem::path absolute_path = root_path / snapshot.relative_path;
    if (snapshot.previous_text.has_value()) {
      const auto write_result =
          SourceDocumentIo::WriteText(absolute_path, *snapshot.previous_text);
      if (!write_result) {
        return std::unexpected(write_result.error());
      }
      continue;
    }

    std::error_code remove_error;
    if (std::filesystem::exists(absolute_path)) {
      std::filesystem::remove(absolute_path, remove_error);
      if (remove_error) {
        return std::unexpected(MakeError(
            "Failed to remove imported file during rollback: " +
                absolute_path.string(),
            kContext));
      }
      PruneEmptyDirectories(absolute_path, root_path);
    }
  }
  return {};
}

auto LoadWorkspaceRecordDocuments(const std::filesystem::path& records_root)
    -> Result<SourceDocumentBatch> {
  if (!std::filesystem::exists(records_root)) {
    return SourceDocumentBatch{};
  }
  if (!std::filesystem::is_directory(records_root)) {
    return std::unexpected(MakeError(
        "Workspace records root must be a directory: " + records_root.string(),
        kContext));
  }
  return SourceDocumentIo::LoadByExtensionRelative(records_root, ".txt");
}

auto MergeDocumentsByDisplayPath(const SourceDocumentBatch& first,
                                 const SourceDocumentBatch& second)
    -> SourceDocumentBatch {
  SourceDocumentBatch merged;
  std::set<std::string> seen_paths;
  auto append_unique = [&](const SourceDocumentBatch& documents) {
    for (const auto& document : documents) {
      if (!seen_paths.insert(document.display_path).second) {
        continue;
      }
      merged.push_back(document);
    }
  };
  append_unique(first);
  append_unique(second);
  return merged;
}

auto RemoveTxtFilesRecursively(const std::filesystem::path& records_root) -> Result<void> {
  if (!std::filesystem::exists(records_root)) {
    return {};
  }

  std::vector<std::filesystem::path> files_to_remove;
  for (const auto& entry : std::filesystem::recursive_directory_iterator(records_root)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    if (!HasTxtExtension(entry.path())) {
      continue;
    }
    files_to_remove.push_back(entry.path());
  }

  std::error_code error;
  for (const auto& file_path : files_to_remove) {
    std::filesystem::remove(file_path, error);
    if (error) {
      return std::unexpected(MakeError(
          "Failed to remove TXT file during restore: " + file_path.string(),
          kContext));
    }
    PruneEmptyDirectories(file_path, records_root);
  }
  return {};
}

auto ComposeImportError(std::string message, const Error& cause,
                        const std::optional<Error>& rollback_error = std::nullopt)
    -> Error {
  std::string full_message = std::move(message);
  full_message += ": ";
  full_message += FormatError(cause);
  if (rollback_error.has_value()) {
    full_message += " | rollback_failed: ";
    full_message += FormatError(*rollback_error);
  }
  return MakeError(std::move(full_message), kContext);
}

auto WriteArchiveAtomically(const std::filesystem::path& output_zip,
                            const std::vector<ZipArchiveTextEntry>& entries)
    -> Result<void> {
  const std::filesystem::path temp_output =
      output_zip.parent_path() /
      (output_zip.filename().string() + ".tmp");
  std::error_code cleanup_error;
  std::filesystem::remove(temp_output, cleanup_error);

  const auto write_result = ZipArchiveIo::WriteTextEntries(temp_output, entries);
  if (!write_result) {
    return std::unexpected(write_result.error());
  }

  std::error_code replace_error;
  if (std::filesystem::exists(output_zip)) {
    std::filesystem::remove(output_zip, replace_error);
    if (replace_error) {
      std::filesystem::remove(temp_output, cleanup_error);
      return std::unexpected(MakeError(
          "Failed to replace existing ZIP archive: " + output_zip.string(),
          kContext));
    }
  }
  std::filesystem::rename(temp_output, output_zip, replace_error);
  if (replace_error) {
    std::filesystem::remove(temp_output, cleanup_error);
    return std::unexpected(MakeError(
        "Failed to move ZIP archive into place: " + output_zip.string(),
        kContext));
  }
  return {};
}

auto MakeImportParseBundleFailure(std::string phase, std::string message)
    -> ParseBundleImportResult {
  ParseBundleImportResult result;
  result.ok = false;
  result.message = std::move(message);
  result.failed_phase = std::move(phase);
  return result;
}

auto MakeImportBackupBundleFailure(std::string phase, std::string message)
    -> BackupBundleImportResult {
  BackupBundleImportResult result;
  result.ok = false;
  result.message = std::move(message);
  result.failed_phase = std::move(phase);
  return result;
}

auto LoadRuntimeConfig(const std::filesystem::path& config_dir)
    -> Result<RuntimeConfigBundle> {
  const auto validated_context = LoadValidatedConfigContext(config_dir);
  if (!validated_context) {
    return std::unexpected(validated_context.error());
  }
  return validated_context->validated.runtime_config;
}

auto EnsureDbParentExists(const std::filesystem::path& db_path) -> Result<void> {
  const std::filesystem::path db_parent = db_path.parent_path();
  if (db_parent.empty()) {
    return {};
  }
  std::error_code create_error;
  std::filesystem::create_directories(db_parent, create_error);
  if (create_error) {
    return std::unexpected(MakeError(
        "Failed to prepare database directory: " + db_parent.string(), kContext));
  }
  return {};
}

auto LoadTableColumns(sqlite3* db_connection, const std::string& table_name)
    -> std::set<std::string> {
  sqlite3_stmt* statement = nullptr;
  const std::string sql = "PRAGMA table_info(" + table_name + ");";
  if (sqlite3_prepare_v2(db_connection, sql.c_str(), -1, &statement, nullptr) !=
      SQLITE_OK) {
    throw std::runtime_error("Failed to inspect database schema.");
  }

  std::set<std::string> columns;
  while (sqlite3_step(statement) == SQLITE_ROW) {
    const auto* name =
        reinterpret_cast<const char*>(sqlite3_column_text(statement, 1));
    if (name != nullptr) {
      columns.emplace(name);
    }
  }
  sqlite3_finalize(statement);
  return columns;
}

auto ContainsRequiredColumns(const std::set<std::string>& columns,
                             const std::vector<std::string>& required_columns)
    -> bool {
  for (const auto& column : required_columns) {
    if (!columns.contains(column)) {
      return false;
    }
  }
  return true;
}

auto RemoveDatabaseFamily(const std::filesystem::path& db_path) -> void {
  std::error_code error;
  std::filesystem::remove(db_path, error);
  std::filesystem::remove(db_path.string() + "-wal", error);
  std::filesystem::remove(db_path.string() + "-shm", error);
}

auto DatabaseFamilyPaths(const std::filesystem::path& db_path)
    -> std::array<std::filesystem::path, 3U> {
  return {db_path, db_path.string() + "-wal", db_path.string() + "-shm"};
}

auto MoveDatabaseFamily(const std::filesystem::path& from_path,
                        const std::filesystem::path& to_path) -> Result<void> {
  const auto from_family = DatabaseFamilyPaths(from_path);
  const auto to_family = DatabaseFamilyPaths(to_path);
  for (std::size_t index = 0U; index < from_family.size(); ++index) {
    if (!std::filesystem::exists(from_family[index])) {
      continue;
    }
    std::error_code rename_error;
    std::filesystem::rename(from_family[index], to_family[index], rename_error);
    if (rename_error) {
      return std::unexpected(MakeError(
          "Failed to move database artifact into place: " +
              from_family[index].string() + " -> " + to_family[index].string(),
          kContext));
    }
  }
  return {};
}

auto PromoteStagedDatabaseFamily(const std::filesystem::path& staged_db_path,
                                 const std::filesystem::path& target_db_path)
    -> Result<void> {
  const std::filesystem::path backup_db_path =
      target_db_path.parent_path() /
      (target_db_path.filename().string() + ".backup_prev");

  RemoveDatabaseFamily(backup_db_path);
  bool moved_existing_to_backup = false;
  if (std::filesystem::exists(target_db_path) ||
      std::filesystem::exists(target_db_path.string() + "-wal") ||
      std::filesystem::exists(target_db_path.string() + "-shm")) {
    const auto move_existing = MoveDatabaseFamily(target_db_path, backup_db_path);
    if (!move_existing) {
      RemoveDatabaseFamily(staged_db_path);
      return std::unexpected(move_existing.error());
    }
    moved_existing_to_backup = true;
  }

  const auto promote_result = MoveDatabaseFamily(staged_db_path, target_db_path);
  if (!promote_result) {
    if (moved_existing_to_backup) {
      const auto rollback_result = MoveDatabaseFamily(backup_db_path, target_db_path);
      if (!rollback_result) {
        return std::unexpected(MakeError(
            FormatError(promote_result.error()) +
                " | rollback_failed: " + FormatError(rollback_result.error()),
            kContext));
      }
    }
    return std::unexpected(promote_result.error());
  }

  RemoveDatabaseFamily(backup_db_path);
  return {};
}

auto ResetLegacyDatabaseIfNeeded(const std::filesystem::path& db_path) -> bool {
  if (!std::filesystem::exists(db_path)) {
    return false;
  }

  sqlite3* db_connection = nullptr;
  const int open_result = sqlite3_open_v2(db_path.string().c_str(), &db_connection,
                                          SQLITE_OPEN_READONLY, nullptr);
  if (open_result != SQLITE_OK) {
    if (db_connection != nullptr) {
      sqlite3_close(db_connection);
    }
    RemoveDatabaseFamily(db_path);
    return true;
  }

  bool should_reset = false;
  try {
    const auto bills_columns = LoadTableColumns(db_connection, "bills");
    const auto transactions_columns = LoadTableColumns(db_connection, "transactions");

    const std::vector<std::string> required_bills_columns = {
        "bill_date", "year", "month", "remark", "total_income",
        "total_expense", "balance"};
    const std::vector<std::string> required_transaction_columns = {
        "bill_id", "parent_category", "sub_category", "description", "amount",
        "source", "comment", "transaction_type"};

    should_reset =
        !ContainsRequiredColumns(bills_columns, required_bills_columns) ||
        !ContainsRequiredColumns(transactions_columns, required_transaction_columns);
  } catch (...) {
    should_reset = true;
  }

  sqlite3_close(db_connection);
  if (should_reset) {
    RemoveDatabaseFamily(db_path);
  }
  return should_reset;
}

auto PrepareDatabaseForIngest(const std::filesystem::path& db_path,
                              bool reset_legacy_database) -> Result<bool> {
  const auto ensure_parent = EnsureDbParentExists(db_path);
  if (!ensure_parent) {
    return std::unexpected(ensure_parent.error());
  }
  if (!reset_legacy_database) {
    return false;
  }
  return ResetLegacyDatabaseIfNeeded(db_path);
}

auto IsMissingBillsTableError(std::string_view message) -> bool {
  return message.find("no such table: bills") != std::string_view::npos;
}

auto ExtractPeriodFromDocumentText(std::string_view text)
    -> std::optional<std::string> {
  const std::size_t line_end = text.find('\n');
  std::string_view first_line =
      text.substr(0U, line_end == std::string_view::npos ? text.size() : line_end);
  if (!first_line.empty() && first_line.back() == '\r') {
    first_line.remove_suffix(1U);
  }
  return bills::core::common::iso_period::extract_year_month_from_date_header(
      first_line);
}

auto BuildRecordWorkspaceRelativePath(std::string_view period) -> Result<std::string> {
  const auto parsed_period = bills::core::common::iso_period::parse_year_month(period);
  if (!parsed_period.has_value()) {
    return std::unexpected(MakeError(
        "Invalid record period for workspace import: " + std::string(period), kContext));
  }
  return std::to_string(parsed_period->year) + "/" + std::string(period) + ".txt";
}

auto BuildBatchFailureMessage(const BillWorkflowBatchResult& result,
                              std::string_view fallback) -> std::string {
  for (const auto& file : result.files) {
    if (file.ok) {
      continue;
    }
    if (!file.stage.empty() && !file.error.empty()) {
      return file.stage + ": " + file.error;
    }
    if (!file.error.empty()) {
      return file.error;
    }
  }
  return std::string(fallback);
}

auto AppendRollbackFailure(std::string message, const Result<void>& rollback_result)
    -> std::string {
  if (!rollback_result) {
    message += " | rollback_failed: " + FormatError(rollback_result.error());
  }
  return message;
}

auto MakeRecordCommitFailure(std::string_view period, std::string relative_path,
                             std::string message) -> HostRecordCommitResult {
  return HostRecordCommitResult{
      .ok = false,
      .message = message,
      .relative_path = std::move(relative_path),
      .period = std::string(period),
      .overwritten = false,
      .error_message = message,
  };
}

auto PreviewInlineRecordDocument(std::string_view expected_period,
                                 std::string_view raw_text,
                                 const std::filesystem::path& config_dir)
    -> Result<RecordPreviewFile> {
  const auto target_relative = BuildRecordWorkspaceRelativePath(expected_period);
  if (!target_relative) {
    return std::unexpected(target_relative.error());
  }

  const auto runtime_config = LoadRuntimeConfig(config_dir);
  if (!runtime_config) {
    return std::unexpected(runtime_config.error());
  }

  SourceDocumentBatch documents;
  documents.push_back(SourceDocument{
      .display_path = *target_relative,
      .text = std::string(raw_text),
  });
  const auto preview_result = RecordTemplateService::PreviewRecords(
      documents, *runtime_config, *target_relative);
  if (!preview_result) {
    return std::unexpected(
        MakeError(FormatRecordTemplateError(preview_result.error()), kContext));
  }
  if (preview_result->files.size() != 1U) {
    return std::unexpected(MakeError(
        "Expected a single preview result for the provided record text.",
        kContext));
  }
  return preview_result->files.front();
}

auto CommitValidatedRecordTextToWorkspaceAndDatabase(
    std::string_view period, std::string_view raw_text,
    const std::filesystem::path& config_dir,
    const std::filesystem::path& records_root,
    const std::filesystem::path& db_path) -> HostRecordCommitResult {
  const auto target_relative = BuildRecordWorkspaceRelativePath(period);
  if (!target_relative) {
    return MakeRecordCommitFailure(period, {}, FormatError(target_relative.error()));
  }

  const SourceDocumentBatch target_documents = {
      SourceDocument{
          .display_path = *target_relative,
          .text = std::string(raw_text),
      },
  };
  const auto snapshots = CaptureSnapshots(records_root, target_documents);
  if (!snapshots) {
    return MakeRecordCommitFailure(
        period, *target_relative,
        "Failed to prepare " + *target_relative + " for save: " +
            FormatError(snapshots.error()));
  }

  const bool existed =
      !snapshots->empty() && snapshots->front().previous_text.has_value();
  const std::filesystem::path target_path =
      records_root / std::filesystem::path(*target_relative);
  const auto write_result = SourceDocumentIo::WriteText(target_path, raw_text);
  if (!write_result) {
    return MakeRecordCommitFailure(
        period, *target_relative,
        "Failed to write " + *target_relative + ": " +
            FormatError(write_result.error()));
  }

  const auto sync_result =
      SyncSingleRecordToDatabase(target_path, config_dir, db_path, period);
  if (!sync_result) {
    const auto rollback_result = RestoreSnapshots(records_root, *snapshots);
    return MakeRecordCommitFailure(
        period, *target_relative,
        AppendRollbackFailure(
            "Failed to sync " + *target_relative + " into SQLite: " +
                FormatError(sync_result.error()),
            rollback_result));
  }
  if (!sync_result->period_matches) {
    const auto rollback_result = RestoreSnapshots(records_root, *snapshots);
    return MakeRecordCommitFailure(
        period, *target_relative,
        AppendRollbackFailure(
            "TXT header period '" + sync_result->actual_period +
                "' does not match selected period '" + std::string(period) + "'.",
            rollback_result));
  }
  if (sync_result->ingest.failure > 0U) {
    const auto rollback_result = RestoreSnapshots(records_root, *snapshots);
    return MakeRecordCommitFailure(
        period, *target_relative,
        AppendRollbackFailure(
            BuildBatchFailureMessage(
                sync_result->ingest,
                "Failed to sync " + *target_relative + " into SQLite."),
            rollback_result));
  }

  return HostRecordCommitResult{
      .ok = true,
      .message = "Saved " + *target_relative + " and synced it to the database.",
      .relative_path = *target_relative,
      .period = std::string(period),
      .overwritten = existed,
      .error_message = {},
  };
}

auto LoadSourceDocumentViews(const std::filesystem::path& input_path)
    -> Result<std::map<std::string, SourceDocumentView>> {
  const auto absolute_documents = LoadSourceDocuments(input_path, ".txt");
  if (!absolute_documents) {
    return std::unexpected(absolute_documents.error());
  }
  const auto relative_documents =
      SourceDocumentIo::LoadByExtensionRelative(input_path, ".txt");
  if (!relative_documents) {
    return std::unexpected(relative_documents.error());
  }
  if (absolute_documents->size() != relative_documents->size()) {
    return std::unexpected(MakeError(
        "Absolute and relative TXT document views are inconsistent.", kContext));
  }

  std::map<std::string, SourceDocumentView> views;
  for (std::size_t index = 0; index < absolute_documents->size(); ++index) {
    views.emplace((*absolute_documents)[index].display_path,
                  SourceDocumentView{
                      .relative_path = (*relative_documents)[index].display_path,
                      .text = (*absolute_documents)[index].text,
                  });
  }
  return views;
}

auto CountMatchingYearBills(sqlite3* db_connection, std::string_view iso_year)
    -> int {
  const char* sql = "SELECT COUNT(*) FROM bills WHERE substr(bill_date, 1, 4) = ?;";
  sqlite3_stmt* statement = nullptr;
  if (sqlite3_prepare_v2(db_connection, sql, -1, &statement, nullptr) != SQLITE_OK) {
    throw std::runtime_error("Failed to prepare year bill count query.");
  }

  sqlite3_bind_text(statement, 1, iso_year.data(),
                    static_cast<int>(iso_year.size()), SQLITE_TRANSIENT);

  int result = 0;
  if (sqlite3_step(statement) == SQLITE_ROW) {
    result = sqlite3_column_int(statement, 0);
  }
  sqlite3_finalize(statement);
  return result;
}

auto CountMatchingMonthBills(sqlite3* db_connection, std::string_view iso_month)
    -> int {
  const char* sql = "SELECT COUNT(*) FROM bills WHERE bill_date = ?;";
  sqlite3_stmt* statement = nullptr;
  if (sqlite3_prepare_v2(db_connection, sql, -1, &statement, nullptr) != SQLITE_OK) {
    throw std::runtime_error("Failed to prepare month bill count query.");
  }

  sqlite3_bind_text(statement, 1, iso_month.data(),
                    static_cast<int>(iso_month.size()), SQLITE_TRANSIENT);

  int result = 0;
  if (sqlite3_step(statement) == SQLITE_ROW) {
    result = sqlite3_column_int(statement, 0);
  }
  sqlite3_finalize(statement);
  return result;
}

auto CountTransactions(const MonthlyReportData& report) -> std::size_t {
  std::size_t count = 0;
  for (const auto& [_, parent] : report.aggregated_data) {
    for (const auto& [__, sub] : parent.sub_categories) {
      count += sub.transactions.size();
    }
  }
  return count;
}

auto BuildEmptyChartData() -> nlohmann::json {
  return {
      {"schema_version", "1.0.0"},
      {"views", nlohmann::json::array()},
  };
}

auto StableColorIndex(std::string_view key, std::size_t palette_size) -> std::size_t {
  constexpr std::uint64_t kFnvOffsetBasis = 14695981039346656037ULL;
  constexpr std::uint64_t kFnvPrime = 1099511628211ULL;
  std::uint64_t hash = kFnvOffsetBasis;
  for (const unsigned char character : key) {
    hash ^= character;
    hash *= kFnvPrime;
  }
  return static_cast<std::size_t>(hash % palette_size);
}

auto FixedPieChartPalette() -> const std::array<std::string_view, 8U>& {
  static const std::array<std::string_view, 8U> palette = {
      "#2563EB", "#DC2626", "#059669", "#D97706",
      "#7C3AED", "#DB2777", "#0891B2", "#65A30D",
  };
  return palette;
}

auto ResolvePieChartColorHex(std::string_view category_key) -> std::string {
  const auto& palette = FixedPieChartPalette();
  return std::string(
      palette[StableColorIndex(category_key, palette.size())]);
}

auto ResolveGroupedBarSeriesColorHex(std::string_view series_id) -> std::string {
  if (series_id == "income") {
    return "#2563EB";
  }
  if (series_id == "expense") {
    return "#DC2626";
  }
  if (series_id == "balance") {
    return "#7C3AED";
  }
  return "#7C3AED";
}

auto BuildYearlyMonthlyOverviewChart(const YearlyReportData& report)
    -> std::optional<nlohmann::json> {
  if (!report.data_found || report.monthly_summary.empty()) {
    return std::nullopt;
  }

  std::array<double, 12U> income_values{};
  std::array<double, 12U> expense_values{};
  std::array<double, 12U> balance_values{};
  for (const auto& [month, summary] : report.monthly_summary) {
    if (month < 1 || month > 12) {
      continue;
    }
    const std::size_t index = static_cast<std::size_t>(month - 1);
    income_values[index] = summary.income;
    expense_values[index] = std::abs(summary.expense);
    balance_values[index] = summary.income + summary.expense;
  }

  nlohmann::json x_labels = nlohmann::json::array();
  for (int month = 1; month <= 12; ++month) {
    std::ostringstream stream;
    stream << std::setw(2) << std::setfill('0') << month;
    x_labels.push_back(stream.str());
  }

  const auto to_json_array = [](const auto& values) -> nlohmann::json {
    nlohmann::json result = nlohmann::json::array();
    for (const double value : values) {
      result.push_back(value);
    }
    return result;
  };

  return nlohmann::json{
      {"id", "yearly_monthly_overview"},
      {"title", "Monthly Income, Expense, and Balance"},
      {"chart_type", "grouped_bar"},
      {"x_labels", std::move(x_labels)},
      {"series",
       nlohmann::json::array(
           {nlohmann::json{{"id", "income"},
                           {"label", "Income"},
                           {"unit", "CNY"},
                           {"color", ResolveGroupedBarSeriesColorHex("income")},
                           {"values", to_json_array(income_values)}},
            nlohmann::json{{"id", "expense"},
                           {"label", "Expense"},
                           {"unit", "CNY"},
                           {"color", ResolveGroupedBarSeriesColorHex("expense")},
                           {"values", to_json_array(expense_values)}},
            nlohmann::json{{"id", "balance"},
                           {"label", "Balance"},
                           {"unit", "CNY"},
                           {"color", ResolveGroupedBarSeriesColorHex("balance")},
                           {"values", to_json_array(balance_values)}}})},
  };
}

auto BuildMonthlyExpenseByCategoryChart(const MonthlyReportData& report)
    -> std::optional<nlohmann::json> {
  if (!report.data_found) {
    return std::nullopt;
  }

  struct Segment {
    std::string id;
    std::string label;
    double value = 0.0;
  };

  std::vector<Segment> segments;
  segments.reserve(report.aggregated_data.size());
  for (const auto& [category_name, category] : report.aggregated_data) {
    if (category.parent_total >= 0.0) {
      continue;
    }
    const double absolute_value = std::abs(category.parent_total);
    if (absolute_value <= 0.0) {
      continue;
    }
    const std::string normalized_name =
        category_name.empty() ? "uncategorized" : category_name;
    segments.push_back(Segment{
        .id = normalized_name,
        .label = normalized_name,
        .value = absolute_value,
    });
  }

  if (segments.empty()) {
    return std::nullopt;
  }

  std::sort(segments.begin(), segments.end(),
            [](const Segment& left, const Segment& right) -> bool {
              if (left.value == right.value) {
                return left.label < right.label;
              }
              return left.value > right.value;
            });

  nlohmann::json json_segments = nlohmann::json::array();
  for (const auto& segment : segments) {
    json_segments.push_back(nlohmann::json{
        {"id", segment.id},
        {"label", segment.label},
        {"value", segment.value},
        {"color", ResolvePieChartColorHex(segment.id)},
    });
  }

  return nlohmann::json{
      {"id", "monthly_expense_by_category"},
      {"title", "Expense by Category"},
      {"chart_type", "pie"},
      {"unit", "CNY"},
      {"segments", std::move(json_segments)},
  };
}

auto BuildChartData(const QueryExecutionResult& query_result) -> nlohmann::json {
  nlohmann::json chart_data = BuildEmptyChartData();
  auto& views = chart_data["views"];
  if (query_result.query_type == "year") {
    const auto yearly_chart =
        BuildYearlyMonthlyOverviewChart(query_result.yearly_data);
    if (yearly_chart.has_value()) {
      views.push_back(*yearly_chart);
    }
    return chart_data;
  }

  if (query_result.query_type == "month") {
    const auto monthly_chart =
        BuildMonthlyExpenseByCategoryChart(query_result.monthly_data);
    if (monthly_chart.has_value()) {
      views.push_back(*monthly_chart);
    }
  }
  return chart_data;
}

auto InjectChartDataIntoStandardReportJson(
    std::string_view standard_report_json,
    const QueryExecutionResult& query_result) -> std::string {
  if (standard_report_json.empty()) {
    return std::string(standard_report_json);
  }

  try {
    nlohmann::json root = nlohmann::json::parse(standard_report_json);
    if (!root.is_object()) {
      return std::string(standard_report_json);
    }
    auto& extensions = root["extensions"];
    if (!extensions.is_object()) {
      extensions = nlohmann::json::object();
    }
    extensions["chart_data"] = BuildChartData(query_result);
    return root.dump(2) + "\n";
  } catch (const nlohmann::json::exception&) {
    return std::string(standard_report_json);
  }
}

auto BuildHostQueryResult(const QueryExecutionResult& query_result,
                          std::string_view query_value,
                          sqlite3* db_connection) -> HostQueryResult {
  HostQueryResult result;
  result.execution = query_result;
  result.standard_report = ReportRenderService::BuildStandardReport(query_result);
  if (StandardReportRendererRegistry::IsFormatAvailable("json")) {
    result.standard_report_json =
        ReportRenderService::Render(result.standard_report, "json");
    result.standard_report_json = InjectChartDataIntoStandardReportJson(
        result.standard_report_json, query_result);
  }
  if (StandardReportRendererRegistry::IsFormatAvailable("md")) {
    result.report_markdown =
        ReportRenderService::Render(result.standard_report, "md");
  }
  if (query_result.query_type == "year") {
    result.matched_bills =
        static_cast<std::size_t>(CountMatchingYearBills(db_connection, query_value));
  } else {
    result.matched_bills =
        static_cast<std::size_t>(CountMatchingMonthBills(db_connection, query_value));
    result.transaction_count = CountTransactions(query_result.monthly_data);
  }
  return result;
}

template <typename Callback>
auto RunTextWorkflow(const std::filesystem::path& input_path,
                     const std::filesystem::path& config_dir, Callback&& callback)
    -> Result<BillWorkflowBatchResult> {
  const auto runtime_config = LoadRuntimeConfig(config_dir);
  if (!runtime_config) {
    return std::unexpected(runtime_config.error());
  }
  const auto documents = LoadSourceDocuments(input_path, ".txt");
  if (!documents) {
    return std::unexpected(documents.error());
  }
  return callback(*documents, *runtime_config);
}

}  // namespace

auto LoadValidatedConfigContext(const std::filesystem::path& config_dir)
    -> Result<HostConfigContext> {
  const auto context = LoadValidatedConfigTextContext(config_dir);
  if (!context) {
    return std::unexpected(context.error());
  }

  return HostConfigContext{
      .documents = context->documents,
      .validated = context->validated,
  };
}

auto LoadSourceDocuments(const std::filesystem::path& root_path,
                         std::string_view extension) -> Result<SourceDocumentBatch> {
  return SourceDocumentIo::LoadByExtension(root_path, extension);
}

auto ListEnabledExportFormats(const std::filesystem::path& config_dir)
    -> Result<std::vector<std::string>> {
  const auto validated_context = LoadValidatedConfigContext(config_dir);
  if (!validated_context) {
    return std::unexpected(validated_context.error());
  }
  return validated_context->validated.report.enabled_export_formats;
}

auto InspectConfig(const std::filesystem::path& config_dir)
    -> Result<HostConfigInspectionResult> {
  const auto validated_context = LoadValidatedConfigContext(config_dir);
  if (!validated_context) {
    return std::unexpected(validated_context.error());
  }
  const auto inspect_result =
      RecordTemplateService::InspectConfig(validated_context->documents.validator);
  if (!inspect_result) {
    return std::unexpected(MakeError(FormatRecordTemplateError(inspect_result.error()),
                                     kContext));
  }
  return HostConfigInspectionResult{
      .inspect = *inspect_result,
      .enabled_export_formats = validated_context->validated.report.enabled_export_formats,
      .available_export_formats = ReportExportService::ListAvailableFormats(),
  };
}

auto ValidateConfigTexts(std::string_view validator_text,
                         std::string_view modifier_text,
                         std::string_view export_formats_text)
    -> Result<HostConfigTextsValidationResult> {
  const ConfigDocumentBundle documents = ConfigDocumentParser::ParseTexts(
      std::string(validator_text), std::string(modifier_text),
      std::string(export_formats_text),
      std::string(kConfigPrefix) + std::string(kConfigFileNames[0]),
      std::string(kConfigPrefix) + std::string(kConfigFileNames[1]),
      std::string(kConfigPrefix) + std::string(kConfigFileNames[2]));
  const auto validated = ConfigBundleService::Validate(documents);
  if (!validated) {
    const Error validation_error = MakeConfigValidationError(validated.error());
    return HostConfigTextsValidationResult{
        .ok = false,
        .message = validation_error.message_,
        .config_validation = validated.error(),
        .enabled_export_formats = validated.error().enabled_export_formats,
        .available_export_formats = validated.error().available_export_formats,
    };
  }

  return HostConfigTextsValidationResult{
      .ok = true,
      .message = "Config texts are valid.",
      .config_validation = validated->report,
      .enabled_export_formats = validated->enabled_export_formats,
      .available_export_formats = validated->available_export_formats,
  };
}

auto GenerateTemplatesFromConfig(
    const std::filesystem::path& config_dir,
    const HostTemplateGenerationRequest& request)
    -> Result<TemplateGenerationResult> {
  const auto validated_context = LoadValidatedConfigContext(config_dir);
  if (!validated_context) {
    return std::unexpected(validated_context.error());
  }
  const auto layout = RecordTemplateService::BuildOrderedTemplateLayout(
      validated_context->documents.validator);
  if (!layout) {
    return std::unexpected(
        MakeError(FormatRecordTemplateError(layout.error()), kContext));
  }

  TemplateGenerationRequest generation_request;
  generation_request.period = request.period;
  generation_request.start_period = request.start_period;
  generation_request.end_period = request.end_period;
  generation_request.start_year = request.start_year;
  generation_request.end_year = request.end_year;
  generation_request.layout = *layout;

  const auto result = RecordTemplateService::GenerateTemplates(generation_request);
  if (!result) {
    return std::unexpected(
        MakeError(FormatRecordTemplateError(result.error()), kContext));
  }
  return *result;
}

auto PreviewRecordDocuments(const std::filesystem::path& input_path,
                            const std::filesystem::path& config_dir)
    -> Result<RecordPreviewResult> {
  const auto runtime_config = LoadRuntimeConfig(config_dir);
  if (!runtime_config) {
    return std::unexpected(runtime_config.error());
  }
  const auto documents = LoadSourceDocuments(input_path, ".txt");
  if (!documents) {
    return std::unexpected(documents.error());
  }
  const auto result = RecordTemplateService::PreviewRecords(
      *documents, *runtime_config, input_path.string());
  if (!result) {
    return std::unexpected(
        MakeError(FormatRecordTemplateError(result.error()), kContext));
  }
  return *result;
}

auto ListRecordPeriods(const std::filesystem::path& input_path)
    -> Result<ListedPeriodsResult> {
  const auto documents = LoadSourceDocuments(input_path, ".txt");
  if (!documents) {
    return std::unexpected(documents.error());
  }
  const auto result =
      RecordTemplateService::ListPeriods(*documents, input_path.string());
  if (!result) {
    return std::unexpected(
        MakeError(FormatRecordTemplateError(result.error()), kContext));
  }
  return *result;
}

auto ValidateDocuments(const std::filesystem::path& input_path,
                       const std::filesystem::path& config_dir)
    -> Result<BillWorkflowBatchResult> {
  return RunTextWorkflow(
      input_path, config_dir,
      [](const SourceDocumentBatch& documents,
         const RuntimeConfigBundle& runtime_config) {
        return Result<BillWorkflowBatchResult>(
            BillWorkflowService::Validate(documents, runtime_config));
      });
}

auto ConvertDocuments(const std::filesystem::path& input_path,
                      const std::filesystem::path& config_dir,
                      bool include_serialized_json)
    -> Result<BillWorkflowBatchResult> {
  return RunTextWorkflow(
      input_path, config_dir,
      [include_serialized_json](const SourceDocumentBatch& documents,
                                const RuntimeConfigBundle& runtime_config) {
        return Result<BillWorkflowBatchResult>(
            BillWorkflowService::Convert(documents, runtime_config,
                                         include_serialized_json));
      });
}

auto IngestDocuments(const std::filesystem::path& input_path,
                     const std::filesystem::path& config_dir,
                     const std::filesystem::path& db_path,
                     bool include_serialized_json)
    -> Result<BillWorkflowBatchResult> {
  const auto ensure_db = EnsureDbParentExists(db_path);
  if (!ensure_db) {
    return std::unexpected(ensure_db.error());
  }
  return RunTextWorkflow(
      input_path, config_dir,
      [&db_path, include_serialized_json](
          const SourceDocumentBatch& documents,
          const RuntimeConfigBundle& runtime_config) -> Result<BillWorkflowBatchResult> {
        auto repository = bills::io::CreateBillRepository(db_path.string());
        return BillWorkflowBatchResult(BillWorkflowService::Ingest(
            documents, runtime_config, *repository, include_serialized_json));
      });
}

auto IngestDocumentsToDatabase(const std::filesystem::path& input_path,
                               const std::filesystem::path& config_dir,
                               const std::filesystem::path& db_path,
                               bool reset_legacy_database,
                               bool include_serialized_json)
    -> Result<HostDatabaseIngestResult> {
  const auto database_reset =
      PrepareDatabaseForIngest(db_path, reset_legacy_database);
  if (!database_reset) {
    return std::unexpected(database_reset.error());
  }

  const auto ingest_result =
      IngestDocuments(input_path, config_dir, db_path, include_serialized_json);
  if (!ingest_result) {
    return std::unexpected(ingest_result.error());
  }

  return HostDatabaseIngestResult{
      .database_reset = *database_reset,
      .ingest = *ingest_result,
  };
}

auto ImportJsonDocuments(const std::filesystem::path& input_path,
                         const std::filesystem::path& db_path)
    -> Result<BillWorkflowBatchResult> {
  const auto ensure_db = EnsureDbParentExists(db_path);
  if (!ensure_db) {
    return std::unexpected(ensure_db.error());
  }
  const auto documents = LoadSourceDocuments(input_path, ".json");
  if (!documents) {
    return std::unexpected(documents.error());
  }
  auto repository = bills::io::CreateBillRepository(db_path.string());
  return BillWorkflowBatchResult(
      BillWorkflowService::ImportJson(*documents, *repository));
}

auto ImportRecordDirectoryToWorkspace(const std::filesystem::path& input_path,
                                      const std::filesystem::path& config_dir,
                                      const std::filesystem::path& records_root)
    -> Result<HostRecordDirectoryImportResult> {
  const auto preview_result = PreviewRecordDocuments(input_path, config_dir);
  if (!preview_result) {
    return std::unexpected(preview_result.error());
  }
  const auto source_views = LoadSourceDocumentViews(input_path);
  if (!source_views) {
    return std::unexpected(source_views.error());
  }

  HostRecordDirectoryImportResult result;
  result.processed = source_views->size();
  if (preview_result->files.empty() && !source_views->empty()) {
    return std::unexpected(MakeError(
        "No preview results were returned for the provided TXT documents.",
        kContext));
  }

  std::map<std::string, RecordPreviewFile> preview_by_path;
  for (const auto& file : preview_result->files) {
    preview_by_path.emplace(file.path, file);
  }

  std::vector<RecordImportCandidate> valid_candidates;
  valid_candidates.reserve(source_views->size());
  for (const auto& [absolute_path, view] : *source_views) {
    const auto preview_it = preview_by_path.find(absolute_path);
    if (preview_it == preview_by_path.end()) {
      ++result.failure;
      if (result.first_failure_message.empty()) {
        result.first_failure_message =
            "No validation result was returned for " + view.relative_path + ".";
      }
      continue;
    }

    const auto& preview_file = preview_it->second;
    if (!preview_file.ok || preview_file.period.empty()) {
      ++result.failure;
      ++result.invalid;
      if (result.first_failure_message.empty()) {
        result.first_failure_message =
            !preview_file.error.empty()
                ? preview_file.error
                : "TXT validation failed for " + view.relative_path + ".";
      }
      continue;
    }

    valid_candidates.push_back(RecordImportCandidate{
        .absolute_path = absolute_path,
        .relative_path = view.relative_path,
        .period = preview_file.period,
        .text = view.text,
    });
  }

  std::map<std::string, std::size_t> period_counts;
  for (const auto& candidate : valid_candidates) {
    ++period_counts[candidate.period];
  }

  for (const auto& candidate : valid_candidates) {
    if (period_counts[candidate.period] > 1U) {
      ++result.failure;
      ++result.duplicate_period_conflicts;
      if (result.first_failure_message.empty()) {
        result.first_failure_message =
            "Duplicate period '" + candidate.period +
            "' was found in the selected directory for " +
            candidate.relative_path + ".";
      }
      continue;
    }

    const auto target_relative =
        BuildRecordWorkspaceRelativePath(candidate.period);
    if (!target_relative) {
      ++result.failure;
      if (result.first_failure_message.empty()) {
        result.first_failure_message = FormatError(target_relative.error());
      }
      continue;
    }

    const std::filesystem::path target_path =
        records_root / std::filesystem::path(*target_relative);
    const bool existed = std::filesystem::is_regular_file(target_path);
    const auto write_result = SourceDocumentIo::WriteText(target_path, candidate.text);
    if (!write_result) {
      ++result.failure;
      if (result.first_failure_message.empty()) {
        result.first_failure_message =
            "Failed to write " + candidate.relative_path + " to " +
            *target_relative + ": " + FormatError(write_result.error());
      }
      continue;
    }

    ++result.imported;
    if (existed) {
      ++result.overwritten;
    }
  }

  return result;
}

auto CommitRecordTextToWorkspaceAndDatabase(
    std::string_view expected_period, std::string_view raw_text,
    const std::filesystem::path& config_dir,
    const std::filesystem::path& records_root,
    const std::filesystem::path& db_path) -> HostRecordCommitResult {
  const auto target_relative = BuildRecordWorkspaceRelativePath(expected_period);
  if (!target_relative) {
    return MakeRecordCommitFailure(
        expected_period, {}, FormatError(target_relative.error()));
  }

  const auto preview_file =
      PreviewInlineRecordDocument(expected_period, raw_text, config_dir);
  if (!preview_file) {
    return MakeRecordCommitFailure(
        expected_period, *target_relative, FormatError(preview_file.error()));
  }
  if (!preview_file->ok || preview_file->period.empty()) {
    return MakeRecordCommitFailure(
        expected_period, *target_relative,
        !preview_file->error.empty()
            ? preview_file->error
            : "TXT validation failed for " + *target_relative + ".");
  }
  if (preview_file->period != expected_period) {
    return MakeRecordCommitFailure(
        expected_period, *target_relative,
        "TXT header period '" + preview_file->period +
            "' does not match selected period '" +
            std::string(expected_period) + "'.");
  }

  return CommitValidatedRecordTextToWorkspaceAndDatabase(
      preview_file->period, raw_text, config_dir, records_root, db_path);
}

auto ImportRecordDirectoryAndSyncDatabase(
    const std::filesystem::path& input_path,
    const std::filesystem::path& config_dir,
    const std::filesystem::path& records_root,
    const std::filesystem::path& db_path) -> HostRecordDirectoryImportResult {
  HostRecordDirectoryImportResult result;

  const auto preview_result = PreviewRecordDocuments(input_path, config_dir);
  if (!preview_result) {
    result.failure = 1U;
    result.first_failure_message = FormatError(preview_result.error());
    return result;
  }
  const auto source_views = LoadSourceDocumentViews(input_path);
  if (!source_views) {
    result.failure = 1U;
    result.first_failure_message = FormatError(source_views.error());
    return result;
  }

  result.processed = source_views->size();
  if (preview_result->files.empty() && !source_views->empty()) {
    result.failure = source_views->size();
    result.first_failure_message =
        "No preview results were returned for the provided TXT documents.";
    return result;
  }

  std::map<std::string, RecordPreviewFile> preview_by_path;
  for (const auto& file : preview_result->files) {
    preview_by_path.emplace(file.path, file);
  }

  std::vector<RecordImportCandidate> valid_candidates;
  valid_candidates.reserve(source_views->size());
  for (const auto& [absolute_path, view] : *source_views) {
    const auto preview_it = preview_by_path.find(absolute_path);
    if (preview_it == preview_by_path.end()) {
      ++result.failure;
      if (result.first_failure_message.empty()) {
        result.first_failure_message =
            "No validation result was returned for " + view.relative_path + ".";
      }
      continue;
    }

    const auto& preview_file = preview_it->second;
    if (!preview_file.ok || preview_file.period.empty()) {
      ++result.failure;
      ++result.invalid;
      if (result.first_failure_message.empty()) {
        result.first_failure_message =
            !preview_file.error.empty()
                ? preview_file.error
                : "TXT validation failed for " + view.relative_path + ".";
      }
      continue;
    }

    valid_candidates.push_back(RecordImportCandidate{
        .absolute_path = absolute_path,
        .relative_path = view.relative_path,
        .period = preview_file.period,
        .text = view.text,
    });
  }

  std::map<std::string, std::size_t> period_counts;
  for (const auto& candidate : valid_candidates) {
    ++period_counts[candidate.period];
  }

  for (const auto& candidate : valid_candidates) {
    if (period_counts[candidate.period] > 1U) {
      ++result.failure;
      ++result.duplicate_period_conflicts;
      if (result.first_failure_message.empty()) {
        result.first_failure_message =
            "Duplicate period '" + candidate.period +
            "' was found in the selected directory for " +
            candidate.relative_path + ".";
      }
      continue;
    }

    const auto commit_result = CommitValidatedRecordTextToWorkspaceAndDatabase(
        candidate.period, candidate.text, config_dir, records_root, db_path);
    if (!commit_result.ok) {
      ++result.failure;
      if (result.first_failure_message.empty()) {
        result.first_failure_message =
            !commit_result.error_message.empty() ? commit_result.error_message
                                                 : commit_result.message;
      }
      continue;
    }

    ++result.imported;
    if (commit_result.overwritten) {
      ++result.overwritten;
    }
  }

  return result;
}

auto ExtractSingleRecordPeriod(const std::filesystem::path& input_path)
    -> Result<std::string> {
  const auto documents = LoadSourceDocuments(input_path, ".txt");
  if (!documents) {
    return std::unexpected(documents.error());
  }
  if (documents->size() != 1U) {
    return std::unexpected(
        MakeError("Expected a single TXT file for database sync.", kContext));
  }

  const auto actual_period =
      ExtractPeriodFromDocumentText(documents->front().text);
  if (!actual_period.has_value()) {
    return std::unexpected(
        MakeError("The first line must be 'date:YYYY-MM'.", kContext));
  }
  return *actual_period;
}

auto SyncSingleRecordToDatabase(const std::filesystem::path& input_path,
                                const std::filesystem::path& config_dir,
                                const std::filesystem::path& db_path,
                                std::string_view expected_period,
                                bool include_serialized_json)
    -> Result<HostRecordDatabaseSyncResult> {
  const auto actual_period = ExtractSingleRecordPeriod(input_path);
  if (!actual_period) {
    return std::unexpected(actual_period.error());
  }

  if (*actual_period != expected_period) {
    return HostRecordDatabaseSyncResult{
        .period_matches = false,
        .actual_period = *actual_period,
    };
  }

  const auto ingest_result =
      IngestDocumentsToDatabase(input_path, config_dir, db_path, false,
                                include_serialized_json);
  if (!ingest_result) {
    return std::unexpected(ingest_result.error());
  }

  return HostRecordDatabaseSyncResult{
      .period_matches = true,
      .actual_period = *actual_period,
      .ingest = std::move(ingest_result->ingest),
  };
}

auto PreflightImportDocuments(
    const std::filesystem::path& input_path,
    const std::filesystem::path& config_dir,
    const std::vector<std::string>& existing_workspace_periods,
    const std::vector<std::string>& existing_db_periods)
    -> Result<ImportPreflightResult> {
  const auto validated_context = LoadValidatedConfigContext(config_dir);
  if (!validated_context) {
    return std::unexpected(validated_context.error());
  }
  const auto documents = LoadSourceDocuments(input_path, ".txt");
  if (!documents) {
    return std::unexpected(documents.error());
  }
  ImportPreflightRequest request;
  request.input_label = input_path.string();
  request.documents = *documents;
  request.config_validation = validated_context->validated.report;
  request.config_bundle = validated_context->validated.runtime_config;
  request.existing_workspace_periods = existing_workspace_periods;
  request.existing_db_periods = existing_db_periods;
  const auto result = ImportPreflightService::Run(request);
  if (!result) {
    return std::unexpected(
        MakeError(FormatRecordTemplateError(result.error()), kContext));
  }
  return *result;
}

auto QueryYearReport(const std::filesystem::path& db_path,
                     std::string_view iso_year) -> Result<HostQueryResult> {
  try {
    auto db_session = bills::io::CreateReportDbSession(db_path.string());
    auto report_data_gateway =
        bills::io::CreateReportDataGateway(db_session->GetConnectionHandle());
    const auto query_result = QueryService::QueryYear(*report_data_gateway, iso_year);
    if (!query_result.data_found) {
      return HostQueryResult{.execution = query_result};
    }
    return BuildHostQueryResult(query_result, iso_year, db_session->GetConnectionHandle());
  } catch (const std::exception& error) {
    if (IsMissingBillsTableError(error.what())) {
      QueryExecutionResult query_result;
      query_result.query_type = "year";
      query_result.query_value = std::string(iso_year);
      return HostQueryResult{.execution = query_result};
    }
    return std::unexpected(MakeError(error.what(), kContext));
  }
}

auto QueryMonthReport(const std::filesystem::path& db_path,
                      std::string_view iso_month) -> Result<HostQueryResult> {
  try {
    auto db_session = bills::io::CreateReportDbSession(db_path.string());
    auto report_data_gateway =
        bills::io::CreateReportDataGateway(db_session->GetConnectionHandle());
    const auto query_result = QueryService::QueryMonth(*report_data_gateway, iso_month);
    if (!query_result.data_found) {
      return HostQueryResult{.execution = query_result};
    }
    return BuildHostQueryResult(query_result, iso_month, db_session->GetConnectionHandle());
  } catch (const std::exception& error) {
    if (IsMissingBillsTableError(error.what())) {
      QueryExecutionResult query_result;
      query_result.query_type = "month";
      query_result.query_value = std::string(iso_month);
      return HostQueryResult{.execution = query_result};
    }
    return std::unexpected(MakeError(error.what(), kContext));
  }
}

auto ListAvailableMonths(const std::filesystem::path& db_path)
    -> Result<std::vector<std::string>> {
  if (!std::filesystem::exists(db_path)) {
    return std::vector<std::string>{};
  }
  try {
    auto db_session = bills::io::CreateReportDbSession(db_path.string());
    auto report_data_gateway =
        bills::io::CreateReportDataGateway(db_session->GetConnectionHandle());
    return report_data_gateway->ListAvailableMonths();
  } catch (const std::exception& error) {
    if (IsMissingBillsTableError(error.what())) {
      return std::vector<std::string>{};
    }
    return std::unexpected(MakeError(error.what(), kContext));
  }
}

auto RenderQueryReport(const HostQueryResult& query_result, std::string_view format_name)
    -> Result<std::string> {
  const std::string normalized_format =
      StandardReportRendererRegistry::NormalizeFormat(format_name);
  if (normalized_format.empty()) {
    return std::unexpected(
        MakeError("Unsupported report format: " + std::string(format_name), kContext));
  }
  if (!query_result.execution.data_found) {
    return std::unexpected(MakeError("No report data found.", kContext));
  }
  return ReportRenderService::Render(query_result.standard_report, normalized_format);
}

auto NormalizeReportExportYear(std::string_view raw) -> Result<ReportExportYear> {
  const auto normalized = TryBuildReportExportYear(raw);
  if (!normalized.has_value()) {
    return std::unexpected(
        MakeError("Report export year requires YYYY.", kContext));
  }
  return *normalized;
}

auto NormalizeReportExportMonth(std::string_view raw) -> Result<ReportExportMonth> {
  const auto normalized = TryBuildReportExportMonth(raw);
  if (!normalized.has_value()) {
    return std::unexpected(
        MakeError("Report export month requires YYYY-MM.", kContext));
  }
  return *normalized;
}

auto NormalizeReportExportRange(std::string_view start_period,
                                std::string_view end_period)
    -> Result<ReportExportRange> {
  const auto start = NormalizeReportExportMonth(start_period);
  if (!start) {
    return std::unexpected(start.error());
  }
  const auto end = NormalizeReportExportMonth(end_period);
  if (!end) {
    return std::unexpected(end.error());
  }
  const ReportExportRange range{
      .start = *start,
      .end = *end,
  };
  if (!IsReportExportRangeInOrder(range)) {
    return std::unexpected(MakeError(
        "Report export range requires start <= end.", kContext));
  }
  return range;
}

auto ExportReports(const HostReportExportRequest& request)
    -> Result<HostReportExportResult> {
  auto db_session = bills::io::CreateReportDbSession(request.db_path.string());
  auto report_data_gateway =
      bills::io::CreateReportDataGateway(db_session->GetConnectionHandle());
  ReportExportService export_service(std::move(report_data_gateway),
                                     request.export_dir.string(),
                                     request.format_folder_names);

  std::optional<ReportExportYear> normalized_year;
  std::optional<ReportExportMonth> normalized_month;
  std::optional<ReportExportRange> normalized_range;
  switch (request.scope) {
    case HostReportExportScope::kYear: {
      const auto year = NormalizeReportExportYear(request.primary_value);
      if (!year) {
        return std::unexpected(year.error());
      }
      normalized_year = *year;
      break;
    }
    case HostReportExportScope::kMonth: {
      const auto month = NormalizeReportExportMonth(request.primary_value);
      if (!month) {
        return std::unexpected(month.error());
      }
      normalized_month = *month;
      break;
    }
    case HostReportExportScope::kRange: {
      const auto range = NormalizeReportExportRange(request.primary_value,
                                                    request.secondary_value);
      if (!range) {
        return std::unexpected(range.error());
      }
      normalized_range = *range;
      break;
    }
    case HostReportExportScope::kAllMonths:
    case HostReportExportScope::kAllYears:
    case HostReportExportScope::kAll:
      break;
  }

  HostReportExportResult result;
  result.attempted_formats = request.formats;
  result.export_dir = request.export_dir;
  for (const auto& format : request.formats) {
    ReportExportRunResult current_result;
    switch (request.scope) {
      case HostReportExportScope::kYear:
        current_result = export_service.export_yearly_report(*normalized_year, format);
        break;
      case HostReportExportScope::kMonth:
        current_result =
            export_service.export_monthly_report(*normalized_month, format);
        break;
      case HostReportExportScope::kRange:
        current_result =
            export_service.export_monthly_range(*normalized_range, format);
        break;
      case HostReportExportScope::kAllMonths:
        current_result = export_service.export_all_monthly_reports(format);
        break;
      case HostReportExportScope::kAllYears:
        current_result = export_service.export_all_yearly_reports(format);
        break;
      case HostReportExportScope::kAll:
        current_result = export_service.export_all_reports(format);
        break;
    }
    result.exported_count += current_result.exported_count;
    if (!current_result.ok) {
      result.failed_formats.push_back(format);
    }
  }
  result.ok = result.failed_formats.empty();
  return result;
}

auto ExportParseBundle(const std::filesystem::path& records_root,
                       const std::filesystem::path& config_dir,
                       const std::filesystem::path& output_zip)
    -> Result<ParseBundleExportResult> {
  if (!std::filesystem::exists(records_root) ||
      !std::filesystem::is_directory(records_root)) {
    return std::unexpected(MakeError(
        "Export bundle records path must be an existing directory: " +
            records_root.string(),
        kContext));
  }

  const auto config_context = LoadValidatedConfigTextContext(config_dir);
  if (!config_context) {
    return std::unexpected(config_context.error());
  }

  const auto record_documents =
      SourceDocumentIo::LoadByExtensionRelative(records_root, ".txt");
  if (!record_documents) {
    return std::unexpected(record_documents.error());
  }
  const auto validation_result = ValidateRecordDocuments(
      *record_documents, config_context->validated.runtime_config,
      "TXT validation failed for parse bundle");
  if (!validation_result) {
    return std::unexpected(validation_result.error());
  }

  std::vector<ZipArchiveTextEntry> archive_entries;
  archive_entries.reserve(1U + kConfigFileNames.size() + record_documents->size());
  archive_entries.push_back(ZipArchiveTextEntry{
      .archive_path = std::string(kManifestPath),
      .text = BuildManifestText(record_documents->size()),
  });
  archive_entries.push_back(ZipArchiveTextEntry{
      .archive_path = std::string(kConfigPrefix) + std::string(kConfigFileNames[0]),
      .text = config_context->texts.validator_text,
  });
  archive_entries.push_back(ZipArchiveTextEntry{
      .archive_path = std::string(kConfigPrefix) + std::string(kConfigFileNames[1]),
      .text = config_context->texts.modifier_text,
  });
  archive_entries.push_back(ZipArchiveTextEntry{
      .archive_path = std::string(kConfigPrefix) + std::string(kConfigFileNames[2]),
      .text = config_context->texts.export_formats_text,
  });
  for (const auto& document : *record_documents) {
    archive_entries.push_back(ZipArchiveTextEntry{
        .archive_path = std::string(kRecordsPrefix) +
                        std::filesystem::path(document.display_path).generic_string(),
        .text = document.text,
    });
  }

  const auto write_result = WriteArchiveAtomically(output_zip, archive_entries);
  if (!write_result) {
    return std::unexpected(write_result.error());
  }

  return ParseBundleExportResult{
      .exported_record_files = record_documents->size(),
      .exported_config_files = kConfigFileNames.size(),
  };
}

auto ImportParseBundle(const std::filesystem::path& bundle_zip,
                       const std::filesystem::path& config_dir,
                       const std::filesystem::path& records_root,
                       std::optional<std::filesystem::path> db_path)
    -> Result<ParseBundleImportResult> {
  ParseBundleImportResult result;
  result.message = "Parse bundle import finished.";

  const auto archive_contents = LoadBundleArchiveContents(bundle_zip);
  if (!archive_contents) {
    return MakeImportParseBundleFailure("load_bundle",
                                        FormatError(archive_contents.error()));
  }

  const ConfigDocumentBundle imported_documents = ConfigDocumentParser::ParseTexts(
      archive_contents->config_texts.validator_text,
      archive_contents->config_texts.modifier_text,
      archive_contents->config_texts.export_formats_text,
      std::string(kConfigPrefix) + std::string(kConfigFileNames[0]),
      std::string(kConfigPrefix) + std::string(kConfigFileNames[1]),
      std::string(kConfigPrefix) + std::string(kConfigFileNames[2]));
  const auto imported_config_context = ConfigBundleService::Validate(imported_documents);
  if (!imported_config_context) {
    result = MakeImportParseBundleFailure(
        "validate_config", MakeConfigValidationError(imported_config_context.error()).message_);
    result.config_validation = imported_config_context.error();
    return result;
  }
  result.config_validation = imported_config_context->report;

  result.record_validation = BillWorkflowService::Validate(
      archive_contents->records, imported_config_context->runtime_config);
  if (result.record_validation.failure > 0U) {
    const auto validation_result = result.record_validation;
    result = MakeImportParseBundleFailure(
        "validate_records",
        BuildValidationError(validation_result,
                             "TXT validation failed for parse bundle")
            .message_);
    result.config_validation = imported_config_context->report;
    result.record_validation = validation_result;
    return result;
  }

  const SourceDocumentBatch config_documents =
      BuildConfigDocumentsForWrite(archive_contents->config_texts);
  const auto config_snapshots = CaptureSnapshots(config_dir, config_documents);
  if (!config_snapshots) {
    return std::unexpected(config_snapshots.error());
  }
  const auto record_snapshots = CaptureSnapshots(records_root, archive_contents->records);
  if (!record_snapshots) {
    return std::unexpected(record_snapshots.error());
  }

  for (const auto& document : config_documents) {
    const auto write_result = SourceDocumentIo::WriteText(
        config_dir / std::filesystem::path(document.display_path), document.text);
    if (!write_result) {
      const auto validated_records = result.record_validation;
      const auto rollback_result = RestoreSnapshots(config_dir, *config_snapshots);
      const auto failure = ComposeImportError(
          "Failed to apply imported config files", write_result.error(),
          rollback_result ? std::nullopt
                          : std::optional<Error>(rollback_result.error()));
      result = MakeImportParseBundleFailure(
          "write_config",
          FormatError(failure));
      result.config_validation = imported_config_context->report;
      result.record_validation = validated_records;
      return result;
    }
  }

  for (const auto& document : archive_contents->records) {
    const auto write_result = SourceDocumentIo::WriteText(
        records_root / std::filesystem::path(document.display_path), document.text);
    if (!write_result) {
      const auto validated_records = result.record_validation;
      const auto records_rollback = RestoreSnapshots(records_root, *record_snapshots);
      const auto config_rollback = RestoreSnapshots(config_dir, *config_snapshots);
      std::optional<Error> rollback_error;
      if (!records_rollback) {
        rollback_error = records_rollback.error();
      } else if (!config_rollback) {
        rollback_error = config_rollback.error();
      }
      const auto failure = ComposeImportError(
          "Failed to apply imported TXT files", write_result.error(),
          rollback_error);
      result = MakeImportParseBundleFailure(
          "write_records",
          FormatError(failure));
      result.config_validation = imported_config_context->report;
      result.record_validation = validated_records;
      return result;
    }
  }

  result.imported_record_files = archive_contents->records.size();
  result.imported_config_files = kConfigFileNames.size();

  if (db_path.has_value()) {
    const auto db_import_result =
        IngestDocuments(records_root, config_dir, *db_path, false);
    if (!db_import_result) {
      result.ok = false;
      result.failed_phase = "ingest_database";
      result.message = FormatError(db_import_result.error());
      return result;
    }
    result.db_ingest = *db_import_result;
    result.imported_bills = db_import_result->success;
    if (db_import_result->failure > 0U) {
      result.ok = false;
      result.failed_phase = "ingest_database";
      result.message = BuildValidationError(
                           *db_import_result,
                           "Database ingest failed after parse bundle import")
                           .message_;
      return result;
    }
    result.message = "Parse bundle import and SQLite ingest finished successfully.";
  }

  return result;
}

auto ExportBackupBundle(const std::filesystem::path& records_root,
                        const std::filesystem::path& config_dir,
                        const std::filesystem::path& output_zip)
    -> Result<BackupBundleExportResult> {
  if (!std::filesystem::exists(records_root) ||
      !std::filesystem::is_directory(records_root)) {
    return std::unexpected(MakeError(
        "Export bundle records path must be an existing directory: " +
            records_root.string(),
        kContext));
  }

  const auto config_context = LoadValidatedConfigTextContext(config_dir);
  if (!config_context) {
    return std::unexpected(config_context.error());
  }

  const auto record_documents =
      SourceDocumentIo::LoadByExtensionRelative(records_root, ".txt");
  if (!record_documents) {
    return std::unexpected(record_documents.error());
  }
  const auto validation_result = ValidateRecordDocuments(
      *record_documents, config_context->validated.runtime_config,
      "TXT validation failed for backup bundle");
  if (!validation_result) {
    return std::unexpected(validation_result.error());
  }

  std::vector<ZipArchiveTextEntry> archive_entries;
  archive_entries.reserve(1U + kBackupConfigFileNames.size() +
                          record_documents->size());
  archive_entries.push_back(ZipArchiveTextEntry{
      .archive_path = std::string(kManifestPath),
      .text = BuildBackupManifestText(record_documents->size()),
  });
  archive_entries.push_back(ZipArchiveTextEntry{
      .archive_path =
          std::string(kConfigPrefix) + std::string(kBackupConfigFileNames[0]),
      .text = config_context->texts.validator_text,
  });
  archive_entries.push_back(ZipArchiveTextEntry{
      .archive_path =
          std::string(kConfigPrefix) + std::string(kBackupConfigFileNames[1]),
      .text = config_context->texts.modifier_text,
  });
  for (const auto& document : *record_documents) {
    archive_entries.push_back(ZipArchiveTextEntry{
        .archive_path = std::string(kRecordsPrefix) +
                        std::filesystem::path(document.display_path).generic_string(),
        .text = document.text,
    });
  }

  const auto write_result = WriteArchiveAtomically(output_zip, archive_entries);
  if (!write_result) {
    return std::unexpected(write_result.error());
  }

  return BackupBundleExportResult{
      .exported_record_files = record_documents->size(),
      .exported_config_files = kBackupConfigFileNames.size(),
  };
}

auto ImportBackupBundle(const std::filesystem::path& bundle_zip,
                        const std::filesystem::path& config_dir,
                        const std::filesystem::path& records_root,
                        std::optional<std::filesystem::path> db_path)
    -> Result<BackupBundleImportResult> {
  BackupBundleImportResult result;
  result.message = "Backup bundle restore finished.";

  const auto archive_contents = LoadBackupArchiveContents(bundle_zip);
  if (!archive_contents) {
    return MakeImportBackupBundleFailure("load_bundle",
                                         FormatError(archive_contents.error()));
  }

  const auto current_config_texts = ReadConfigTextsFromDirectory(config_dir);
  if (!current_config_texts) {
    return MakeImportBackupBundleFailure(
        "load_current_config", FormatError(current_config_texts.error()));
  }

  ConfigTexts merged_config_texts = *current_config_texts;
  merged_config_texts.validator_text = archive_contents->validator_text;
  merged_config_texts.modifier_text = archive_contents->modifier_text;
  const auto imported_config_context = ParseAndValidateConfigTexts(
      std::move(merged_config_texts),
      std::string(kConfigPrefix) + std::string(kBackupConfigFileNames[0]),
      std::string(kConfigPrefix) + std::string(kBackupConfigFileNames[1]),
      (config_dir / kConfigFileNames[2]).string());
  if (!imported_config_context) {
    result = MakeImportBackupBundleFailure(
        "validate_config", FormatError(imported_config_context.error()));
    return result;
  }
  result.config_validation = imported_config_context->validated.report;

  result.record_validation = BillWorkflowService::Validate(
      archive_contents->records, imported_config_context->validated.runtime_config);
  if (result.record_validation.failure > 0U) {
    const auto validation_result = result.record_validation;
    result = MakeImportBackupBundleFailure(
        "validate_records",
        BuildValidationError(validation_result,
                             "TXT validation failed for backup bundle")
            .message_);
    result.config_validation = imported_config_context->validated.report;
    result.record_validation = validation_result;
    return result;
  }

  const SourceDocumentBatch config_documents = BuildBackupConfigDocumentsForWrite(
      archive_contents->validator_text, archive_contents->modifier_text);
  const auto existing_record_documents = LoadWorkspaceRecordDocuments(records_root);
  if (!existing_record_documents) {
    return std::unexpected(existing_record_documents.error());
  }
  const auto record_snapshot_targets =
      MergeDocumentsByDisplayPath(*existing_record_documents, archive_contents->records);
  const auto config_snapshots = CaptureSnapshots(config_dir, config_documents);
  if (!config_snapshots) {
    return std::unexpected(config_snapshots.error());
  }
  const auto record_snapshots = CaptureSnapshots(records_root, record_snapshot_targets);
  if (!record_snapshots) {
    return std::unexpected(record_snapshots.error());
  }

  for (const auto& document : config_documents) {
    const auto write_result = SourceDocumentIo::WriteText(
        config_dir / std::filesystem::path(document.display_path), document.text);
    if (!write_result) {
      const auto rollback_result = RestoreSnapshots(config_dir, *config_snapshots);
      const auto failure = ComposeImportError(
          "Failed to apply restored config files", write_result.error(),
          rollback_result ? std::nullopt
                          : std::optional<Error>(rollback_result.error()));
      result = MakeImportBackupBundleFailure("write_config", FormatError(failure));
      result.config_validation = imported_config_context->validated.report;
      result.record_validation = result.record_validation;
      return result;
    }
  }

  const auto remove_existing_records = RemoveTxtFilesRecursively(records_root);
  if (!remove_existing_records) {
    const auto records_rollback = RestoreSnapshots(records_root, *record_snapshots);
    const auto config_rollback = RestoreSnapshots(config_dir, *config_snapshots);
    std::optional<Error> rollback_error;
    if (!records_rollback) {
      rollback_error = records_rollback.error();
    } else if (!config_rollback) {
      rollback_error = config_rollback.error();
    }
    const auto failure = ComposeImportError(
        "Failed to clear existing TXT files before restore",
        remove_existing_records.error(),
        rollback_error);
    result = MakeImportBackupBundleFailure("clear_records", FormatError(failure));
    result.config_validation = imported_config_context->validated.report;
    return result;
  }

  for (const auto& document : archive_contents->records) {
    const auto write_result = SourceDocumentIo::WriteText(
        records_root / std::filesystem::path(document.display_path), document.text);
    if (!write_result) {
      const auto records_rollback = RestoreSnapshots(records_root, *record_snapshots);
      const auto config_rollback = RestoreSnapshots(config_dir, *config_snapshots);
      std::optional<Error> rollback_error;
      if (!records_rollback) {
        rollback_error = records_rollback.error();
      } else if (!config_rollback) {
        rollback_error = config_rollback.error();
      }
      const auto failure = ComposeImportError(
          "Failed to apply restored TXT files", write_result.error(),
          rollback_error);
      result = MakeImportBackupBundleFailure("write_records", FormatError(failure));
      result.config_validation = imported_config_context->validated.report;
      return result;
    }
  }

  result.restored_record_files = archive_contents->records.size();
  result.restored_config_files = kBackupConfigFileNames.size();

  if (db_path.has_value()) {
    const std::filesystem::path staged_db_path =
        db_path->parent_path() / (db_path->filename().string() + ".backup_restore");
    RemoveDatabaseFamily(staged_db_path);

    const auto staged_db_import_result = IngestDocuments(records_root, config_dir,
                                                         staged_db_path, false);
    if (!staged_db_import_result) {
      const auto records_rollback = RestoreSnapshots(records_root, *record_snapshots);
      const auto config_rollback = RestoreSnapshots(config_dir, *config_snapshots);
      std::optional<Error> rollback_error;
      if (!records_rollback) {
        rollback_error = records_rollback.error();
      } else if (!config_rollback) {
        rollback_error = config_rollback.error();
      }
      const auto failure = ComposeImportError(
          "Failed to rebuild SQLite from restored TXT files",
          staged_db_import_result.error(), rollback_error);
      RemoveDatabaseFamily(staged_db_path);
      result = MakeImportBackupBundleFailure("rebuild_database", FormatError(failure));
      result.config_validation = imported_config_context->validated.report;
      return result;
    }
    result.db_ingest = *staged_db_import_result;
    result.restored_bills = staged_db_import_result->success;
    if (staged_db_import_result->failure > 0U) {
      const auto records_rollback = RestoreSnapshots(records_root, *record_snapshots);
      const auto config_rollback = RestoreSnapshots(config_dir, *config_snapshots);
      std::optional<Error> rollback_error;
      if (!records_rollback) {
        rollback_error = records_rollback.error();
      } else if (!config_rollback) {
        rollback_error = config_rollback.error();
      }
      const auto failure = ComposeImportError(
          "Database rebuild failed after backup restore",
          BuildValidationError(*staged_db_import_result,
                               "Database rebuild failed after backup restore"),
          rollback_error);
      RemoveDatabaseFamily(staged_db_path);
      result = MakeImportBackupBundleFailure("rebuild_database", FormatError(failure));
      result.config_validation = imported_config_context->validated.report;
      result.record_validation = result.record_validation;
      result.db_ingest = *staged_db_import_result;
      return result;
    }

    const auto promote_result = PromoteStagedDatabaseFamily(staged_db_path, *db_path);
    if (!promote_result) {
      const auto records_rollback = RestoreSnapshots(records_root, *record_snapshots);
      const auto config_rollback = RestoreSnapshots(config_dir, *config_snapshots);
      std::optional<Error> rollback_error;
      if (!records_rollback) {
        rollback_error = records_rollback.error();
      } else if (!config_rollback) {
        rollback_error = config_rollback.error();
      }
      const auto failure = ComposeImportError(
          "Failed to promote rebuilt SQLite into place", promote_result.error(),
          rollback_error);
      RemoveDatabaseFamily(staged_db_path);
      result = MakeImportBackupBundleFailure("promote_database", FormatError(failure));
      result.config_validation = imported_config_context->validated.report;
      result.db_ingest = *staged_db_import_result;
      return result;
    }
    result.message = "Backup bundle restore and SQLite rebuild finished successfully.";
  }

  return result;
}

auto WriteTemplateFiles(const std::filesystem::path& output_dir,
                        const TemplateGenerationResult& result)
    -> Result<std::vector<std::string>> {
  SourceDocumentBatch documents;
  documents.reserve(result.templates.size());
  for (const auto& generated_template : result.templates) {
    documents.push_back(SourceDocument{
        .display_path = generated_template.relative_path,
        .text = generated_template.text,
    });
  }
  return SourceDocumentIo::WriteDocuments(output_dir, documents);
}

auto WriteSerializedJsonOutputs(
    const std::vector<BillWorkflowFileResult>& files,
    const YearPartitionOutputPathBuilder& output_path_builder)
    -> Result<std::vector<std::string>> {
  SourceDocumentBatch documents;
  for (const auto& file : files) {
    if (!file.ok || file.serialized_json.empty()) {
      continue;
    }
    documents.push_back(SourceDocument{
        .display_path = output_path_builder
                            .build_output_path(std::filesystem::path(file.display_path))
                            .string(),
        .text = file.serialized_json,
    });
  }
  if (documents.empty()) {
    return std::vector<std::string>{};
  }
  return SourceDocumentIo::WriteDocuments({}, documents);
}

}  // namespace bills::io
