// application/use_cases/WorkflowUseCase.hpp
#ifndef WORKFLOW_USE_CASE_HPP
#define WORKFLOW_USE_CASE_HPP

#include <string>

#include "app_controller/workflow/PathBuilder.hpp"
#include "common/ProcessStats.hpp"
#include "common/Result.hpp"
#include "conversion/modifier/_shared_structures/BillDataStructures.hpp"
#include "conversion/validator/config/BillConfig.hpp"
#include "ports/BillContentReader.hpp"
#include "ports/BillFileEnumerator.hpp"
#include "ports/BillRepository.hpp"
#include "ports/BillSerializer.hpp"

class WorkflowUseCase {
 public:
  WorkflowUseCase(BillConfig validator_config, Config modifier_config,
                  BillContentReader& content_reader,
                  BillFileEnumerator& file_enumerator,
                  BillSerializer& serializer, PathBuilder& path_builder);

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
  PathBuilder& path_builder_;
};

#endif  // WORKFLOW_USE_CASE_HPP
