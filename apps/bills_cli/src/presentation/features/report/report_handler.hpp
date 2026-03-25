#ifndef PRESENTATION_FEATURES_REPORT_REPORT_HANDLER_HPP_
#define PRESENTATION_FEATURES_REPORT_REPORT_HANDLER_HPP_

#if defined(BILLS_CLI_MODULES_ENABLED)
import bill.cli.presentation.entry.runtime_context;
import bill.cli.presentation.parsing.cli_request;
#else
#include <presentation/entry/runtime_context.hpp>
#include <presentation/parsing/cli_request.hpp>
#endif

namespace bills::cli {

class ReportHandler {
 public:
  explicit ReportHandler(const RuntimeContext& context);

  [[nodiscard]] auto Handle(const ReportRequest& request) const -> bool;

 private:
  const RuntimeContext& context_;
};

}  // namespace bills::cli

#endif  // PRESENTATION_FEATURES_REPORT_REPORT_HANDLER_HPP_
