#ifndef RECORD_TEMPLATE_PERIOD_SUPPORT_HPP_
#define RECORD_TEMPLATE_PERIOD_SUPPORT_HPP_

#include <filesystem>
#include <string>
#include <vector>

#include "record_template/record_template_types.hpp"

class RecordTemplatePeriodSupport {
 public:
  [[nodiscard]] static auto ExpandTemplatePeriods(
      const TemplateGenerationRequest& request)
      -> RecordTemplateResult<std::vector<std::string>>;

  [[nodiscard]] static auto ExtractPeriodFromNormalizedText(
      const std::string& normalized_text)
      -> RecordTemplateResult<std::string>;

  [[nodiscard]] static auto ExtractPeriodFromFile(
      const std::filesystem::path& file_path)
      -> RecordTemplateResult<std::string>;
};

#endif  // RECORD_TEMPLATE_PERIOD_SUPPORT_HPP_
