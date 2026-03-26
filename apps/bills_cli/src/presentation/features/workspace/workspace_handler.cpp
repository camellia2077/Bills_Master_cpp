#if defined(BILLS_CLI_MODULES_ENABLED)
import bill.cli.presentation.features.workspace_handler;
import bill.cli.presentation.entry.runtime_context;
import bill.cli.presentation.parsing.cli_request;
import bill.cli.deps.io_year_partition_output_path_builder;
import bill.cli.deps.io_host_flow_support;
import bill.cli.deps.common_utils;
#else
#include <presentation/features/workspace/workspace_handler.hpp>
#endif

#include <pch.hpp>
#include <common/Result.hpp>

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

namespace terminal = bills::cli::terminal;

namespace bills::cli {
namespace {

auto PrintBatchSummary(const char* title,
                       const BillWorkflowBatchResult& result) -> void {
  std::cout << title << ": processed=" << result.processed
            << ", success=" << result.success
            << ", failure=" << result.failure << '\n';
  for (const auto& file : result.files) {
    if (file.ok) {
      continue;
    }
    std::cerr << "  [FAIL] " << file.display_path;
    if (!file.stage_group.empty()) {
      std::cerr << " | class=" << file.stage_group;
    }
    if (!file.stage.empty()) {
      std::cerr << " | stage=" << file.stage;
    }
    if (!file.error.empty()) {
      std::cerr << " | " << file.error;
    }
    std::cerr << '\n';
  }
}

auto WriteJsonCache(const RuntimeContext& context,
                    const std::vector<BillWorkflowFileResult>& files) -> bool {
  const YearPartitionOutputPathBuilder output_path_builder(
      context.json_cache_dir.string());
  const auto write_result =
      bills::io::WriteSerializedJsonOutputs(files, output_path_builder);
  if (!write_result) {
    std::cerr << terminal::kRed << "Error: " << terminal::kReset
              << FormatError(write_result.error()) << '\n';
    return false;
  }
  if (!write_result->empty()) {
    std::cout << "Wrote " << write_result->size()
              << " JSON cache file(s) to " << context.json_cache_dir.string()
              << '\n';
  }
  return true;
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

auto BuildDefaultBundlePath(const RuntimeContext& context) -> std::filesystem::path {
  const auto now =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  const std::tm tm_value = ToTm(now);
  std::ostringstream stream;
  stream << "parse_bundle_" << std::put_time(&tm_value, "%Y%m%d_%H%M%S")
         << ".zip";
  return context.export_dir / stream.str();
}

auto ResolveCliPath(const std::filesystem::path& path) -> std::filesystem::path {
  if (path.empty()) {
    return path;
  }
  if (path.is_absolute()) {
    return path.lexically_normal();
  }
  return std::filesystem::absolute(path).lexically_normal();
}

}  // namespace

WorkspaceHandler::WorkspaceHandler(const RuntimeContext& context)
    : context_(context) {}

auto WorkspaceHandler::Handle(const WorkspaceRequest& request) const -> bool {
  if (request.action == WorkspaceAction::kExportBundle) {
    const std::filesystem::path records_root = ResolveCliPath(request.input_path);
    const std::filesystem::path output_zip =
        request.output_path.has_value()
            ? ResolveCliPath(*request.output_path)
            : BuildDefaultBundlePath(context_);
    const auto export_result =
        bills::io::ExportParseBundle(records_root, context_.config_dir, output_zip);
    if (!export_result) {
      std::cerr << terminal::kRed << "Error: " << terminal::kReset
                << FormatError(export_result.error()) << '\n';
      return false;
    }

    std::cout << "Exported parse bundle to " << output_zip.string() << '\n'
              << "Included " << export_result->exported_record_files
              << " TXT record file(s) and "
              << export_result->exported_config_files
              << " TOML config file(s)\n";
    return true;
  }

  if (request.action == WorkspaceAction::kImportBundle) {
    if (!request.target_path.has_value()) {
      std::cerr << terminal::kRed << "Error: " << terminal::kReset
                << "WorkspaceHandler: import-bundle requires a target records directory."
                << '\n';
      return false;
    }
    const std::filesystem::path bundle_zip = ResolveCliPath(request.input_path);
    const std::filesystem::path records_root = ResolveCliPath(*request.target_path);
    const auto import_result =
        bills::io::ImportParseBundle(bundle_zip, context_.config_dir, records_root);
    if (!import_result) {
      std::cerr << terminal::kRed << "Error: " << terminal::kReset
                << FormatError(import_result.error()) << '\n';
      return false;
    }
    if (!import_result->ok) {
      std::cerr << terminal::kRed << "Error: " << terminal::kReset
                << import_result->message;
      if (!import_result->failed_phase.empty()) {
        std::cerr << " | phase=" << import_result->failed_phase;
      }
      std::cerr << '\n';
      return false;
    }

    std::cout << "Imported parse bundle from " << bundle_zip.string() << '\n'
              << "Applied " << import_result->imported_config_files
              << " TOML config file(s) to " << context_.config_dir.string() << '\n'
              << "Extracted " << import_result->imported_record_files
              << " TXT record file(s) to " << records_root.string() << '\n';
    return true;
  }

  switch (request.action) {
    case WorkspaceAction::kValidate: {
      const auto result =
          bills::io::ValidateDocuments(request.input_path, context_.config_dir);
      if (!result) {
        std::cerr << terminal::kRed << "Error: " << terminal::kReset
                  << FormatError(result.error()) << '\n';
        return false;
      }
      PrintBatchSummary("Validation", *result);
      return result->failure == 0U;
    }
    case WorkspaceAction::kConvert: {
      const auto result = bills::io::ConvertDocuments(
          request.input_path, context_.config_dir, request.write_json_cache);
      if (!result) {
        std::cerr << terminal::kRed << "Error: " << terminal::kReset
                  << FormatError(result.error()) << '\n';
        return false;
      }
      PrintBatchSummary("Convert", *result);
      if (request.write_json_cache && !WriteJsonCache(context_, result->files)) {
        return false;
      }
      return result->failure == 0U;
    }
    case WorkspaceAction::kIngest: {
      const auto db_path = ResolveDbPath(context_, request.db_path);
      std::cout << "Database: " << db_path.string() << '\n';
      const auto result = bills::io::IngestDocuments(
          request.input_path, context_.config_dir, db_path, request.write_json_cache);
      if (!result) {
        std::cerr << terminal::kRed << "Error: " << terminal::kReset
                  << FormatError(result.error()) << '\n';
        return false;
      }
      PrintBatchSummary("Ingest", *result);
      if (request.write_json_cache && !WriteJsonCache(context_, result->files)) {
        return false;
      }
      return result->failure == 0U;
    }
    case WorkspaceAction::kImportJson: {
      const auto db_path = ResolveDbPath(context_, request.db_path);
      std::cout << "Database: " << db_path.string() << '\n';
      const auto result = bills::io::ImportJsonDocuments(request.input_path, db_path);
      if (!result) {
        std::cerr << terminal::kRed << "Error: " << terminal::kReset
                  << FormatError(result.error()) << '\n';
        return false;
      }
      PrintBatchSummary("Import JSON", *result);
      return result->failure == 0U;
    }
    case WorkspaceAction::kExportBundle:
    case WorkspaceAction::kImportBundle:
      return false;
  }

  return false;
}

}  // namespace bills::cli
