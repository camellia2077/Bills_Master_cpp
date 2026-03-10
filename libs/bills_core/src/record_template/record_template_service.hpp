#ifndef RECORD_TEMPLATE_RECORD_TEMPLATE_SERVICE_HPP_
#define RECORD_TEMPLATE_RECORD_TEMPLATE_SERVICE_HPP_

#include <filesystem>

#include "record_template/record_template_types.hpp"

class RecordTemplateService {
 public:
  [[nodiscard]] static auto LoadOrderedTemplateLayout(
      const std::filesystem::path& validator_config_path)
      -> RecordTemplateResult<OrderedTemplateLayout>;

  [[nodiscard]] static auto LoadOrderedTemplateLayoutFromConfigDir(
      const std::filesystem::path& config_dir)
      -> RecordTemplateResult<OrderedTemplateLayout>;

  [[nodiscard]] static auto GenerateTemplates(
      const TemplateGenerationRequest& request)
      -> RecordTemplateResult<TemplateGenerationResult>;

  [[nodiscard]] static auto PreviewRecords(
      const std::filesystem::path& input_path,
      const std::filesystem::path& config_dir)
      -> RecordTemplateResult<RecordPreviewResult>;

  [[nodiscard]] static auto PreviewRecords(
      const std::filesystem::path& input_path,
      const std::filesystem::path& validator_config_path,
      const std::filesystem::path& modifier_config_path)
      -> RecordTemplateResult<RecordPreviewResult>;

  [[nodiscard]] static auto InspectConfig(
      const std::filesystem::path& config_dir)
      -> RecordTemplateResult<ConfigInspectResult>;

  [[nodiscard]] static auto InspectValidatorFile(
      const std::filesystem::path& validator_config_path)
      -> RecordTemplateResult<ConfigInspectResult>;

  [[nodiscard]] static auto ListPeriods(
      const std::filesystem::path& input_path)
      -> RecordTemplateResult<ListedPeriodsResult>;
};

#endif  // RECORD_TEMPLATE_RECORD_TEMPLATE_SERVICE_HPP_
