#ifndef RECORD_TEMPLATE_ORDERED_TEMPLATE_LAYOUT_LOADER_HPP_
#define RECORD_TEMPLATE_ORDERED_TEMPLATE_LAYOUT_LOADER_HPP_

#include <filesystem>

#include "record_template/record_template_types.hpp"

class OrderedTemplateLayoutLoader {
 public:
  [[nodiscard]] static auto LoadFromValidatorFile(
      const std::filesystem::path& validator_config_path)
      -> RecordTemplateResult<OrderedTemplateLayout>;
};

#endif  // RECORD_TEMPLATE_ORDERED_TEMPLATE_LAYOUT_LOADER_HPP_
