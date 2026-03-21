#ifndef BILLS_IO_HOST_FLOW_SUPPORT_HPP_
#define BILLS_IO_HOST_FLOW_SUPPORT_HPP_

#include <filesystem>
#include <string_view>
#include <vector>

#include "config/config_bundle_service.hpp"
#include "ingest/bill_workflow_service.hpp"
#include "record_template/record_template_types.hpp"

class YearPartitionOutputPathBuilder;

namespace bills::io {

struct HostConfigContext {
  ConfigDocumentBundle documents;
  ValidatedConfigBundle validated;
};

[[nodiscard]] auto LoadValidatedConfigContext(
    const std::filesystem::path& config_dir) -> Result<HostConfigContext>;

[[nodiscard]] auto LoadSourceDocuments(const std::filesystem::path& root_path,
                                       std::string_view extension)
    -> Result<SourceDocumentBatch>;

[[nodiscard]] auto WriteTemplateFiles(
    const std::filesystem::path& output_dir,
    const TemplateGenerationResult& result) -> Result<std::vector<std::string>>;

[[nodiscard]] auto WriteSerializedJsonOutputs(
    const std::vector<BillWorkflowFileResult>& files,
    const YearPartitionOutputPathBuilder& output_path_builder)
    -> Result<std::vector<std::string>>;

}  // namespace bills::io

#endif  // BILLS_IO_HOST_FLOW_SUPPORT_HPP_
