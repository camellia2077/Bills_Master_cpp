#if defined(BILLS_CLI_MODULES_ENABLED)
import bill.cli.presentation.features.meta_handler;
import bill.cli.presentation.entry.runtime_context;
import bill.cli.presentation.parsing.cli_request;
import bill.cli.deps.cli_version;
import bill.cli.deps.common_utils;
import bill.cli.deps.core_version;
#else
#include <presentation/features/meta/meta_handler.hpp>
#endif

#include <pch.hpp>
#include <common/Result.hpp>

#include <iostream>

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
