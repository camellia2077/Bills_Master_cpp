#include "presentation/parsing/cli_parser.hpp"

#include <filesystem>
#include <optional>
#include <string_view>
#include <vector>

namespace bills::cli {
namespace {

constexpr std::string_view kContext = "CliParser";

auto ParseError(std::string message) -> std::unexpected<Error> {
  return std::unexpected(MakeError(std::move(message), std::string(kContext)));
}

auto ConsumeValue(const std::vector<std::string>& args, std::size_t& index,
                  std::string_view option_name) -> Result<std::string> {
  if (index + 1U >= args.size()) {
    return std::unexpected(MakeError(
        "Missing value for option '" + std::string(option_name) + "'.",
        std::string(kContext)));
  }
  ++index;
  return args[index];
}

auto HasGenerationScope(const TemplateRequest& request) -> bool {
  return !request.period.empty() || !request.start_period.empty() ||
         !request.end_period.empty() || !request.start_year.empty() ||
         !request.end_year.empty();
}

auto ParseWorkspaceRequest(const std::vector<std::string>& args)
    -> Result<CliRequest> {
  if (args.size() < 2U) {
    return ParseError(
        "Missing workspace subcommand. Use 'workspace validate', "
        "'workspace convert', 'workspace ingest', or 'workspace import-json'.");
  }

  WorkspaceRequest request;
  const std::string& subcommand = args[1];
  if (subcommand == "validate") {
    request.action = WorkspaceAction::kValidate;
  } else if (subcommand == "convert") {
    request.action = WorkspaceAction::kConvert;
  } else if (subcommand == "ingest") {
    request.action = WorkspaceAction::kIngest;
  } else if (subcommand == "import-json") {
    request.action = WorkspaceAction::kImportJson;
  } else {
    return ParseError("Unknown workspace subcommand: '" + subcommand + "'.");
  }

  std::vector<std::string> positional_args;
  for (std::size_t index = 2U; index < args.size(); ++index) {
    const std::string& arg = args[index];
    if (arg == "--db") {
      if (request.action != WorkspaceAction::kIngest &&
          request.action != WorkspaceAction::kImportJson) {
        return ParseError("Option '--db' is only valid for "
                          "'workspace ingest' or 'workspace import-json'.");
      }
      auto value = ConsumeValue(args, index, arg);
      if (!value) {
        return std::unexpected(value.error());
      }
      request.db_path = std::filesystem::path(*value);
      continue;
    }
    if (arg == "--write-json-cache") {
      if (request.action != WorkspaceAction::kConvert &&
          request.action != WorkspaceAction::kIngest) {
        return ParseError("Option '--write-json-cache' is only valid for "
                          "'workspace convert' or 'workspace ingest'.");
      }
      request.write_json_cache = true;
      continue;
    }
    if (arg.starts_with("--")) {
      return ParseError("Unknown workspace option: '" + arg + "'.");
    }
    positional_args.push_back(arg);
  }

  if (positional_args.size() != 1U) {
    return ParseError(
        "Workspace commands require exactly one <path> argument.");
  }
  request.input_path = std::filesystem::path(positional_args.front());
  return CliRequest{request};
}

auto ParseReportRequest(const std::vector<std::string>& args)
    -> Result<CliRequest> {
  if (args.size() < 2U) {
    return ParseError(
        "Missing report subcommand. Use 'report show ...' or 'report export ...'.");
  }

  ReportRequest request;
  const std::string& mode = args[1];
  std::size_t value_start = 0U;
  if (mode == "show") {
    if (args.size() < 3U) {
      return ParseError(
          "Missing report show target. Use 'report show year <YYYY>' or "
          "'report show month <YYYY-MM>'.");
    }
    if (args[2] == "year") {
      request.action = ReportAction::kShowYear;
    } else if (args[2] == "month") {
      request.action = ReportAction::kShowMonth;
    } else {
      return ParseError("Unknown report show target: '" + args[2] + "'.");
    }
    value_start = 3U;
  } else if (mode == "export") {
    if (args.size() < 3U) {
      return ParseError(
          "Missing report export target. Use 'report export year|month|range|all-months|all-years|all'.");
    }
    if (args[2] == "year") {
      request.action = ReportAction::kExportYear;
    } else if (args[2] == "month") {
      request.action = ReportAction::kExportMonth;
    } else if (args[2] == "range") {
      request.action = ReportAction::kExportRange;
    } else if (args[2] == "all-months") {
      request.action = ReportAction::kExportAllMonths;
    } else if (args[2] == "all-years") {
      request.action = ReportAction::kExportAllYears;
    } else if (args[2] == "all") {
      request.action = ReportAction::kExportAll;
    } else {
      return ParseError("Unknown report export target: '" + args[2] + "'.");
    }
    value_start = 3U;
  } else {
    return ParseError("Unknown report subcommand: '" + mode + "'.");
  }

  std::vector<std::string> positional_args;
  for (std::size_t index = value_start; index < args.size(); ++index) {
    const std::string& arg = args[index];
    if (arg == "--format") {
      auto value = ConsumeValue(args, index, arg);
      if (!value) {
        return std::unexpected(value.error());
      }
      request.format = *value;
      continue;
    }
    if (arg.starts_with("--")) {
      return ParseError("Unknown report option: '" + arg + "'.");
    }
    positional_args.push_back(arg);
  }

  const auto require_exact = [&](std::size_t expected_count,
                                 std::string usage) -> Result<void> {
    if (positional_args.size() != expected_count) {
      return std::unexpected(MakeError(std::move(usage), std::string(kContext)));
    }
    return {};
  };

  switch (request.action) {
    case ReportAction::kShowYear:
    case ReportAction::kShowMonth:
    case ReportAction::kExportYear:
    case ReportAction::kExportMonth: {
      const auto result = require_exact(
          1U, "Report command expects exactly one value argument.");
      if (!result) {
        return std::unexpected(result.error());
      }
      request.primary_value = positional_args[0];
      break;
    }
    case ReportAction::kExportRange: {
      const auto result = require_exact(
          2U, "Report range export expects <start YYYY-MM> <end YYYY-MM>.");
      if (!result) {
        return std::unexpected(result.error());
      }
      request.primary_value = positional_args[0];
      request.secondary_value = positional_args[1];
      break;
    }
    case ReportAction::kExportAllMonths:
    case ReportAction::kExportAllYears:
    case ReportAction::kExportAll: {
      const auto result = require_exact(
          0U, "This report export command does not accept positional values.");
      if (!result) {
        return std::unexpected(result.error());
      }
      break;
    }
  }

  return CliRequest{request};
}

auto ParseTemplateRequest(const std::vector<std::string>& args)
    -> Result<CliRequest> {
  if (args.size() < 2U) {
    return ParseError(
        "Missing template subcommand. Use 'template generate', "
        "'template preview', or 'template list-periods'.");
  }

  TemplateRequest request;
  const std::string& subcommand = args[1];
  if (subcommand == "generate") {
    request.action = TemplateAction::kGenerate;
    for (std::size_t index = 2U; index < args.size(); ++index) {
      const std::string& arg = args[index];
      if (arg == "--period") {
        auto value = ConsumeValue(args, index, arg);
        if (!value) {
          return std::unexpected(value.error());
        }
        request.period = *value;
      } else if (arg == "--start-period") {
        auto value = ConsumeValue(args, index, arg);
        if (!value) {
          return std::unexpected(value.error());
        }
        request.start_period = *value;
      } else if (arg == "--end-period") {
        auto value = ConsumeValue(args, index, arg);
        if (!value) {
          return std::unexpected(value.error());
        }
        request.end_period = *value;
      } else if (arg == "--start-year") {
        auto value = ConsumeValue(args, index, arg);
        if (!value) {
          return std::unexpected(value.error());
        }
        request.start_year = *value;
      } else if (arg == "--end-year") {
        auto value = ConsumeValue(args, index, arg);
        if (!value) {
          return std::unexpected(value.error());
        }
        request.end_year = *value;
      } else if (arg == "--output-dir") {
        auto value = ConsumeValue(args, index, arg);
        if (!value) {
          return std::unexpected(value.error());
        }
        request.output_dir = std::filesystem::path(*value);
      } else {
        return ParseError("Unknown template generate option: '" + arg + "'.");
      }
    }
    if (!HasGenerationScope(request)) {
      return ParseError(
          "template generate requires --period, --start-period/--end-period, "
          "or --start-year/--end-year.");
    }
    return CliRequest{request};
  }

  std::vector<std::string> positional_args;
  for (std::size_t index = 2U; index < args.size(); ++index) {
    const std::string& arg = args[index];
    if (arg.starts_with("--")) {
      return ParseError("Unknown template option: '" + arg + "'.");
    }
    positional_args.push_back(arg);
  }
  if (positional_args.size() != 1U) {
    return ParseError("Template preview/list commands require exactly one <path>.");
  }
  request.input_path = std::filesystem::path(positional_args.front());

  if (subcommand == "preview") {
    request.action = TemplateAction::kPreview;
    return CliRequest{request};
  }
  if (subcommand == "list-periods") {
    request.action = TemplateAction::kListPeriods;
    return CliRequest{request};
  }

  return ParseError("Unknown template subcommand: '" + subcommand + "'.");
}

auto ParseConfigRequest(const std::vector<std::string>& args)
    -> Result<CliRequest> {
  if (args.size() < 2U) {
    return ParseError("Missing config subcommand. Use 'config inspect' or 'config formats'.");
  }

  ConfigRequest request;
  if (args[1] == "inspect") {
    request.action = ConfigAction::kInspect;
  } else if (args[1] == "formats") {
    request.action = ConfigAction::kFormats;
  } else {
    return ParseError("Unknown config subcommand: '" + args[1] + "'.");
  }

  if (args.size() != 2U) {
    return ParseError("Config commands do not accept extra positional arguments.");
  }
  return CliRequest{request};
}

auto ParseMetaRequest(const std::vector<std::string>& args) -> Result<CliRequest> {
  if (args.size() < 2U) {
    return ParseError("Missing meta subcommand. Use 'meta version' or 'meta notices'.");
  }

  MetaRequest request;
  if (args[1] == "version") {
    request.action = MetaAction::kVersion;
    if (args.size() != 2U) {
      return ParseError("meta version does not accept extra arguments.");
    }
    return CliRequest{request};
  }

  if (args[1] != "notices") {
    return ParseError("Unknown meta subcommand: '" + args[1] + "'.");
  }

  request.action = MetaAction::kNotices;
  for (std::size_t index = 2U; index < args.size(); ++index) {
    const std::string& arg = args[index];
    if (arg == "--json") {
      request.raw_json = true;
      continue;
    }
    return ParseError("Unknown meta notices option: '" + arg + "'.");
  }
  return CliRequest{request};
}

}  // namespace

auto ParseCliRequest(const std::vector<std::string>& args) -> Result<CliRequest> {
  if (args.empty()) {
    return CliRequest{HelpRequest{}};
  }

  const std::string& group = args.front();
  if (group == "help" || group == "--help" || group == "-h") {
    return CliRequest{HelpRequest{}};
  }
  if (group == "--version") {
    return ParseError("The '--version' flag was removed. Use 'meta version'.");
  }
  if (group == "--notices" || group == "--notices-json") {
    return ParseError("Legacy notices flags were removed. Use 'meta notices [--json]'.");
  }
  if (group == "workspace") {
    return ParseWorkspaceRequest(args);
  }
  if (group == "report") {
    return ParseReportRequest(args);
  }
  if (group == "template") {
    return ParseTemplateRequest(args);
  }
  if (group == "config") {
    return ParseConfigRequest(args);
  }
  if (group == "meta") {
    return ParseMetaRequest(args);
  }

  return ParseError("Unknown command group: '" + group + "'.");
}

}  // namespace bills::cli
