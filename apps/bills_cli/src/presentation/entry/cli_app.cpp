#include "presentation/entry/cli_app.hpp"

#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include "common/Result.hpp"
#include "presentation/entry/runtime_context.hpp"
#include "presentation/features/config/config_handler.hpp"
#include "presentation/features/meta/meta_handler.hpp"
#include "presentation/features/report/report_handler.hpp"
#include "presentation/features/template/template_handler.hpp"
#include "presentation/features/workspace/workspace_handler.hpp"
#include "presentation/output/help_text.hpp"
#include "presentation/parsing/cli_parser.hpp"

namespace bills::cli {
namespace {

auto ExecuteRequest(const CliRequest& request, const RuntimeContext& context) -> bool {
  return std::visit(
      [&](const auto& typed_request) -> bool {
        using T = std::decay_t<decltype(typed_request)>;
        if constexpr (std::is_same_v<T, HelpRequest>) {
          return true;
        } else if constexpr (std::is_same_v<T, WorkspaceRequest>) {
          WorkspaceHandler handler(context);
          return handler.Handle(typed_request);
        } else if constexpr (std::is_same_v<T, ReportRequest>) {
          ReportHandler handler(context);
          return handler.Handle(typed_request);
        } else if constexpr (std::is_same_v<T, TemplateRequest>) {
          TemplateHandler handler(context);
          return handler.Handle(typed_request);
        } else if constexpr (std::is_same_v<T, ConfigRequest>) {
          ConfigHandler handler(context);
          return handler.Handle(typed_request);
        } else if constexpr (std::is_same_v<T, MetaRequest>) {
          MetaHandler handler(context);
          return handler.Handle(typed_request);
        }
        return false;
      },
      request);
}

}  // namespace

auto CliApp::Run(int argc, char* argv[]) -> int {
  try {
    std::vector<std::string> args;
    args.reserve(argc > 0 ? static_cast<std::size_t>(argc - 1) : 0U);
    for (int index = 1; index < argc; ++index) {
      args.emplace_back(argv[index]);
    }

    const auto request = ParseCliRequest(args);
    if (!request) {
      std::cerr << "Error: " << FormatError(request.error()) << '\n' << '\n';
      PrintHelp(std::cerr, argc > 0 ? argv[0] : "bill_master_cli");
      return 1;
    }

    if (std::holds_alternative<HelpRequest>(*request)) {
      PrintHelp(std::cout, argc > 0 ? argv[0] : "bill_master_cli");
      return 0;
    }

    const RuntimeContext context = BuildRuntimeContext();
    return ExecuteRequest(*request, context) ? 0 : 1;
  } catch (const std::exception& error) {
    std::cerr << "Critical Error: " << error.what() << '\n';
    return 1;
  }
}

}  // namespace bills::cli
