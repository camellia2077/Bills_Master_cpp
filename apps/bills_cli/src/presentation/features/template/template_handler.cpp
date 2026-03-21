#include "presentation/features/template/template_handler.hpp"

#include <iostream>
#include <sstream>

#include "bills_io/host_flow_support.hpp"
#include "common/common_utils.hpp"
#include "record_template/record_template_service.hpp"

namespace terminal = common::terminal;

namespace bills::cli {
namespace {

auto JoinPeriods(const std::vector<std::string>& periods) -> std::string {
  if (periods.empty()) {
    return "(none)";
  }

  std::ostringstream output;
  for (std::size_t index = 0; index < periods.size(); ++index) {
    if (index != 0U) {
      output << ", ";
    }
    output << periods[index];
  }
  return output.str();
}

}  // namespace

TemplateHandler::TemplateHandler(const RuntimeContext& context) : context_(context) {}

auto TemplateHandler::Handle(const TemplateRequest& request) const -> bool {
  const auto validated_documents =
      bills::io::LoadValidatedConfigContext(context_.config_dir);
  if (!validated_documents) {
    std::cerr << terminal::kRed << "Error: " << terminal::kReset
              << FormatError(validated_documents.error()) << '\n';
    return false;
  }

  switch (request.action) {
    case TemplateAction::kGenerate: {
      const auto layout = RecordTemplateService::BuildOrderedTemplateLayout(
          validated_documents->documents.validator);
      if (!layout) {
        std::cerr << terminal::kRed << "Error: " << terminal::kReset
                  << FormatRecordTemplateError(layout.error()) << '\n';
        return false;
      }

      TemplateGenerationRequest generation_request;
      generation_request.period = request.period;
      generation_request.start_period = request.start_period;
      generation_request.end_period = request.end_period;
      generation_request.start_year = request.start_year;
      generation_request.end_year = request.end_year;
      generation_request.layout = *layout;

      const auto result =
          RecordTemplateService::GenerateTemplates(generation_request);
      if (!result) {
        std::cerr << terminal::kRed << "Error: " << terminal::kReset
                  << FormatRecordTemplateError(result.error()) << '\n';
        return false;
      }
      if (result->templates.empty()) {
        std::cerr << terminal::kYellow << "Warning: " << terminal::kReset
                  << "No templates were generated." << '\n';
        return false;
      }

      if (request.output_dir.has_value()) {
        const auto write_result =
            bills::io::WriteTemplateFiles(*request.output_dir, *result);
        if (!write_result) {
          std::cerr << terminal::kRed << "Error: " << terminal::kReset
                    << FormatError(write_result.error()) << '\n';
          return false;
        }
        std::cout << "Generated " << result->templates.size()
                  << " template file(s)." << '\n';
        for (const auto& written_path : *write_result) {
          std::cout << "  " << written_path << '\n';
        }
        return true;
      }

      for (std::size_t index = 0; index < result->templates.size(); ++index) {
        const auto& generated_template = result->templates[index];
        std::cout << "=== " << generated_template.period << " ("
                  << generated_template.relative_path << ") ===" << '\n';
        std::cout << generated_template.text << '\n';
        if (index + 1U < result->templates.size()) {
          std::cout << '\n';
        }
      }
      return true;
    }

    case TemplateAction::kPreview: {
      const auto source_documents =
          bills::io::LoadSourceDocuments(request.input_path, ".txt");
      if (!source_documents) {
        std::cerr << terminal::kRed << "Error: " << terminal::kReset
                  << FormatError(source_documents.error()) << '\n';
        return false;
      }
      const auto result = RecordTemplateService::PreviewRecords(
          *source_documents, validated_documents->validated.runtime_config,
          request.input_path.string());
      if (!result) {
        std::cerr << terminal::kRed << "Error: " << terminal::kReset
                  << FormatRecordTemplateError(result.error()) << '\n';
        return false;
      }
      for (const auto& preview_file : result->files) {
        if (preview_file.ok) {
          std::cout << "[OK] " << preview_file.path << " | period="
                    << preview_file.period << " | txns="
                    << preview_file.transaction_count << " | income="
                    << preview_file.total_income << " | expense="
                    << preview_file.total_expense << " | balance="
                    << preview_file.balance << '\n';
          continue;
        }
        std::cerr << "[FAIL] " << preview_file.path << " | "
                  << preview_file.error << '\n';
      }
      std::cout << "Processed: " << result->processed
                << ", Success: " << result->success
                << ", Failure: " << result->failure << '\n';
      std::cout << "Periods: " << JoinPeriods(result->periods) << '\n';
      return result->failure == 0U;
    }

    case TemplateAction::kListPeriods: {
      const auto source_documents =
          bills::io::LoadSourceDocuments(request.input_path, ".txt");
      if (!source_documents) {
        std::cerr << terminal::kRed << "Error: " << terminal::kReset
                  << FormatError(source_documents.error()) << '\n';
        return false;
      }
      const auto result = RecordTemplateService::ListPeriods(
          *source_documents, request.input_path.string());
      if (!result) {
        std::cerr << terminal::kRed << "Error: " << terminal::kReset
                  << FormatRecordTemplateError(result.error()) << '\n';
        return false;
      }
      for (const auto& period : result->periods) {
        std::cout << period << '\n';
      }
      for (const auto& invalid_file : result->invalid_files) {
        std::cerr << "[INVALID] " << invalid_file.path << " | "
                  << invalid_file.error << '\n';
      }
      return result->invalid == 0U;
    }
  }

  return false;
}

}  // namespace bills::cli
