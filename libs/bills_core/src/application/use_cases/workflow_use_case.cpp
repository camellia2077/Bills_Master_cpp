// application/use_cases/workflow_use_case.cpp
#include "workflow_use_case.hpp"

#include <filesystem>
#include <string_view>
#include <vector>

#if BILLS_CORE_MODULES_ENABLED
import bill.core.billing.pipeline;
import bill.core.domain.bill_record;
import bill.core.ports.bills_content_reader;
import bill.core.ports.bills_file_enumerator;
import bill.core.ports.bills_repository;
import bill.core.ports.bills_serializer;
import bill.core.ports.output_path_builder;
using bills::core::modules::billing::BillProcessingPipeline;
using bills::core::modules::domain_bill_record::ParsedBill;
using bills::core::modules::ports::BillContentReader;
using bills::core::modules::ports::BillFileEnumerator;
using bills::core::modules::ports::BillRepository;
using bills::core::modules::ports::BillSerializer;
using bills::core::modules::ports::OutputPathBuilder;
#else
#include "billing/conversion/bills_processing_pipeline.hpp"
#include "ports/bills_content_reader.hpp"
#include "ports/bills_file_enumerator.hpp"
#include "ports/bills_repository.hpp"
#include "ports/bills_serializer.hpp"
#include "ports/output_path_builder.hpp"
#endif

namespace fs = std::filesystem;

namespace {
const std::string kTextExtension = ".txt";
const std::string kJsonExtension = ".json";
const std::string kValidationLabel = "Validation";
const std::string kConversionLabel = "Conversion";
const std::string kIngestLabel = "Ingest";
const std::string kImportLabel = "Database Import";
const std::string kWorkflowLabel = "Full Workflow";

auto normalize_failure_message(const std::string& message,
                               const std::string& fallback) -> std::string {
  return message.empty() ? fallback : message;
}

void record_failure(ProcessStats& stats, const fs::path& path,
                    std::string stage, std::string message) {
  stats.failure++;
  stats.add_failure(path.string(), std::move(stage), std::move(message));
}

void record_pipeline_failure(ProcessStats& stats, const fs::path& path,
                             const BillProcessingPipeline& pipeline,
                             const std::string& fallback_stage,
                             const std::string& fallback_message) {
  record_failure(
      stats, path,
      normalize_failure_message(pipeline.last_failure_stage(), fallback_stage),
      normalize_failure_message(pipeline.last_failure_message(),
                                fallback_message));
}
}  // namespace

WorkflowUseCase::WorkflowUseCase(BillConfig validator_config,
                                 Config modifier_config,
                                 BillContentReader& content_reader,
                                 BillFileEnumerator& file_enumerator,
                                 BillSerializer& serializer,
                                 OutputPathBuilder& output_path_builder)
    : validator_config_(std::move(validator_config)),
      modifier_config_(std::move(modifier_config)),
      content_reader_(content_reader),
      file_enumerator_(file_enumerator),
      serializer_(serializer),
      output_path_builder_(output_path_builder) {}

auto WorkflowUseCase::Validate(const std::string& path)
    -> Result<ProcessStats> {
  ProcessStats stats;
  try {
    BillProcessingPipeline bill_processing_pipeline(validator_config_,
                                                    modifier_config_);
    std::vector<fs::path> files = file_enumerator_.ListFilesByExtension(
        std::string_view{path}, std::string_view{kTextExtension});
    for (const auto& file : files) {
      try {
        std::string bill_content = content_reader_.Read(file.string());
        if (bill_processing_pipeline.validate_content(bill_content,
                                                      file.string())) {
          stats.success++;
        } else {
          record_pipeline_failure(stats, file, bill_processing_pipeline,
                                  "validate_content",
                                  "Bill validation failed.");
        }
      } catch (const std::exception& ex) {
        record_failure(stats, file, "read_input", ex.what());
      }
    }
  } catch (const std::exception& e) {
    return std::unexpected(MakeError(std::string(e.what()), kValidationLabel));
  }
  stats.print_summary(kValidationLabel);
  return stats;
}

auto WorkflowUseCase::Convert(const std::string& path) -> Result<ProcessStats> {
  ProcessStats stats;
  try {
    BillProcessingPipeline bill_processing_pipeline(validator_config_,
                                                    modifier_config_);
    std::vector<fs::path> files = file_enumerator_.ListFilesByExtension(
        std::string_view{path}, std::string_view{kTextExtension});
    for (const auto& file : files) {
      fs::path final_output_path = output_path_builder_.build_output_path(file);
      std::string bill_content;
      try {
        bill_content = content_reader_.Read(file.string());
      } catch (const std::exception& ex) {
        record_failure(stats, file, "read_input", ex.what());
        continue;
      }

      ParsedBill bill_data{};
      if (!bill_processing_pipeline.convert_content(bill_content, bill_data)) {
        record_pipeline_failure(stats, file, bill_processing_pipeline,
                                "convert_content",
                                "Bill conversion failed.");
        continue;
      }

      try {
        serializer_.WriteJson(bill_data, final_output_path.string());
        stats.success++;
      } catch (const std::exception& ex) {
        record_failure(stats, file, "write_json", ex.what());
      }
    }
  } catch (const std::exception& e) {
    return std::unexpected(MakeError(std::string(e.what()), kConversionLabel));
  }
  stats.print_summary(kConversionLabel);
  return stats;
}

