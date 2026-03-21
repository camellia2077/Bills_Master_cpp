#include "presentation/features/workspace/workspace_handler.hpp"

#include <filesystem>
#include <iostream>
#include <string>

#include "bills_io/adapters/io/year_partition_output_path_builder.hpp"
#include "bills_io/host_flow_support.hpp"
#include "bills_io/io_factory.hpp"
#include "common/Result.hpp"
#include "common/common_utils.hpp"
#include "ingest/bill_workflow_service.hpp"

namespace terminal = common::terminal;

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
    if (!file.stage.empty()) {
      std::cerr << " | stage=" << file.stage;
    }
    if (!file.error.empty()) {
      std::cerr << " | " << file.error;
    }
    std::cerr << '\n';
  }
}

auto LoadRuntimeConfig(const RuntimeContext& context) -> Result<RuntimeConfigBundle> {
  const auto validated_context =
      bills::io::LoadValidatedConfigContext(context.config_dir);
  if (!validated_context) {
    return std::unexpected(validated_context.error());
  }
  return validated_context->validated.runtime_config;
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

}  // namespace

WorkspaceHandler::WorkspaceHandler(const RuntimeContext& context)
    : context_(context) {}

auto WorkspaceHandler::Handle(const WorkspaceRequest& request) const -> bool {
  const std::string extension =
      request.action == WorkspaceAction::kImportJson ? ".json" : ".txt";
  const auto documents =
      bills::io::LoadSourceDocuments(request.input_path, extension);
  if (!documents) {
    std::cerr << terminal::kRed << "Error: " << terminal::kReset
              << FormatError(documents.error()) << '\n';
    return false;
  }

  switch (request.action) {
    case WorkspaceAction::kValidate: {
      const auto runtime_config = LoadRuntimeConfig(context_);
      if (!runtime_config) {
        std::cerr << terminal::kRed << "Error: " << terminal::kReset
                  << FormatError(runtime_config.error()) << '\n';
        return false;
      }
      const auto result = BillWorkflowService::Validate(*documents, *runtime_config);
      PrintBatchSummary("Validation", result);
      return result.failure == 0U;
    }
    case WorkspaceAction::kConvert: {
      const auto runtime_config = LoadRuntimeConfig(context_);
      if (!runtime_config) {
        std::cerr << terminal::kRed << "Error: " << terminal::kReset
                  << FormatError(runtime_config.error()) << '\n';
        return false;
      }
      const auto result =
          BillWorkflowService::Convert(*documents, *runtime_config,
                                       request.write_json_cache);
      PrintBatchSummary("Convert", result);
      if (request.write_json_cache && !WriteJsonCache(context_, result.files)) {
        return false;
      }
      return result.failure == 0U;
    }
    case WorkspaceAction::kIngest: {
      const auto runtime_config = LoadRuntimeConfig(context_);
      if (!runtime_config) {
        std::cerr << terminal::kRed << "Error: " << terminal::kReset
                  << FormatError(runtime_config.error()) << '\n';
        return false;
      }
      const auto db_path = ResolveDbPath(context_, request.db_path);
      std::cout << "Database: " << db_path.string() << '\n';
      auto repository = bills::io::CreateBillRepository(db_path.string());
      const auto result = BillWorkflowService::Ingest(
          *documents, *runtime_config, *repository, request.write_json_cache);
      PrintBatchSummary("Ingest", result);
      if (request.write_json_cache && !WriteJsonCache(context_, result.files)) {
        return false;
      }
      return result.failure == 0U;
    }
    case WorkspaceAction::kImportJson: {
      const auto db_path = ResolveDbPath(context_, request.db_path);
      std::cout << "Database: " << db_path.string() << '\n';
      auto repository = bills::io::CreateBillRepository(db_path.string());
      const auto result = BillWorkflowService::ImportJson(*documents, *repository);
      PrintBatchSummary("Import JSON", result);
      return result.failure == 0U;
    }
  }

  return false;
}

}  // namespace bills::cli
