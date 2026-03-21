#ifndef RECORD_TEMPLATE_IMPORT_PREFLIGHT_SERVICE_HPP_
#define RECORD_TEMPLATE_IMPORT_PREFLIGHT_SERVICE_HPP_

#include <string>
#include <vector>

#include "common/source_document.hpp"
#include "config/config_bundle_service.hpp"
#include "record_template/record_template_types.hpp"

struct ImportPreflightRequest {
  std::string input_label = "inline_batch";
  SourceDocumentBatch documents;
  ConfigBundleValidationReport config_validation;
  RuntimeConfigBundle config_bundle;
  std::vector<std::string> existing_workspace_periods;
  std::vector<std::string> existing_db_periods;
};

struct ImportPreflightResult {
  ConfigBundleValidationReport config_validation;
  RecordPreviewResult record_validation;
  std::size_t success = 0;
  std::size_t failure = 0;
  bool all_clear = false;
  std::vector<std::string> periods;
  std::vector<std::string> duplicate_periods;
  std::vector<std::string> workspace_conflict_periods;
  std::vector<std::string> db_conflict_periods;
  std::vector<ValidationIssue> issues;
};

class ImportPreflightService {
 public:
  [[nodiscard]] static auto Run(const ImportPreflightRequest& request)
      -> RecordTemplateResult<ImportPreflightResult>;
};

#endif  // RECORD_TEMPLATE_IMPORT_PREFLIGHT_SERVICE_HPP_
