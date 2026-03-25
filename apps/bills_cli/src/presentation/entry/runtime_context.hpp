#ifndef PRESENTATION_ENTRY_RUNTIME_CONTEXT_HPP_
#define PRESENTATION_ENTRY_RUNTIME_CONTEXT_HPP_

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <common/Result.hpp>

namespace bills::cli {

struct RuntimeContext {
  std::filesystem::path repo_root;
  std::filesystem::path executable_dir;
  std::filesystem::path runtime_workspace_dir;
  std::filesystem::path config_dir;
  std::filesystem::path default_db_path;
  std::filesystem::path json_cache_dir;
  std::filesystem::path export_dir;
  std::map<std::string, std::string> format_folder_names;
};

[[nodiscard]] auto BuildRuntimeContext() -> RuntimeContext;

[[nodiscard]] auto ResolveDbPath(
    const RuntimeContext& context,
    const std::optional<std::filesystem::path>& override_path)
    -> std::filesystem::path;

[[nodiscard]] auto LoadEnabledFormats(const RuntimeContext& context)
    -> Result<std::vector<std::string>>;

[[nodiscard]] auto ResolveSingleReportFormat(
    const RuntimeContext& context, std::string requested_format)
    -> Result<std::string>;

[[nodiscard]] auto ResolveExportFormats(const RuntimeContext& context,
                                        std::string requested_format)
    -> Result<std::vector<std::string>>;

[[nodiscard]] auto ReadBundledNotices(const RuntimeContext& context,
                                      bool raw_json) -> Result<std::string>;

}  // namespace bills::cli

#endif  // PRESENTATION_ENTRY_RUNTIME_CONTEXT_HPP_
