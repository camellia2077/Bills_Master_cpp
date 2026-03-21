#include "presentation/features/meta/meta_handler.hpp"

#include <iostream>

#include "common/cli_version.hpp"
#include "common/common_utils.hpp"
#include "common/version.hpp"

namespace terminal = common::terminal;

namespace bills::cli {

MetaHandler::MetaHandler(const RuntimeContext& context) : context_(context) {}

auto MetaHandler::Handle(const MetaRequest& request) const -> bool {
  switch (request.action) {
    case MetaAction::kVersion:
      std::cout << "Core Version: " << bills::core::version::kVersion << '\n';
      std::cout << "Core Last Updated: "
                << bills::core::version::kLastUpdated << '\n';
      std::cout << "CLI Version: " << bills::cli::version::kVersion << '\n';
      std::cout << "CLI Last Updated: " << bills::cli::version::kLastUpdated
                << '\n';
      return true;

    case MetaAction::kNotices: {
      const auto notices = ReadBundledNotices(context_, request.raw_json);
      if (!notices) {
        std::cerr << terminal::kRed << "Error: " << terminal::kReset
                  << FormatError(notices.error()) << '\n';
        return false;
      }
      std::cout << *notices;
      return true;
    }
  }

  return false;
}

}  // namespace bills::cli
