#include "record_command.hpp"

#include <cstddef>
#include <stdexcept>

#include "controllers/app_controller.hpp"

namespace {

auto ParseTemplateArgs(const std::vector<std::string>& args)
    -> RecordTemplateOptions {
  RecordTemplateOptions options;
  for (std::size_t index = 0; index < args.size(); ++index) {
    const auto& arg = args[index];
    auto require_value = [&args, &index](const char* flag_name) -> std::string {
      if (index + 1U >= args.size()) {
        throw std::runtime_error(std::string("Missing value for ") + flag_name + ".");
      }
      ++index;
      return args[index];
    };

    if (arg == "--period") {
      options.period = require_value("--period");
      continue;
    }
    if (arg == "--start-period") {
      options.start_period = require_value("--start-period");
      continue;
    }
    if (arg == "--end-period") {
      options.end_period = require_value("--end-period");
      continue;
    }
    if (arg == "--start-year") {
      options.start_year = require_value("--start-year");
      continue;
    }
    if (arg == "--end-year") {
      options.end_year = require_value("--end-year");
      continue;
    }
    if (arg == "--output-dir") {
      options.output_dir = require_value("--output-dir");
      continue;
    }

    throw std::runtime_error("Unknown argument for 'record template': '" + arg +
                             "'.");
  }
  return options;
}

}  // namespace

auto RecordCommand::execute(const std::vector<std::string>& args,
                            AppController& controller) -> bool {
  if (args.empty()) {
    throw std::runtime_error(
        "Missing sub-command for 'record'. Use 'template', 'preview', or 'list'.");
  }

  const std::string& sub_command = args[0];
  if (sub_command == "template") {
    const std::vector<std::string> template_args(args.begin() + 1, args.end());
    return controller.handle_record_template(ParseTemplateArgs(template_args));
  }

  if (sub_command == "preview") {
    if (args.size() != 2U) {
      throw std::runtime_error("record preview expects exactly one <path> argument.");
    }
    return controller.handle_record_preview(args[1]);
  }

  if (sub_command == "list") {
    if (args.size() != 2U) {
      throw std::runtime_error("record list expects exactly one <path> argument.");
    }
    return controller.handle_record_list(args[1]);
  }

  throw std::runtime_error("Unknown sub-command for 'record': " + sub_command);
}
