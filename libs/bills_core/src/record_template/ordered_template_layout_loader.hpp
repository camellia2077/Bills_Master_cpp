#ifndef RECORD_TEMPLATE_ORDERED_TEMPLATE_LAYOUT_LOADER_HPP_
#define RECORD_TEMPLATE_ORDERED_TEMPLATE_LAYOUT_LOADER_HPP_

#include "config/config_document_types.hpp"
#include "record_template/record_template_types.hpp"

class OrderedTemplateLayoutLoader {
 public:
  [[nodiscard]] static auto BuildFromDocument(
      const ValidatorConfigDocument& validator_document)
      -> RecordTemplateResult<OrderedTemplateLayout>;
};

#endif  // RECORD_TEMPLATE_ORDERED_TEMPLATE_LAYOUT_LOADER_HPP_