auto WorkflowUseCase::Ingest(const std::string& path,
                             BillRepository& repository, bool write_json)
    -> Result<ProcessStats> {
  ProcessStats stats;
  try {
    BillProcessingPipeline bill_processing_pipeline(validator_config_,
                                                    modifier_config_);
    std::vector<fs::path> files = file_enumerator_.ListFilesByExtension(
        std::string_view{path}, std::string_view{kTextExtension});
    if (files.empty()) {
      stats.print_summary(kIngestLabel);
      return stats;
    }

    for (const auto& file : files) {
      ParsedBill bill_data{};
      try {
        std::string bill_content = content_reader_.Read(file.string());
        if (!bill_processing_pipeline.validate_and_convert_content(
                bill_content, file.string(), bill_data)) {
          record_pipeline_failure(stats, file, bill_processing_pipeline,
                                  "validate_and_convert",
                                  "Bill ingest validation failed.");
          continue;
        }
      } catch (const std::exception& ex) {
        record_failure(stats, file, "read_input", ex.what());
        continue;
      }

      if (write_json) {
        fs::path output_path = output_path_builder_.build_output_path(file);
        try {
          serializer_.WriteJson(bill_data, output_path.string());
        } catch (const std::exception& ex) {
          record_failure(stats, file, "write_json", ex.what());
          continue;
        }
      }

      try {
        repository.InsertBill(bill_data);
        stats.success++;
      } catch (const std::exception& ex) {
        record_failure(stats, file, "insert_repository", ex.what());
      }
    }
  } catch (const std::exception& e) {
    return std::unexpected(MakeError(std::string(e.what()), kIngestLabel));
  }
  stats.print_summary(kIngestLabel);
  return stats;
}

auto WorkflowUseCase::Import(const std::string& path,
                             BillRepository& repository)
    -> Result<ProcessStats> {
  ProcessStats stats;
  try {
    std::vector<fs::path> files = file_enumerator_.ListFilesByExtension(
        std::string_view{path}, std::string_view{kJsonExtension});
    for (const auto& file : files) {
      try {
        ParsedBill import_bill = serializer_.ReadJson(file.string());
        repository.InsertBill(import_bill);
        stats.success++;
      } catch (const std::exception& ex) {
        record_failure(stats, file, "import_json", ex.what());
      }
    }
  } catch (const std::exception& e) {
    return std::unexpected(MakeError(std::string(e.what()), kImportLabel));
  }
  stats.print_summary(kImportLabel);
  return stats;
}

auto WorkflowUseCase::FullWorkflow(const std::string& path,
                                   BillRepository& repository)
    -> Result<ProcessStats> {
  ProcessStats stats;
  try {
    BillProcessingPipeline bill_processing_pipeline(validator_config_,
                                                    modifier_config_);
    std::vector<fs::path> files = file_enumerator_.ListFilesByExtension(
        std::string_view{path}, std::string_view{kTextExtension});
    if (files.empty()) {
      stats.print_summary(kWorkflowLabel);
      return stats;
    }

    for (const auto& file_path : files) {
      std::string bill_content;
      try {
        bill_content = content_reader_.Read(file_path.string());
      } catch (const std::exception& ex) {
        record_failure(stats, file_path, "read_input", ex.what());
        continue;
      }

      if (!bill_processing_pipeline.validate_content(bill_content,
                                                     file_path.string())) {
        record_pipeline_failure(stats, file_path, bill_processing_pipeline,
                                "validate_content",
                                "Bill validation failed.");
        continue;
      }

      fs::path modified_path =
          output_path_builder_.build_output_path(file_path);

      ParsedBill bill_data{};
      if (!bill_processing_pipeline.convert_content(bill_content, bill_data)) {
        record_pipeline_failure(stats, file_path, bill_processing_pipeline,
                                "convert_content",
                                "Bill conversion failed.");
        continue;
      }
      try {
        serializer_.WriteJson(bill_data, modified_path.string());
      } catch (const std::exception& ex) {
        record_failure(stats, file_path, "write_json", ex.what());
        continue;
      }

      try {
        ParsedBill import_bill = serializer_.ReadJson(modified_path.string());
        repository.InsertBill(import_bill);
        stats.success++;
      } catch (const std::exception& ex) {
        record_failure(stats, file_path, "import_json", ex.what());
      }
    }
  } catch (const std::exception& e) {
    return std::unexpected(MakeError(std::string(e.what()), kWorkflowLabel));
  }
  stats.print_summary(kWorkflowLabel);
  return stats;
}
