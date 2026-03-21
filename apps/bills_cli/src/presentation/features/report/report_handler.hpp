#ifndef PRESENTATION_FEATURES_REPORT_REPORT_HANDLER_HPP_
#define PRESENTATION_FEATURES_REPORT_REPORT_HANDLER_HPP_

#include "presentation/entry/runtime_context.hpp"
#include "presentation/parsing/cli_request.hpp"

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
