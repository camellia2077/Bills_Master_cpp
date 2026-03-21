#include "bills_io/host_flow_support.hpp"

#include <filesystem>
#include <utility>

#include "bills_io/adapters/config/config_document_parser.hpp"
#include "bills_io/adapters/io/source_document_io.hpp"
#include "bills_io/adapters/io/year_partition_output_path_builder.hpp"

namespace bills::io {

auto LoadValidatedConfigContext(const std::filesystem::path& config_dir)
    -> Result<HostConfigContext> {
  const auto documents = ConfigDocumentParser::ParseFiles(
      config_dir / "validator_config.toml", config_dir / "modifier_config.toml",
      config_dir / "export_formats.toml");
  if (!documents) {
    return std::unexpected(documents.error());
  }

  const auto validated = ConfigBundleService::Validate(*documents);
  if (!validated) {
    return std::unexpected(
        MakeError("Config validation failed.", "HostFlowSupport"));
  }

  return HostConfigContext{
      .documents = *documents,
      .validated = *validated,
  };
}

auto LoadSourceDocuments(const std::filesystem::path& root_path,
                         std::string_view extension) -> Result<SourceDocumentBatch> {
  return SourceDocumentIo::LoadByExtension(root_path, extension);
}

auto WriteTemplateFiles(const std::filesystem::path& output_dir,
                        const TemplateGenerationResult& result)
    -> Result<std::vector<std::string>> {
  SourceDocumentBatch documents;
  documents.reserve(result.templates.size());
  for (const auto& generated_template : result.templates) {
    documents.push_back(SourceDocument{
        .display_path = generated_template.relative_path,
        .text = generated_template.text,
    });
  }
  return SourceDocumentIo::WriteDocuments(output_dir, documents);
}

auto WriteSerializedJsonOutputs(
    const std::vector<BillWorkflowFileResult>& files,
    const YearPartitionOutputPathBuilder& output_path_builder)
    -> Result<std::vector<std::string>> {
  SourceDocumentBatch documents;
  for (const auto& file : files) {
    if (!file.ok || file.serialized_json.empty()) {
      continue;
    }
    documents.push_back(SourceDocument{
        .display_path = output_path_builder
                            .build_output_path(std::filesystem::path(file.display_path))
                            .string(),
        .text = file.serialized_json,
    });
  }
  if (documents.empty()) {
    return std::vector<std::string>{};
  }
  return SourceDocumentIo::WriteDocuments({}, documents);
}

}  // namespace bills::io
