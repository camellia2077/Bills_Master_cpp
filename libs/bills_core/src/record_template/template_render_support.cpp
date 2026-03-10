#include "record_template/template_render_support.hpp"

#include <sstream>
#include <vector>

auto TemplateRenderSupport::BuildRelativeTemplatePath(std::string_view period)
    -> std::filesystem::path {
  return std::filesystem::path(std::string(period.substr(0U, 4U))) /
         std::filesystem::path(std::string(period) + ".txt");
}

auto TemplateRenderSupport::RenderTemplate(std::string_view period,
                                           const OrderedTemplateLayout& layout)
    -> std::string {
  std::vector<std::string> lines;
  lines.reserve(3U + layout.categories.size() * 4U);
  lines.push_back("date:" + std::string(period));
  lines.push_back("remark:");
  lines.push_back("");

  for (std::size_t category_index = 0; category_index < layout.categories.size();
       ++category_index) {
    const auto& category = layout.categories[category_index];
    lines.push_back(category.parent_item);
    lines.push_back("");

    for (const auto& sub_item : category.sub_items) {
      lines.push_back(sub_item);
      lines.push_back("");
    }

    if (category_index + 1U < layout.categories.size()) {
      lines.push_back("");
    }
  }

  std::ostringstream buffer;
  for (std::size_t line_index = 0; line_index < lines.size(); ++line_index) {
    buffer << lines[line_index];
    if (line_index + 1U < lines.size()) {
      buffer << '\n';
    }
  }
  return buffer.str();
}
