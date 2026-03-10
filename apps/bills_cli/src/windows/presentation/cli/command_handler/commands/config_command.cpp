#include "config_command.hpp"

#include <stdexcept>

#include "controllers/app_controller.hpp"

auto ConfigCommand::execute(const std::vector<std::string>& args,
                            AppController& controller) -> bool {
  if (args.empty()) {
    throw std::runtime_error("Missing sub-command for 'config'. Use 'inspect'.");
  }
  if (args.size() != 1U || args[0] != "inspect") {
    throw std::runtime_error(
        "Unknown or invalid sub-command for 'config'. Use 'config inspect'.");
  }
  return controller.handle_config_inspect();
}
