#include "standard_report_renderer_registry.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "reporting/standard_report/standard_report_json_serializer.hpp"
#include "standard_json_latex_renderer.hpp"
#include "standard_json_markdown_renderer.hpp"
#include "standard_json_rst_renderer.hpp"
#include "standard_json_typst_renderer.hpp"

namespace {

struct RendererEntry {
  std::string_view canonical_name;
  auto (*render)(const StandardReport&) -> std::string;
};

auto to_ascii_lower(std::string_view format_name) -> std::string {
  std::string normalized(format_name);
  std::ranges::transform(normalized, normalized.begin(),
                         [](unsigned char ch) -> char {
                           return static_cast<char>(std::tolower(ch));
                         });
  return normalized;
}

auto canonical_format_name(std::string_view format_name) -> std::string {
  std::string normalized = to_ascii_lower(format_name);
  if (normalized == "markdown") {
    return "md";
  }
  if (normalized == "latex") {
    return "tex";
  }
  if (normalized == "typst") {
    return "typ";
  }
  return normalized;
}

auto render_json(const StandardReport& standard_report) -> std::string {
  return StandardReportJsonSerializer::ToString(standard_report);
}

auto render_markdown(const StandardReport& standard_report) -> std::string {
  return StandardJsonMarkdownRenderer::render(standard_report);
}

#if BILLS_CORE_ENABLE_RENDERER_TEX
auto render_latex(const StandardReport& standard_report) -> std::string {
  return StandardJsonLatexRenderer::render(standard_report);
}
#endif

auto renderer_entries() -> const std::vector<RendererEntry>& {
  static const std::vector<RendererEntry> entries = [] {
    std::vector<RendererEntry> value;
#if BILLS_CORE_ENABLE_RENDERER_JSON
    value.push_back({"json", &render_json});
#endif
#if BILLS_CORE_ENABLE_RENDERER_MD
    value.push_back({"md", &render_markdown});
#endif
#if BILLS_CORE_ENABLE_RENDERER_RST
    value.push_back({"rst", &StandardJsonRstRenderer::render});
#endif
#if BILLS_CORE_ENABLE_RENDERER_TEX
    value.push_back({"tex", &render_latex});
#endif
#if BILLS_CORE_ENABLE_RENDERER_TYP
    value.push_back({"typ", &StandardJsonTypstRenderer::render});
#endif
    return value;
  }();
  return entries;
}

}  // namespace

auto StandardReportRendererRegistry::ListAvailableFormats()
    -> std::vector<std::string> {
  std::vector<std::string> formats;
  formats.reserve(renderer_entries().size());
  for (const auto& entry : renderer_entries()) {
    formats.emplace_back(entry.canonical_name);
  }
  return formats;
}

auto StandardReportRendererRegistry::NormalizeFormat(std::string_view format_name)
    -> std::string {
  return canonical_format_name(format_name);
}

auto StandardReportRendererRegistry::IsFormatAvailable(
    std::string_view format_name) -> bool {
  const std::string canonical = canonical_format_name(format_name);
  return std::ranges::any_of(renderer_entries(),
                             [&canonical](const RendererEntry& entry) -> bool {
                               return entry.canonical_name == canonical;
                             });
}

auto StandardReportRendererRegistry::Render(
    const StandardReport& standard_report, std::string_view format_name)
    -> std::string {
  const std::string canonical = canonical_format_name(format_name);
  const auto it = std::ranges::find_if(
      renderer_entries(), [&canonical](const RendererEntry& entry) -> bool {
        return entry.canonical_name == canonical;
      });
  if (it == renderer_entries().end()) {
    throw std::runtime_error("Report format '" + canonical +
                             "' is not available in the current build.");
  }
  return it->render(standard_report);
}
