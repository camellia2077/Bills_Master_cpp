// application/use_cases/workflow_use_case.hpp
#ifndef WORKFLOW_USE_CASE_HPP
#define WORKFLOW_USE_CASE_HPP

#include <filesystem>
#include <string>

#include "common/process_stats.hpp"
#include "common/result.hpp"
#include "billing/conversion/modifier/_shared_structures/bills_data_structures.hpp"
#include "billing/conversion/validator/config/bills_config.hpp"
#include "ports/bills_content_reader.hpp"
#include "ports/bills_file_enumerator.hpp"
#include "ports/output_path_builder.hpp"
#include "ports/bills_repository.hpp"
#include "ports/bills_serializer.hpp"

class WorkflowUseCase {
 public:
  WorkflowUseCase(BillConfig validator_config, Config modifier_config,
                  BillContentReader& content_reader,
                  BillFileEnumerator& file_enumerator,
                  BillSerializer& serializer,
                  OutputPathBuilder& output_path_builder);

  [[nodiscard]] auto Validate(const std::string& path) -> Result<ProcessStats>;
  [[nodiscard]] auto Convert(const std::string& path) -> Result<ProcessStats>;
  [[nodiscard]] auto Ingest(const std::string& path, BillRepository& repository,
                            bool write_json) -> Result<ProcessStats>;
  [[nodiscard]] auto Import(const std::string& path, BillRepository& repository)
      -> Result<ProcessStats>;
  [[nodiscard]] auto FullWorkflow(const std::string& path,
                                  BillRepository& repository)
      -> Result<ProcessStats>;

 private:
  BillConfig validator_config_;
  Config modifier_config_;
  BillContentReader& content_reader_;
  BillFileEnumerator& file_enumerator_;
  BillSerializer& serializer_;
  OutputPathBuilder& output_path_builder_;
};

#endif  // WORKFLOW_USE_CASE_HPP


