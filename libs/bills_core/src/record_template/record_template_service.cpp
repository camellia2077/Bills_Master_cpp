#include "record_template/record_template_service.hpp"

#include <set>
#include <string_view>
#include <stdexcept>
#include <utility>

#include "billing/conversion/bills_processing_pipeline.hpp"
#include "common/iso_period.hpp"
#include "config_loading/runtime_config_loader.hpp"
#include "record_template/file_support.hpp"
#include "record_template/ordered_template_layout_loader.hpp"
#include "record_template/period_support.hpp"
#include "record_template/template_render_support.hpp"

namespace {
namespace fs = std::filesystem;

constexpr const char* kValidatorConfigName = "validator_config.toml";
constexpr const char* kModifierConfigName = "modifier_config.toml";

auto ResolveValidatorConfigPath(const TemplateGenerationRequest& request)
    -> RecordTemplateResult<fs::path> {
  if (!request.config_dir.empty()) {
    return request.config_dir / kValidatorConfigName;
  }
  if (!request.validator_config_path.empty()) {
    return request.validator_config_path;
  }
  return std::unexpected(MakeRecordTemplateError(
      RecordTemplateErrorCategory::kRequest,
      "Provide either config_dir or validator_config_path for template generation."));
}

auto LoadPipelineConfigs(const fs::path& validator_config_path,
                         const fs::path& modifier_config_path)
    -> RecordTemplateResult<RuntimeConfigBundle> {
  const auto config_bundle =
      RuntimeConfigLoader::LoadFromFiles(validator_config_path, modifier_config_path);
  if (!config_bundle) {
    return std::unexpected(MakeRecordTemplateErrorFromCommon(
        config_bundle.error(), RecordTemplateErrorCategory::kConfig));
  }
  return *config_bundle;
}

auto ParsePeriodFromFileName(const fs::path& file_path) -> std::string {
  const auto parsed =
      bills::core::common::iso_period::parse_year_month(file_path.stem().string());
  if (!parsed.has_value()) {
    return {};
  }
  return bills::core::common::iso_period::format_year_month(parsed->year,
                                                            parsed->month);
}

auto MapPipelineStage(std::string_view detailed_stage) -> std::string_view {
  if (detailed_stage == "validate_bill") {
    return "business";
  }
  return "parse";
}

auto BuildPipelineIssueCode(std::string_view detailed_stage) -> std::string {
  return "record." + std::string(MapPipelineStage(detailed_stage)) + "." +
         std::string(detailed_stage.empty() ? "validation_failed"
                                            : detailed_stage);
}

auto BuildPipelineIssues(const BillProcessingPipeline& pipeline,
                         const fs::path& file_path)
    -> std::vector<ValidationIssue> {
  const std::string stage =
      pipeline.last_failure_stage().empty() ? "validation_failed"
                                            : pipeline.last_failure_stage();
  const std::string_view mapped_stage = MapPipelineStage(stage);
  const auto& messages = pipeline.last_failure_messages();
  std::vector<ValidationIssue> issues;
  if (messages.empty()) {
    issues.push_back(MakeValidationIssue(
        "record_txt", std::string(mapped_stage), BuildPipelineIssueCode(stage),
        pipeline.last_failure_message().empty() ? "Validation failed."
                                                : pipeline.last_failure_message(),
        file_path));
    return issues;
  }

  issues.reserve(messages.size());
  for (const auto& message : messages) {
    issues.push_back(MakeValidationIssue(
        "record_txt", std::string(mapped_stage), BuildPipelineIssueCode(stage),
        message, file_path));
  }
  return issues;
}

}  // namespace

auto RecordTemplateService::LoadOrderedTemplateLayout(
    const fs::path& validator_config_path)
    -> RecordTemplateResult<OrderedTemplateLayout> {
  return OrderedTemplateLayoutLoader::LoadFromValidatorFile(validator_config_path);
}

auto RecordTemplateService::LoadOrderedTemplateLayoutFromConfigDir(
    const fs::path& config_dir) -> RecordTemplateResult<OrderedTemplateLayout> {
  return LoadOrderedTemplateLayout(config_dir / kValidatorConfigName);
}

auto RecordTemplateService::GenerateTemplates(
    const TemplateGenerationRequest& request)
    -> RecordTemplateResult<TemplateGenerationResult> {
  const auto validator_config_path = ResolveValidatorConfigPath(request);
  if (!validator_config_path) {
    return std::unexpected(validator_config_path.error());
  }

  const auto layout = LoadOrderedTemplateLayout(*validator_config_path);
  if (!layout) {
    return std::unexpected(layout.error());
  }

  const auto periods = RecordTemplatePeriodSupport::ExpandTemplatePeriods(request);
  if (!periods) {
    return std::unexpected(periods.error());
  }

  if (request.write_files && request.output_dir.empty()) {
    return std::unexpected(MakeRecordTemplateError(
        RecordTemplateErrorCategory::kRequest,
        "write_files=true requires non-empty output_dir."));
  }

  TemplateGenerationResult result;
  result.write_files = request.write_files;
  result.output_dir = request.output_dir;
  result.templates.reserve(periods->size());

  for (const auto& period : *periods) {
    GeneratedTemplateFile generated_file;
    generated_file.period = period;
    generated_file.relative_path =
        TemplateRenderSupport::BuildRelativeTemplatePath(period);
    generated_file.text = TemplateRenderSupport::RenderTemplate(period, *layout);

    if (request.write_files) {
      generated_file.output_path = request.output_dir / generated_file.relative_path;
      const auto write_result = RecordTemplateFileSupport::WriteTextFile(
          generated_file.output_path, generated_file.text);
      if (!write_result) {
        return std::unexpected(write_result.error());
      }
    }

    result.templates.push_back(std::move(generated_file));
  }

  return result;
}

