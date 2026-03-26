#ifndef RECORD_TEMPLATE_TEMPLATE_RENDER_SUPPORT_HPP_
#define RECORD_TEMPLATE_TEMPLATE_RENDER_SUPPORT_HPP_

#include <string>
#include <string_view>

#include "record_template/record_template_types.hpp"

class TemplateRenderSupport {
 public:
  [[nodiscard]] static auto BuildRelativeTemplatePath(std::string_view period)
      -> std::string;

  [[nodiscard]] static auto RenderTemplate(std::string_view period,
                                           const OrderedTemplateLayout& layout)
      -> std::string;
};

#endif  // RECORD_TEMPLATE_TEMPLATE_RENDER_SUPPORT_HPP_
