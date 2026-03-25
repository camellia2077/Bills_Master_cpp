#include "record_template/record_template_service.hpp"

#include <set>
#include <string_view>
#include <utility>

#include "ingest/pipeline/bills_processing_pipeline.hpp"
#include "ingest/workflow_validation_issue_support.hpp"
#include "common/iso_period.hpp"
#include "common/text_normalizer.hpp"
#include "record_template/ordered_template_layout_loader.hpp"
#include "record_template/period_support.hpp"
#include "record_template/template_render_support.hpp"

namespace {

auto parse_period_from_display_path(const std::string& display_path) -> std::string {
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

}  // namespace

auto RecordTemplateService::BuildOrderedTemplateLayout(
    const ValidatorConfigDocument& validator_document)
    -> RecordTemplateResult<OrderedTemplateLayout> {
  return OrderedTemplateLayoutLoader::BuildFromDocument(validator_document);
}

auto RecordTemplateService::GenerateTemplates(
    const TemplateGenerationRequest& request)
    -> RecordTemplateResult<TemplateGenerationResult> {
  if (request.layout.categories.empty()) {
    return std::unexpected(MakeRecordTemplateError(
        RecordTemplateErrorCategory::kConfig,
        "Template generation requires a non-empty ordered template layout."));
  }

  const auto periods = RecordTemplatePeriodSupport::ExpandTemplatePeriods(request);
  if (!periods) {
    return std::unexpected(periods.error());
  }

  TemplateGenerationResult result;
  result.templates.reserve(periods->size());
  for (const auto& period : *periods) {
    GeneratedTemplateFile generated_file;
    generated_file.period = period;
    generated_file.relative_path =
        TemplateRenderSupport::BuildRelativeTemplatePath(period);
    generated_file.text = TemplateRenderSupport::RenderTemplate(
        period, request.layout);
    result.templates.push_back(std::move(generated_file));
  }
  return result;
}

auto RecordTemplateService::ValidateRecordBatch(
    const SourceDocumentBatch& documents, const RuntimeConfigBundle& config_bundle,
    std::string input_label) -> RecordTemplateResult<RecordPreviewResult> {
  RecordPreviewResult preview_result;
  preview_result.input_path = std::move(input_label);
  preview_result.processed = documents.size();
  preview_result.files.reserve(documents.size());

  BillProcessingPipeline pipeline(config_bundle.validator_config,
                                  config_bundle.modifier_config);
  std::set<std::string> periods;

  for (const auto& document : documents) {
    RecordPreviewFile preview_file;
    preview_file.path = document.display_path;
    preview_file.file_name_period = parse_period_from_display_path(document.display_path);
    preview_file.file_name_matches_period = true;

    try {
      ParsedBill bill_data{};
      const bool ok = pipeline.validate_and_convert_content(
          document.text, document.display_path, bill_data);
      preview_file.ok = ok;

      if (!ok) {
        preview_file.issues = bills::core::ingest::BuildWorkflowIssues(
            pipeline.last_failure_stage(), pipeline.last_failure_message(),
            pipeline.last_failure_messages(), document.display_path);
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
      preview_file.issues = bills::core::ingest::BuildWorkflowIssues(
          "unhandled_exception", error.what(), {}, document.display_path);
      ++preview_result.failure;
    }

    preview_result.files.push_back(std::move(preview_file));
  }

  preview_result.periods.assign(periods.begin(), periods.end());
  return preview_result;
}

auto RecordTemplateService::PreviewRecords(const SourceDocumentBatch& documents,
                                           const RuntimeConfigBundle& config_bundle,
                                           std::string input_label)
    -> RecordTemplateResult<RecordPreviewResult> {
  return ValidateRecordBatch(documents, config_bundle, std::move(input_label));
}

auto RecordTemplateService::InspectConfig(
    const ValidatorConfigDocument& validator_document)
    -> RecordTemplateResult<ConfigInspectResult> {
  const auto layout = BuildOrderedTemplateLayout(validator_document);
  if (!layout) {
    return std::unexpected(layout.error());
  }

  ConfigInspectResult result;
  result.categories = layout->categories;
  return result;
}

auto RecordTemplateService::ListPeriods(const SourceDocumentBatch& documents,
                                        std::string input_label)
    -> RecordTemplateResult<ListedPeriodsResult> {
  ListedPeriodsResult result;
  result.input_path = std::move(input_label);
  result.processed = documents.size();

  std::set<std::string> periods;
  for (const auto& document : documents) {
    const auto normalized_text = NormalizeBillText(document.text);
    if (!normalized_text) {
      result.invalid_files.push_back(
          {.path = document.display_path,
           .error = normalized_text.error().message_});
      continue;
    }

    const auto period =
        RecordTemplatePeriodSupport::ExtractPeriodFromNormalizedText(*normalized_text);
    if (!period) {
      result.invalid_files.push_back(
          {.path = document.display_path,
           .error = FormatRecordTemplateError(period.error())});
      continue;
    }
    periods.insert(*period);
  }

  result.valid = result.processed - result.invalid_files.size();
  result.invalid = result.invalid_files.size();
  result.periods.assign(periods.begin(), periods.end());
  return result;
}
