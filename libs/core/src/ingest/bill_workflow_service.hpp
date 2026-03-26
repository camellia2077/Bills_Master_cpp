#ifndef INGEST_BILL_WORKFLOW_SERVICE_HPP_
#define INGEST_BILL_WORKFLOW_SERVICE_HPP_

#include <string>
#include <vector>

#include "common/source_document.hpp"
#include "common/validation_issue.hpp"
#include "config/config_bundle_service.hpp"
#include "domain/bill/bill_record.hpp"
#include "ports/bills_repository.hpp"

struct BillWorkflowFileResult {
  std::string display_path;
  bool ok = false;
  std::string stage;
  std::string stage_group;
  std::string error;
  std::vector<ValidationIssue> issues;
  int year = 0;
  int month = 0;
  std::size_t transaction_count = 0;
  std::string serialized_json;
};

struct BillWorkflowBatchResult {
  std::size_t processed = 0;
  std::size_t success = 0;
  std::size_t failure = 0;
  std::vector<BillWorkflowFileResult> files;
};

class BillWorkflowService {
 public:
  [[nodiscard]] static auto Validate(const SourceDocumentBatch& documents,
                                     const RuntimeConfigBundle& config_bundle)
      -> BillWorkflowBatchResult;

  [[nodiscard]] static auto Convert(const SourceDocumentBatch& documents,
                                    const RuntimeConfigBundle& config_bundle,
                                    bool include_serialized_json)
      -> BillWorkflowBatchResult;

  [[nodiscard]] static auto Ingest(const SourceDocumentBatch& documents,
                                   const RuntimeConfigBundle& config_bundle,
                                   BillRepository& repository,
                                   bool include_serialized_json)
      -> BillWorkflowBatchResult;

  [[nodiscard]] static auto ImportJson(const SourceDocumentBatch& documents,
                                       BillRepository& repository)
      -> BillWorkflowBatchResult;
};

#endif  // INGEST_BILL_WORKFLOW_SERVICE_HPP_
