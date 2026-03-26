#ifndef CONFIG_CONFIG_BUNDLE_SERVICE_HPP_
#define CONFIG_CONFIG_BUNDLE_SERVICE_HPP_

#include <expected>
#include <string>
#include <vector>

#include "config/modifier_data.hpp"
#include "ingest/validation/bills_config.hpp"
#include "common/validation_issue.hpp"
#include "config/config_document_types.hpp"

struct ConfigFileValidationReport {
  std::string source_kind;
  std::string file_name;
  std::string path;
  bool ok = false;
  std::vector<ValidationIssue> issues;
};

struct ConfigBundleValidationReport {
  bool ok = false;
  std::size_t processed = 0;
  std::size_t success = 0;
  std::size_t failure = 0;
  std::vector<ConfigFileValidationReport> files;
  std::vector<std::string> enabled_export_formats;
  std::vector<std::string> available_export_formats;
};

struct RuntimeConfigBundle {
  BillConfig validator_config = BillConfig(BillValidationRules{});
  Config modifier_config;
};

struct ValidatedConfigBundle {
  RuntimeConfigBundle runtime_config;
  std::vector<std::string> enabled_export_formats;
  std::vector<std::string> available_export_formats;
  ConfigBundleValidationReport report;
};

class ConfigBundleService {
 public:
  [[nodiscard]] static auto Validate(const ConfigDocumentBundle& documents)
      -> std::expected<ValidatedConfigBundle, ConfigBundleValidationReport>;
};

#endif  // CONFIG_CONFIG_BUNDLE_SERVICE_HPP_
