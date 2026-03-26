#if defined(BILLS_CLI_MODULES_ENABLED)
import bill.cli.presentation.entry.runtime_context;
import bill.cli.deps.io_host_flow_support;
import bill.cli.deps.common_utils;
import bill.cli.deps.renderer_registry;
#else
#include <presentation/entry/runtime_context.hpp>
#endif

#include <pch.hpp>
#include <common/Result.hpp>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include <set>
#include <sstream>
#include <system_error>

#ifdef _WIN32
#include <windows.h>
#endif

namespace terminal = bills::cli::terminal;

namespace bills::cli {
namespace {

constexpr const char* kRuntimeWorkspaceOverrideEnv =
    "BILLS_TRACER_RUNTIME_WORKSPACE_DIR";
constexpr const char* kProjectName = "bills_tracer";
constexpr const char* kNoticesMarkdownPath = "notices/NOTICE.md";
constexpr const char* kNoticesJsonPath = "notices/notices.json";

auto GetExecutableDirectory() -> std::filesystem::path {
#ifdef _WIN32
  wchar_t path[MAX_PATH] = {0};
  GetModuleFileNameW(nullptr, path, MAX_PATH);
  return std::filesystem::path(path).parent_path();
#else
  return std::filesystem::current_path();
#endif
}

auto HasRepoMarkers(const std::filesystem::path& candidate) -> bool {
  return std::filesystem::exists(candidate / "apps") &&
         std::filesystem::exists(candidate / "libs") &&
         std::filesystem::exists(candidate / "tools") &&
         std::filesystem::exists(candidate / "config");
}

auto FindRepoRoot(const std::filesystem::path& start_dir)
    -> std::filesystem::path {
  for (std::filesystem::path current = start_dir; !current.empty();
       current = current.parent_path()) {
    if (HasRepoMarkers(current)) {
      return current;
    }
    if (current == current.root_path()) {
      break;
    }
  }
  return {};
}

auto ResolveRepoRoot() -> std::filesystem::path {
  const std::filesystem::path executable_dir = GetExecutableDirectory();
  if (const auto from_executable = FindRepoRoot(executable_dir);
      !from_executable.empty()) {
    return from_executable;
  }
  if (const auto from_cwd = FindRepoRoot(std::filesystem::current_path());
      !from_cwd.empty()) {
    return from_cwd;
  }
  return executable_dir;
}

auto ResolveRuntimeWorkspace(const std::filesystem::path& repo_root)
    -> std::filesystem::path {
  if (const char* override_value = std::getenv(kRuntimeWorkspaceOverrideEnv);
      override_value != nullptr && override_value[0] != '\0') {
    std::filesystem::path override_path(override_value);
    if (override_path.is_relative()) {
      return (repo_root / override_path).lexically_normal();
    }
    return override_path.lexically_normal();
  }
  return (repo_root / "dist" / "runtime" / kProjectName / "workspace")
      .lexically_normal();
}

void EnsureDirectory(const std::filesystem::path& dir_path) {
  std::error_code create_error;
  std::filesystem::create_directories(dir_path, create_error);
  if (create_error) {
    std::cerr << terminal::kYellow << "Warning: " << terminal::kReset
              << "Failed to ensure runtime directory exists: "
              << dir_path.string() << ". Reason: " << create_error.message()
              << '\n';
  }
}

auto ReadTextFile(const std::filesystem::path& file_path) -> Result<std::string> {
  std::ifstream input(file_path, std::ios::binary);
  if (!input) {
    return std::unexpected(
        MakeError("Failed to open file: " + file_path.string(), "RuntimeContext"));
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

auto TrimAsciiWhitespace(std::string value) -> std::string {
  const auto is_space = [](unsigned char ch) -> bool {
    return std::isspace(ch) != 0;
  };
  while (!value.empty() && is_space(static_cast<unsigned char>(value.front()))) {
    value.erase(value.begin());
  }
  while (!value.empty() && is_space(static_cast<unsigned char>(value.back()))) {
    value.pop_back();
  }
  return value;
}

auto SplitRequestedFormats(std::string_view requested_formats)
    -> std::vector<std::string> {
  std::vector<std::string> parts;
  std::string current;
  for (const char ch : requested_formats) {
    if (ch == ',') {
      parts.push_back(TrimAsciiWhitespace(current));
      current.clear();
      continue;
    }
    current.push_back(ch);
  }
  parts.push_back(TrimAsciiWhitespace(current));
  return parts;
}

}  // namespace

auto BuildRuntimeContext() -> RuntimeContext {
  const std::filesystem::path repo_root = ResolveRepoRoot();
  const std::filesystem::path executable_dir = GetExecutableDirectory();
  const std::filesystem::path runtime_workspace_dir =
      ResolveRuntimeWorkspace(repo_root);
  const std::filesystem::path default_db_path =
      runtime_workspace_dir / "db" / "bills.sqlite3";
  const std::filesystem::path json_cache_dir =
      runtime_workspace_dir / "cache" / "txt2json";
  const std::filesystem::path export_dir = runtime_workspace_dir / "exports";

  EnsureDirectory(default_db_path.parent_path());
  EnsureDirectory(json_cache_dir);
  EnsureDirectory(export_dir);

  return RuntimeContext{
      .repo_root = repo_root,
      .executable_dir = executable_dir,
      .runtime_workspace_dir = runtime_workspace_dir,
      .config_dir = executable_dir / "config",
      .default_db_path = default_db_path,
      .json_cache_dir = json_cache_dir,
      .export_dir = export_dir,
      .format_folder_names = {{"json", "JSON_bills"},
                              {"md", "Markdown_bills"},
                              {"rst", "reST_bills"},
                              {"tex", "LaTeX_bills"},
                              {"typ", "Typst_bills"}},
  };
}

auto ResolveDbPath(const RuntimeContext& context,
                   const std::optional<std::filesystem::path>& override_path)
    -> std::filesystem::path {
  if (!override_path.has_value() || override_path->empty()) {
    return context.default_db_path;
  }
  if (override_path->is_relative()) {
    return std::filesystem::absolute(*override_path).lexically_normal();
  }
  return override_path->lexically_normal();
}

auto LoadEnabledFormats(const RuntimeContext& context)
    -> Result<std::vector<std::string>> {
  const auto validated_documents =
      bills::io::LoadValidatedConfigContext(context.config_dir);
  if (!validated_documents) {
    return std::unexpected(validated_documents.error());
  }

  std::set<std::string> available_formats(
      validated_documents->validated.available_export_formats.begin(),
      validated_documents->validated.available_export_formats.end());
  std::set<std::string> enabled_formats(
      validated_documents->validated.enabled_export_formats.begin(),
      validated_documents->validated.enabled_export_formats.end());

  const auto build_formats = StandardReportRendererRegistry::ListAvailableFormats();
  if (available_formats.empty()) {
    available_formats.insert(build_formats.begin(), build_formats.end());
  }
  if (enabled_formats.empty()) {
    enabled_formats = available_formats;
  }

  std::vector<std::string> ordered_formats;
  for (const auto& format : build_formats) {
    if (enabled_formats.contains(format)) {
      ordered_formats.push_back(format);
    }
  }
  for (const auto& format : enabled_formats) {
    if (std::ranges::find(ordered_formats, format) == ordered_formats.end()) {
      ordered_formats.push_back(format);
    }
  }
  return ordered_formats;
}

auto ResolveSingleReportFormat(const RuntimeContext& context,
                               std::string requested_format)
    -> Result<std::string> {
  const std::string normalized =
      StandardReportRendererRegistry::NormalizeFormat(requested_format);
  if (normalized.empty() || normalized == "all") {
    return std::unexpected(MakeError(
        "Report format must be one of: md, json, rst, tex, typ.",
        "RuntimeContext"));
  }

  const auto enabled_formats = LoadEnabledFormats(context);
  if (!enabled_formats) {
    return std::unexpected(enabled_formats.error());
  }
  if (std::ranges::find(*enabled_formats, normalized) == enabled_formats->end()) {
    return std::unexpected(MakeError(
        "Format '" + normalized + "' is not enabled in config.", "RuntimeContext"));
  }
  if (!StandardReportRendererRegistry::IsFormatAvailable(normalized)) {
    return std::unexpected(MakeError(
        "Format '" + normalized + "' is not available in this build.",
        "RuntimeContext"));
  }
  return normalized;
}

auto ResolveExportFormats(const RuntimeContext& context, std::string requested_format)
    -> Result<std::vector<std::string>> {
  const std::string trimmed_requested = TrimAsciiWhitespace(requested_format);
  if (trimmed_requested.empty()) {
    return LoadEnabledFormats(context);
  }

  std::vector<std::string> resolved_formats;
  for (const auto& raw_part : SplitRequestedFormats(trimmed_requested)) {
    if (raw_part.empty()) {
      return std::unexpected(
          MakeError("Report format list contains an empty item.",
                    "RuntimeContext"));
    }

    const std::string normalized =
        StandardReportRendererRegistry::NormalizeFormat(raw_part);
    if (normalized.empty()) {
      return std::unexpected(
          MakeError("Unknown report format: '" + raw_part + "'.",
                    "RuntimeContext"));
    }
    if (normalized == "all") {
      const auto enabled_formats = LoadEnabledFormats(context);
      if (!enabled_formats) {
        return std::unexpected(enabled_formats.error());
      }
      for (const auto& enabled_format : *enabled_formats) {
        if (std::ranges::find(resolved_formats, enabled_format) ==
            resolved_formats.end()) {
          resolved_formats.push_back(enabled_format);
        }
      }
      continue;
    }

    auto single_format = ResolveSingleReportFormat(context, normalized);
    if (!single_format) {
      return std::unexpected(single_format.error());
    }
    if (std::ranges::find(resolved_formats, *single_format) ==
        resolved_formats.end()) {
      resolved_formats.push_back(*single_format);
    }
  }

  if (resolved_formats.empty()) {
    return std::unexpected(MakeError(
        "No enabled export formats are available in this build.",
        "RuntimeContext"));
  }
  return resolved_formats;
}

auto ReadBundledNotices(const RuntimeContext& context, bool raw_json)
    -> Result<std::string> {
  const std::filesystem::path notices_path =
      context.runtime_workspace_dir /
      (raw_json ? kNoticesJsonPath : kNoticesMarkdownPath);
  return ReadTextFile(notices_path);
}

}  // namespace bills::cli
