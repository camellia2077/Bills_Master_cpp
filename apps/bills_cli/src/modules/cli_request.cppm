module;
#include "presentation/parsing/cli_request.hpp"

export module bill.cli.presentation.parsing.cli_request;

export namespace bills::cli {
using ::bills::cli::CliRequest;
using ::bills::cli::ConfigAction;
using ::bills::cli::ConfigRequest;
using ::bills::cli::HelpRequest;
using ::bills::cli::MetaAction;
using ::bills::cli::MetaRequest;
using ::bills::cli::ReportAction;
using ::bills::cli::ReportRequest;
using ::bills::cli::TemplateAction;
using ::bills::cli::TemplateRequest;
using ::bills::cli::WorkspaceAction;
using ::bills::cli::WorkspaceRequest;
}
