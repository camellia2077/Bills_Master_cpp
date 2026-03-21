#include "ingest/bill_workflow_service.hpp"

#include <utility>

#include "ingest/json/bills_json_serializer.hpp"
#include "ingest/pipeline/bills_processing_pipeline.hpp"

namespace {

auto make_success_result(const std::string& display_path, const ParsedBill& bill,
                         bool include_serialized_json) -> BillWorkflowFileResult {
  BillWorkflowFileResult result;
  result.display_path = display_path;
  result.ok = true;
  result.year = bill.year;
  result.month = bill.month;
  result.transaction_count = bill.transactions.size();
  if (include_serialized_json) {
    result.serialized_json = BillJsonSerializer::serialize(bill);
  }
  return result;
}

auto make_failure_result(const std::string& display_path, std::string stage,
                         std::string error) -> BillWorkflowFileResult {
  BillWorkflowFileResult result;
  result.display_path = display_path;
  result.ok = false;
  result.stage = std::move(stage);
  result.error = std::move(error);
  return result;
}

template <typename FileProcessor>
auto process_documents(const SourceDocumentBatch& documents,
                       FileProcessor&& processor) -> BillWorkflowBatchResult {
  BillWorkflowBatchResult batch;
  batch.processed = documents.size();
  batch.files.reserve(documents.size());

  for (const auto& document : documents) {
    auto result = processor(document);
    if (result.ok) {
      ++batch.success;
    } else {
      ++batch.failure;
    }
    batch.files.push_back(std::move(result));
  }

  return batch;
}

}  // namespace

auto BillWorkflowService::Validate(const SourceDocumentBatch& documents,
                                   const RuntimeConfigBundle& config_bundle)
    -> BillWorkflowBatchResult {
  return process_documents(
      documents, [&config_bundle](const SourceDocument& document) {
        BillProcessingPipeline pipeline(config_bundle.validator_config,
                                        config_bundle.modifier_config);
        ParsedBill bill;
        if (!pipeline.validate_and_convert_content(document.text,
                                                   document.display_path, bill)) {
          return make_failure_result(
              document.display_path, pipeline.last_failure_stage(),
              pipeline.last_failure_message().empty()
                  ? "Bill validation failed."
                  : pipeline.last_failure_message());
        }
        return make_success_result(document.display_path, bill, false);
      });
}

auto BillWorkflowService::Convert(const SourceDocumentBatch& documents,
                                  const RuntimeConfigBundle& config_bundle,
                                  bool include_serialized_json)
    -> BillWorkflowBatchResult {
  return process_documents(
      documents, [&config_bundle, include_serialized_json](const SourceDocument& document) {
        BillProcessingPipeline pipeline(config_bundle.validator_config,
                                        config_bundle.modifier_config);
        ParsedBill bill;
        if (!pipeline.validate_and_convert_content(document.text,
                                                   document.display_path, bill)) {
          return make_failure_result(
              document.display_path, pipeline.last_failure_stage(),
              pipeline.last_failure_message().empty()
                  ? "Bill conversion failed."
                  : pipeline.last_failure_message());
        }
        return make_success_result(document.display_path, bill,
                                   include_serialized_json);
      });
}

auto BillWorkflowService::Ingest(const SourceDocumentBatch& documents,
                                 const RuntimeConfigBundle& config_bundle,
                                 BillRepository& repository,
                                 bool include_serialized_json)
    -> BillWorkflowBatchResult {
  return process_documents(
      documents,
      [&config_bundle, &repository,
       include_serialized_json](const SourceDocument& document) {
        BillProcessingPipeline pipeline(config_bundle.validator_config,
                                        config_bundle.modifier_config);
        ParsedBill bill;
        if (!pipeline.validate_and_convert_content(document.text,
                                                   document.display_path, bill)) {
          return make_failure_result(
              document.display_path, pipeline.last_failure_stage(),
              pipeline.last_failure_message().empty()
                  ? "Bill ingest failed."
                  : pipeline.last_failure_message());
        }
        try {
          repository.InsertBill(bill);
        } catch (const std::exception& error) {
          return make_failure_result(document.display_path, "insert_repository",
                                     error.what());
        }
        return make_success_result(document.display_path, bill,
                                   include_serialized_json);
      });
}

auto BillWorkflowService::ImportJson(const SourceDocumentBatch& documents,
                                     BillRepository& repository)
    -> BillWorkflowBatchResult {
  return process_documents(documents, [&repository](const SourceDocument& document) {
    try {
      const ParsedBill bill = BillJsonSerializer::deserialize(document.text);
      repository.InsertBill(bill);
      return make_success_result(document.display_path, bill, false);
    } catch (const std::exception& error) {
      return make_failure_result(document.display_path, "import_json",
                                 error.what());
    }
  });
}
