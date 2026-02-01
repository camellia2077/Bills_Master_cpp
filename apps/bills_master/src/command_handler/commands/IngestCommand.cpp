// command_handler/commands/IngestCommand.cpp
#include "IngestCommand.hpp"
#include <stdexcept>

auto IngestCommand::execute(const std::vector<std::string>& args,
                            AppController& controller) -> bool {
  std::string path;
  bool write_json = false;
  parse_args(args, path, write_json);
  if (path.empty()) {
    throw std::runtime_error("Missing path for 'ingest' command.");
  }
  return controller.handle_ingest(path, write_json);
}

auto IngestCommand::parse_args(const std::vector<std::string>& args,
                               std::string& path,
                               bool& write_json) -> void {
  for (const auto& arg : args) {
    if (arg == "--json" || arg == "-j") {
      write_json = true;
      continue;
    }
    if (arg == "--no-json") {
      write_json = false;
      continue;
    }
    if (path.empty()) {
      path = arg;
      continue;
    }
    throw std::runtime_error("Unknown or duplicate argument for 'ingest': '" +
                             arg + "'.");
  }
}
