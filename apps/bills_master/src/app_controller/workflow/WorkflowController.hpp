// app_controller/workflow/WorkflowController.hpp
#ifndef WORKFLOW_CONTROLLER_HPP
#define WORKFLOW_CONTROLLER_HPP

#include <memory>
#include <optional>
#include <string>

#include "PathBuilder.hpp"
#include "application/use_cases/WorkflowUseCase.hpp"
#include "common/Result.hpp"
#include "file_handler/FileHandler.hpp"  // 直接包含
#include "ports/BillContentReader.hpp"
#include "ports/BillFileEnumerator.hpp"
#include "ports/BillRepository.hpp"
#include "ports/BillSerializer.hpp"
#include "ports/ConfigProvider.hpp"

class WorkflowController {
 public:
  explicit WorkflowController(const std::string& config_path,
                              const std::string& modified_output_dir);

  bool handle_validation(const std::string& path);
  bool handle_modification(const std::string& path);
  bool handle_convert(const std::string& path);
  bool handle_ingest(const std::string& path, const std::string& db_path,
                     bool write_json);
  bool handle_import(const std::string& path, const std::string& db_path);
  bool handle_full_workflow(const std::string& path,
                            const std::string& db_path);

 private:
  bool ensure_initialized() const;

  FileHandler m_file_handler;  // 添加 FileHandler 成员
  PathBuilder m_path_builder;
  std::unique_ptr<BillContentReader> m_content_reader;
  std::unique_ptr<BillFileEnumerator> m_file_enumerator;
  std::unique_ptr<BillSerializer> m_serializer;
  std::unique_ptr<ConfigProvider> m_config_provider;
  std::unique_ptr<WorkflowUseCase> m_use_case;
  std::optional<Error> m_init_error;
};

#endif  // WORKFLOW_CONTROLLER_HPP
