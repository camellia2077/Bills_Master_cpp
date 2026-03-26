#if defined(BILLS_CLI_MODULES_ENABLED)
import bill.cli.presentation.parsing.cli_parser;
import bill.cli.presentation.parsing.cli_request;
#else
#include <presentation/parsing/cli_parser.hpp>
#endif

#include <pch.hpp>
#include <common/Result.hpp>

#include <CLI/CLI.hpp>

#include <filesystem>
#include <optional>
#include <sstream>
#include <string_view>
#include <vector>

namespace bills::cli {
namespace {

constexpr std::string_view kDefaultProgramName = "bills_tracer_cli";
constexpr std::string_view kFormatDescription =
    "Output format for the rendered or exported report. Use 'config formats' "
    "to inspect currently enabled formats.";

auto ParseError(std::string message) -> std::unexpected<Error> {
  return std::unexpected(MakeError(std::move(message)));
}

auto EnsureTrailingNewline(std::string text) -> std::string {
  if (!text.empty() && text.back() != '\n') {
    text.push_back('\n');
  }
  return text;
}

auto NormalizeProgramName(std::string_view program_name) -> std::string {
  if (program_name.empty()) {
    return std::string(kDefaultProgramName);
  }
  return std::string(program_name);
}

auto MakeExamplesBlock(std::initializer_list<std::string_view> examples)
    -> std::string {
  if (examples.size() == 0U) {
    return {};
  }

  std::ostringstream output;
  output << "Examples:\n";
  for (const auto& example : examples) {
    output << "  " << example << '\n';
  }
  return output.str();
}

void ConfigureCommand(CLI::App& command) {
  command.subcommand_fallthrough(false);
  command.allow_windows_style_options(false);
}

void SetExamples(CLI::App& command,
                 std::initializer_list<std::string_view> examples) {
  command.footer(MakeExamplesBlock(examples));
}

auto RenderParseOutput(CLI::App& app, const CLI::ParseError& error) -> std::string {
  std::ostringstream output;
  std::ostringstream err_output;
  app.exit(error, output, err_output);

  std::string rendered = err_output.str();
  if (!output.str().empty()) {
    if (!rendered.empty() && rendered.back() != '\n') {
      rendered.push_back('\n');
    }
    rendered += output.str();
  }
  return EnsureTrailingNewline(std::move(rendered));
}

}  // namespace

auto ParseCliRequest(std::string_view program_name,
                     const std::vector<std::string>& args)
    -> Result<CliRequest> {
  const std::string program = NormalizeProgramName(program_name);

  if (!args.empty()) {
    if (args.front() == "help") {
      return ParseError(
          "The 'help' subcommand was removed. Use '--help' or '<command> "
          "--help'.");
    }
    if (args.front() == "--version") {
      return ParseError("The '--version' flag was removed. Use 'meta version'.");
    }
    if (args.front() == "--notices" || args.front() == "--notices-json") {
      return ParseError(
          "Legacy notices flags were removed. Use 'meta notices [--json]'.");
    }
  }

  std::optional<CliRequest> parsed_request;

  CLI::App app("Bills CLI");
  ConfigureCommand(app);
  app.require_subcommand(0, 1);

  auto* workspace = app.add_subcommand(
      "workspace", "Validate, convert, ingest, and bundle workspace data.");
  ConfigureCommand(*workspace);
  workspace->require_subcommand(1);

  std::string workspace_validate_path;
  auto* workspace_validate = workspace->add_subcommand(
      "validate", "Validate source records under the provided path.");
  ConfigureCommand(*workspace_validate);
  workspace_validate
      ->add_option("path", workspace_validate_path,
                   "Path to the source records directory.")
      ->required();
  SetExamples(*workspace_validate,
              {"bills_tracer_cli workspace validate <path>"});
  workspace_validate->callback([&parsed_request, &workspace_validate_path]() {
    WorkspaceRequest request;
    request.action = WorkspaceAction::kValidate;
    request.input_path = std::filesystem::path(workspace_validate_path);
    parsed_request = CliRequest{request};
  });

  std::string workspace_convert_path;
  bool workspace_convert_write_json_cache = false;
  auto* workspace_convert = workspace->add_subcommand(
      "convert", "Convert source records into JSON cache entries.");
  ConfigureCommand(*workspace_convert);
  workspace_convert
      ->add_option("path", workspace_convert_path,
                   "Path to the source records directory.")
      ->required();
  workspace_convert->add_flag(
      "--write-json-cache", workspace_convert_write_json_cache,
      "Persist converted JSON cache files under the runtime workspace.");
  SetExamples(*workspace_convert,
              {"bills_tracer_cli workspace convert <path> --write-json-cache"});
  workspace_convert->callback(
      [&parsed_request, &workspace_convert_path,
       &workspace_convert_write_json_cache]() {
        WorkspaceRequest request;
        request.action = WorkspaceAction::kConvert;
        request.input_path = std::filesystem::path(workspace_convert_path);
        request.write_json_cache = workspace_convert_write_json_cache;
        parsed_request = CliRequest{request};
      });

  std::string workspace_ingest_path;
  std::string workspace_ingest_db;
  bool workspace_ingest_write_json_cache = false;
  auto* workspace_ingest = workspace->add_subcommand(
      "ingest", "Validate, convert, and import source records into the DB.");
  ConfigureCommand(*workspace_ingest);
  workspace_ingest
      ->add_option("path", workspace_ingest_path,
                   "Path to the source records directory.")
      ->required();
  workspace_ingest->add_option(
      "--db", workspace_ingest_db,
      "Override the runtime database path for the import step.");
  workspace_ingest->add_flag(
      "--write-json-cache", workspace_ingest_write_json_cache,
      "Persist converted JSON cache files under the runtime workspace.");
  SetExamples(
      *workspace_ingest,
      {"bills_tracer_cli workspace ingest <path>",
       "bills_tracer_cli workspace ingest <path> --db <path> "
       "--write-json-cache"});
  workspace_ingest->callback(
      [&parsed_request, &workspace_ingest_path, &workspace_ingest_db,
       &workspace_ingest_write_json_cache]() {
        WorkspaceRequest request;
        request.action = WorkspaceAction::kIngest;
        request.input_path = std::filesystem::path(workspace_ingest_path);
        if (!workspace_ingest_db.empty()) {
          request.db_path = std::filesystem::path(workspace_ingest_db);
        }
        request.write_json_cache = workspace_ingest_write_json_cache;
        parsed_request = CliRequest{request};
      });

  std::string workspace_import_json_path;
  std::string workspace_import_json_db;
  auto* workspace_import_json = workspace->add_subcommand(
      "import-json", "Import JSON cache entries into the runtime DB.");
  ConfigureCommand(*workspace_import_json);
  workspace_import_json
      ->add_option("path", workspace_import_json_path,
                   "Path to the JSON cache directory.")
      ->required();
  workspace_import_json->add_option(
      "--db", workspace_import_json_db,
      "Override the runtime database path for the import step.");
  SetExamples(
      *workspace_import_json,
      {"bills_tracer_cli workspace import-json <path>",
       "bills_tracer_cli workspace import-json <path> --db <path>"});
  workspace_import_json->callback(
      [&parsed_request, &workspace_import_json_path,
       &workspace_import_json_db]() {
        WorkspaceRequest request;
        request.action = WorkspaceAction::kImportJson;
        request.input_path = std::filesystem::path(workspace_import_json_path);
        if (!workspace_import_json_db.empty()) {
          request.db_path = std::filesystem::path(workspace_import_json_db);
        }
        parsed_request = CliRequest{request};
      });

  std::string workspace_export_bundle_records_dir;
  std::string workspace_export_bundle_output;
  auto* workspace_export_bundle = workspace->add_subcommand(
      "export-bundle", "Export a parse bundle ZIP from a records directory.");
  ConfigureCommand(*workspace_export_bundle);
  workspace_export_bundle
      ->add_option("records_dir", workspace_export_bundle_records_dir,
                   "Path to the source records directory.")
      ->required();
  workspace_export_bundle->add_option(
      "--output", workspace_export_bundle_output,
      "Write the bundle ZIP to an explicit output path.");
  SetExamples(
      *workspace_export_bundle,
      {"bills_tracer_cli workspace export-bundle <records-dir>",
       "bills_tracer_cli workspace export-bundle <records-dir> --output "
       "<bundle.zip>"});
  workspace_export_bundle->callback(
      [&parsed_request, &workspace_export_bundle_records_dir,
       &workspace_export_bundle_output]() {
        WorkspaceRequest request;
        request.action = WorkspaceAction::kExportBundle;
        request.input_path =
            std::filesystem::path(workspace_export_bundle_records_dir);
        if (!workspace_export_bundle_output.empty()) {
          request.output_path = std::filesystem::path(workspace_export_bundle_output);
        }
        parsed_request = CliRequest{request};
      });

  std::string workspace_import_bundle_path;
  std::string workspace_import_bundle_records_dir;
  auto* workspace_import_bundle = workspace->add_subcommand(
      "import-bundle", "Import a parse bundle ZIP into a records directory.");
  ConfigureCommand(*workspace_import_bundle);
  workspace_import_bundle
      ->add_option("bundle_path", workspace_import_bundle_path,
                   "Path to the source bundle ZIP file.")
      ->required();
  workspace_import_bundle
      ->add_option("records_dir", workspace_import_bundle_records_dir,
                   "Path to the destination records directory.")
      ->required();
  SetExamples(
      *workspace_import_bundle,
      {"bills_tracer_cli workspace import-bundle <bundle.zip> <records-dir>"});
  workspace_import_bundle->callback(
      [&parsed_request, &workspace_import_bundle_path,
       &workspace_import_bundle_records_dir]() {
        WorkspaceRequest request;
        request.action = WorkspaceAction::kImportBundle;
        request.input_path = std::filesystem::path(workspace_import_bundle_path);
        request.target_path =
            std::filesystem::path(workspace_import_bundle_records_dir);
        parsed_request = CliRequest{request};
      });

  auto* report = app.add_subcommand("report", "Show or export reports.");
  ConfigureCommand(*report);
  report->require_subcommand(1);

  auto* report_show =
      report->add_subcommand("show", "Render a report to standard output.");
  ConfigureCommand(*report_show);
  report_show->require_subcommand(1);

  std::string report_show_year_value;
  std::string report_show_year_format = "md";
  auto* report_show_year = report_show->add_subcommand(
      "year", "Render a yearly report for the requested year.");
  ConfigureCommand(*report_show_year);
  report_show_year
      ->add_option("year", report_show_year_value, "Year to query, such as 2025.")
      ->required();
  report_show_year->add_option("--format", report_show_year_format,
                               std::string(kFormatDescription));
  SetExamples(
      *report_show_year,
      {"bills_tracer_cli report show year <YYYY>",
       "bills_tracer_cli report show year <YYYY> --format json"});
  report_show_year->callback(
      [&parsed_request, &report_show_year_value, &report_show_year_format]() {
        ReportRequest request;
        request.action = ReportAction::kShowYear;
        request.primary_value = report_show_year_value;
        request.format = report_show_year_format;
        parsed_request = CliRequest{request};
      });

  std::string report_show_month_value;
  std::string report_show_month_format = "md";
  auto* report_show_month = report_show->add_subcommand(
      "month", "Render a monthly report for the requested period.");
  ConfigureCommand(*report_show_month);
  report_show_month
      ->add_option("month", report_show_month_value,
                   "Month to query, such as 2025-01.")
      ->required();
  report_show_month->add_option("--format", report_show_month_format,
                                std::string(kFormatDescription));
  SetExamples(
      *report_show_month,
      {"bills_tracer_cli report show month <YYYY-MM>",
       "bills_tracer_cli report show month <YYYY-MM> --format rst"});
  report_show_month->callback([&parsed_request, &report_show_month_value,
                               &report_show_month_format]() {
    ReportRequest request;
    request.action = ReportAction::kShowMonth;
    request.primary_value = report_show_month_value;
    request.format = report_show_month_format;
    parsed_request = CliRequest{request};
  });

  auto* report_export =
      report->add_subcommand("export", "Export reports into the runtime workspace.");
  ConfigureCommand(*report_export);
  report_export->require_subcommand(1);

  std::string report_export_year_value;
  std::string report_export_year_format = "md";
  auto* report_export_year = report_export->add_subcommand(
      "year", "Export yearly reports for the requested year.");
  ConfigureCommand(*report_export_year);
  report_export_year
      ->add_option("year", report_export_year_value,
                   "Year to export, such as 2025.")
      ->required();
  report_export_year->add_option("--format", report_export_year_format,
                                 std::string(kFormatDescription));
  SetExamples(
      *report_export_year,
      {"bills_tracer_cli report export year <YYYY>",
       "bills_tracer_cli report export year <YYYY> --format typ"});
  report_export_year->callback(
      [&parsed_request, &report_export_year_value, &report_export_year_format]() {
        ReportRequest request;
        request.action = ReportAction::kExportYear;
        request.primary_value = report_export_year_value;
        request.format = report_export_year_format;
        parsed_request = CliRequest{request};
      });

  std::string report_export_month_value;
  std::string report_export_month_format = "md";
  auto* report_export_month = report_export->add_subcommand(
      "month", "Export monthly reports for the requested period.");
  ConfigureCommand(*report_export_month);
  report_export_month
      ->add_option("month", report_export_month_value,
                   "Month to export, such as 2025-01.")
      ->required();
  report_export_month->add_option("--format", report_export_month_format,
                                  std::string(kFormatDescription));
  SetExamples(
      *report_export_month,
      {"bills_tracer_cli report export month <YYYY-MM>",
       "bills_tracer_cli report export month <YYYY-MM> --format tex"});
  report_export_month->callback(
      [&parsed_request, &report_export_month_value,
       &report_export_month_format]() {
        ReportRequest request;
        request.action = ReportAction::kExportMonth;
        request.primary_value = report_export_month_value;
        request.format = report_export_month_format;
        parsed_request = CliRequest{request};
      });

  std::string report_export_range_start;
  std::string report_export_range_end;
  std::string report_export_range_format = "md";
  auto* report_export_range = report_export->add_subcommand(
      "range", "Export monthly reports for an inclusive period range.");
  ConfigureCommand(*report_export_range);
  report_export_range
      ->add_option("start_month", report_export_range_start,
                   "Start month, such as 2025-03.")
      ->required();
  report_export_range
      ->add_option("end_month", report_export_range_end,
                   "End month, such as 2025-04.")
      ->required();
  report_export_range->add_option("--format", report_export_range_format,
                                  std::string(kFormatDescription));
  SetExamples(
      *report_export_range,
      {"bills_tracer_cli report export range <YYYY-MM> <YYYY-MM>",
       "bills_tracer_cli report export range <YYYY-MM> <YYYY-MM> --format "
       "json"});
  report_export_range->callback(
      [&parsed_request, &report_export_range_start, &report_export_range_end,
       &report_export_range_format]() {
        ReportRequest request;
        request.action = ReportAction::kExportRange;
        request.primary_value = report_export_range_start;
        request.secondary_value = report_export_range_end;
        request.format = report_export_range_format;
        parsed_request = CliRequest{request};
      });

  std::string report_export_all_months_format = "md";
  auto* report_export_all_months = report_export->add_subcommand(
      "all-months", "Export all monthly reports that exist in the database.");
  ConfigureCommand(*report_export_all_months);
  report_export_all_months->add_option("--format", report_export_all_months_format,
                                       std::string(kFormatDescription));
  SetExamples(
      *report_export_all_months,
      {"bills_tracer_cli report export all-months",
       "bills_tracer_cli report export all-months --format all"});
  report_export_all_months->callback(
      [&parsed_request, &report_export_all_months_format]() {
        ReportRequest request;
        request.action = ReportAction::kExportAllMonths;
        request.format = report_export_all_months_format;
        parsed_request = CliRequest{request};
      });

  std::string report_export_all_years_format = "md";
  auto* report_export_all_years = report_export->add_subcommand(
      "all-years", "Export all yearly reports that exist in the database.");
  ConfigureCommand(*report_export_all_years);
  report_export_all_years->add_option("--format", report_export_all_years_format,
                                      std::string(kFormatDescription));
  SetExamples(
      *report_export_all_years,
      {"bills_tracer_cli report export all-years",
       "bills_tracer_cli report export all-years --format rst"});
  report_export_all_years->callback(
      [&parsed_request, &report_export_all_years_format]() {
        ReportRequest request;
        request.action = ReportAction::kExportAllYears;
        request.format = report_export_all_years_format;
        parsed_request = CliRequest{request};
      });

  std::string report_export_all_format = "md";
  auto* report_export_all = report_export->add_subcommand(
      "all", "Export every supported report scope from the database.");
  ConfigureCommand(*report_export_all);
  report_export_all->add_option("--format", report_export_all_format,
                                std::string(kFormatDescription));
  SetExamples(*report_export_all,
              {"bills_tracer_cli report export all",
               "bills_tracer_cli report export all --format json"});
  report_export_all->callback([&parsed_request, &report_export_all_format]() {
    ReportRequest request;
    request.action = ReportAction::kExportAll;
    request.format = report_export_all_format;
    parsed_request = CliRequest{request};
  });

  auto* template_command = app.add_subcommand(
      "template", "Generate or inspect record templates.");
  ConfigureCommand(*template_command);
  template_command->require_subcommand(1);

  std::string template_generate_period;
  std::string template_generate_start_period;
  std::string template_generate_end_period;
  std::string template_generate_start_year;
  std::string template_generate_end_year;
  std::string template_generate_output_dir;
  auto* template_generate = template_command->add_subcommand(
      "generate", "Generate record templates for the requested period scope.");
  ConfigureCommand(*template_generate);
  template_generate->add_option("--period", template_generate_period,
                                "Generate a template for one month.");
  template_generate->add_option(
      "--start-period", template_generate_start_period,
      "Start month for a range generation request.");
  template_generate->add_option(
      "--end-period", template_generate_end_period,
      "End month for a range generation request.");
  template_generate->add_option("--start-year", template_generate_start_year,
                                "Start year for a yearly generation request.");
  template_generate->add_option("--end-year", template_generate_end_year,
                                "End year for a yearly generation request.");
  template_generate->add_option(
      "--output-dir", template_generate_output_dir,
      "Write generated template files under an explicit output directory.");
  SetExamples(
      *template_generate,
      {"bills_tracer_cli template generate --period <YYYY-MM>",
       "bills_tracer_cli template generate --start-period <YYYY-MM> "
       "--end-period <YYYY-MM>",
       "bills_tracer_cli template generate --start-year <YYYY> --end-year "
       "<YYYY>"});
  template_generate->callback(
      [&parsed_request, &template_generate_period,
       &template_generate_start_period, &template_generate_end_period,
       &template_generate_start_year, &template_generate_end_year,
       &template_generate_output_dir]() {
        TemplateRequest request;
        request.action = TemplateAction::kGenerate;
        request.period = template_generate_period;
        request.start_period = template_generate_start_period;
        request.end_period = template_generate_end_period;
        request.start_year = template_generate_start_year;
        request.end_year = template_generate_end_year;
        if (!template_generate_output_dir.empty()) {
          request.output_dir = std::filesystem::path(template_generate_output_dir);
        }
        parsed_request = CliRequest{request};
      });

  std::string template_preview_path;
  auto* template_preview = template_command->add_subcommand(
      "preview", "Preview a generated record template file.");
  ConfigureCommand(*template_preview);
  template_preview
      ->add_option("path", template_preview_path,
                   "Path to the generated record template file.")
      ->required();
  SetExamples(*template_preview,
              {"bills_tracer_cli template preview <path>"});
  template_preview->callback([&parsed_request, &template_preview_path]() {
    TemplateRequest request;
    request.action = TemplateAction::kPreview;
    request.input_path = std::filesystem::path(template_preview_path);
    parsed_request = CliRequest{request};
  });

  std::string template_list_periods_path;
  auto* template_list_periods = template_command->add_subcommand(
      "list-periods", "List available periods under a template output root.");
  ConfigureCommand(*template_list_periods);
  template_list_periods
      ->add_option("path", template_list_periods_path,
                   "Path to the template output root.")
      ->required();
  SetExamples(
      *template_list_periods,
      {"bills_tracer_cli template list-periods <path>"});
  template_list_periods->callback(
      [&parsed_request, &template_list_periods_path]() {
        TemplateRequest request;
        request.action = TemplateAction::kListPeriods;
        request.input_path = std::filesystem::path(template_list_periods_path);
        parsed_request = CliRequest{request};
      });

  auto* config = app.add_subcommand(
      "config", "Inspect runtime configuration and enabled formats.");
  ConfigureCommand(*config);
  config->require_subcommand(1);

  auto* config_inspect =
      config->add_subcommand("inspect", "Render the effective runtime config.");
  ConfigureCommand(*config_inspect);
  SetExamples(*config_inspect, {"bills_tracer_cli config inspect"});
  config_inspect->callback([&parsed_request]() {
    ConfigRequest request;
    request.action = ConfigAction::kInspect;
    parsed_request = CliRequest{request};
  });

  auto* config_formats =
      config->add_subcommand("formats", "List currently enabled report formats.");
  ConfigureCommand(*config_formats);
  SetExamples(*config_formats, {"bills_tracer_cli config formats"});
  config_formats->callback([&parsed_request]() {
    ConfigRequest request;
    request.action = ConfigAction::kFormats;
    parsed_request = CliRequest{request};
  });

  auto* meta = app.add_subcommand("meta", "Version and notices information.");
  ConfigureCommand(*meta);
  meta->require_subcommand(1);

  auto* meta_version =
      meta->add_subcommand("version", "Print the CLI version string.");
  ConfigureCommand(*meta_version);
  SetExamples(*meta_version, {"bills_tracer_cli meta version"});
  meta_version->callback([&parsed_request]() {
    MetaRequest request;
    request.action = MetaAction::kVersion;
    parsed_request = CliRequest{request};
  });

  bool meta_notices_json = false;
  auto* meta_notices = meta->add_subcommand(
      "notices", "Print bundled notices in markdown or JSON form.");
  ConfigureCommand(*meta_notices);
  meta_notices->add_flag("--json", meta_notices_json,
                         "Print notices as JSON instead of markdown.");
  SetExamples(
      *meta_notices,
      {"bills_tracer_cli meta notices",
       "bills_tracer_cli meta notices --json"});
  meta_notices->callback([&parsed_request, &meta_notices_json]() {
    MetaRequest request;
    request.action = MetaAction::kNotices;
    request.raw_json = meta_notices_json;
    parsed_request = CliRequest{request};
  });

  if (args.empty()) {
    return CliRequest{HelpRequest{EnsureTrailingNewline(app.help(program))}};
  }

  std::vector<std::string> argv_storage;
  argv_storage.reserve(args.size() + 1U);
  argv_storage.emplace_back(program);
  for (const auto& arg : args) {
    argv_storage.push_back(arg);
  }

  std::vector<char*> argv;
  argv.reserve(argv_storage.size());
  for (auto& arg : argv_storage) {
    argv.push_back(arg.data());
  }

  try {
    app.parse(static_cast<int>(argv.size()), argv.data());
  } catch (const CLI::ParseError& error) {
    const int exit_code = error.get_exit_code();
    const std::string rendered = RenderParseOutput(app, error);
    if (exit_code == 0) {
      return CliRequest{HelpRequest{rendered}};
    }
    return ParseError(rendered);
  }

  if (!parsed_request.has_value()) {
    return ParseError("No command was selected.\n");
  }
  return *parsed_request;
}

}  // namespace bills::cli