auto RecordTemplateService::PreviewRecords(const fs::path& input_path,
                                           const fs::path& config_dir)
    -> RecordTemplateResult<RecordPreviewResult> {
  return ValidateRecordBatch(input_path, config_dir);
}

auto RecordTemplateService::ValidateRecordBatch(const fs::path& input_path,
                                                const fs::path& config_dir)
    -> RecordTemplateResult<RecordPreviewResult> {
  return ValidateRecordBatch(input_path, config_dir / kValidatorConfigName,
                             config_dir / kModifierConfigName);
}

auto RecordTemplateService::PreviewRecords(
    const fs::path& input_path, const fs::path& validator_config_path,
    const fs::path& modifier_config_path)
    -> RecordTemplateResult<RecordPreviewResult> {
  return ValidateRecordBatch(input_path, validator_config_path,
                             modifier_config_path);
}

auto RecordTemplateService::ValidateRecordBatch(
    const fs::path& input_path, const fs::path& validator_config_path,
    const fs::path& modifier_config_path)
    -> RecordTemplateResult<RecordPreviewResult> {
  const auto files =
      RecordTemplateFileSupport::ListFilesByExtension(input_path, ".txt");
  if (!files) {
    return std::unexpected(files.error());
  }

  const auto config_bundle =
      LoadPipelineConfigs(validator_config_path, modifier_config_path);
  if (!config_bundle) {
    return std::unexpected(config_bundle.error());
  }

  RecordPreviewResult preview_result;
  preview_result.input_path = input_path;
  preview_result.processed = files->size();
  preview_result.files.reserve(files->size());

  BillProcessingPipeline pipeline(config_bundle->validator_config,
                                  config_bundle->modifier_config);
  std::set<std::string> periods;

  for (const auto& file_path : *files) {
    RecordPreviewFile preview_file;
    preview_file.path = file_path;
    preview_file.file_name_period = ParsePeriodFromFileName(file_path);
    preview_file.file_name_matches_period = true;

    try {
      const auto content = RecordTemplateFileSupport::ReadTextFile(file_path);
      if (!content) {
        throw std::runtime_error(FormatRecordTemplateError(content.error()));
      }

      ParsedBill bill_data{};
      const bool ok = pipeline.validate_and_convert_content(
          *content, file_path.string(), bill_data);
      preview_file.ok = ok;

      if (!ok) {
        preview_file.issues = BuildPipelineIssues(pipeline, file_path);
        preview_file.error = pipeline.last_failure_stage().empty()
                                 ? "Preview failed."
                                 : pipeline.last_failure_stage() + ": " +
                                       pipeline.last_failure_message();
        ++preview_result.failure;
        preview_result.files.push_back(std::move(preview_file));
        continue;
      }

      preview_file.period = bill_data.date;
      preview_file.year = bill_data.year;
      preview_file.month = bill_data.month;
      preview_file.transaction_count = bill_data.transactions.size();
      preview_file.total_income = bill_data.total_income;
      preview_file.total_expense = bill_data.total_expense;
      preview_file.balance = bill_data.balance;
      ++preview_result.success;
      periods.insert(preview_file.period);
    } catch (const std::exception& error) {
      preview_file.ok = false;
      preview_file.error = error.what();
      preview_file.issues.push_back(MakeValidationIssue(
          "record_txt", "parse", "record.parse.read_failed", error.what(),
          file_path));
      ++preview_result.failure;
    }

    preview_result.files.push_back(std::move(preview_file));
  }

  preview_result.periods.assign(periods.begin(), periods.end());
  return preview_result;
}

auto RecordTemplateService::InspectConfig(const fs::path& config_dir)
    -> RecordTemplateResult<ConfigInspectResult> {
  return InspectValidatorFile(config_dir / kValidatorConfigName);
}

auto RecordTemplateService::InspectValidatorFile(
    const fs::path& validator_config_path)
    -> RecordTemplateResult<ConfigInspectResult> {
  const auto layout = LoadOrderedTemplateLayout(validator_config_path);
  if (!layout) {
    return std::unexpected(layout.error());
  }

  ConfigInspectResult result;
  result.categories = std::move(layout->categories);
  return result;
}

auto RecordTemplateService::ListPeriods(const fs::path& input_path)
    -> RecordTemplateResult<ListedPeriodsResult> {
  const auto files =
      RecordTemplateFileSupport::ListFilesByExtension(input_path, ".txt");
  if (!files) {
    return std::unexpected(files.error());
  }

  ListedPeriodsResult result;
  result.input_path = input_path;
  result.processed = files->size();

  std::set<std::string> periods;
  for (const auto& file_path : *files) {
    const auto period = RecordTemplatePeriodSupport::ExtractPeriodFromFile(file_path);
    if (!period) {
      result.invalid_files.push_back(
          {.path = file_path, .error = FormatRecordTemplateError(period.error())});
      continue;
    }
    periods.insert(*period);
  }

  result.valid = result.processed - result.invalid_files.size();
  result.invalid = result.invalid_files.size();
  result.periods.assign(periods.begin(), periods.end());
  return result;
}
