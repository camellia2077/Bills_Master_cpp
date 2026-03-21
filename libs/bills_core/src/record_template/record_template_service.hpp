#ifndef RECORD_TEMPLATE_RECORD_TEMPLATE_SERVICE_HPP_
#define RECORD_TEMPLATE_RECORD_TEMPLATE_SERVICE_HPP_

#include "common/source_document.hpp"
#include "config/config_bundle_service.hpp"
#include "config/config_document_types.hpp"
#include "record_template/record_template_types.hpp"

class RecordTemplateService {
 public:
  [[nodiscard]] static auto BuildOrderedTemplateLayout(
      const ValidatorConfigDocument& validator_document)
      -> RecordTemplateResult<OrderedTemplateLayout>;

  [[nodiscard]] static auto GenerateTemplates(
      const TemplateGenerationRequest& request)
      -> RecordTemplateResult<TemplateGenerationResult>;

  [[nodiscard]] static auto ValidateRecordBatch(
      const SourceDocumentBatch& documents,
      const RuntimeConfigBundle& config_bundle,
      std::string input_label = "inline_batch")
      -> RecordTemplateResult<RecordPreviewResult>;

  [[nodiscard]] static auto PreviewRecords(
      const SourceDocumentBatch& documents,
      const RuntimeConfigBundle& config_bundle,
      std::string input_label = "inline_batch")
      -> RecordTemplateResult<RecordPreviewResult>;

  [[nodiscard]] static auto InspectConfig(
      const ValidatorConfigDocument& validator_document)
      -> RecordTemplateResult<ConfigInspectResult>;

  [[nodiscard]] static auto ListPeriods(
      const SourceDocumentBatch& documents,
      std::string input_label = "inline_batch")
      -> RecordTemplateResult<ListedPeriodsResult>;
};

#endif  // RECORD_TEMPLATE_RECORD_TEMPLATE_SERVICE_HPP_
