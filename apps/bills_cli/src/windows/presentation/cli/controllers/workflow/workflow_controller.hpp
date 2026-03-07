// windows/presentation/cli/controllers/workflow/workflow_controller.hpp
#ifndef WINDOWS_PRESENTATION_CLI_CONTROLLERS_WORKFLOW_WORKFLOW_CONTROLLER_H_
#define WINDOWS_PRESENTATION_CLI_CONTROLLERS_WORKFLOW_WORKFLOW_CONTROLLER_H_

#include <memory>
#include <optional>
#include <string>

#include "path_builder.hpp"

class BillContentReader;
class BillFileEnumerator;
class BillSerializer;
class ConfigProvider;
class WorkflowUseCase;

class WorkflowController {
 public:
  explicit WorkflowController(const std::string& config_path,
                              const std::string& modified_output_dir);
  ~WorkflowController();

  bool handle_validation(const std::string& path);
  bool handle_convert(const std::string& path);
  bool handle_ingest(const std::string& path, const std::string& db_path,
                     bool write_json);
  bool handle_import(const std::string& path, const std::string& db_path);
  bool handle_full_workflow(const std::string& path,
                            const std::string& db_path);

 private:
  bool ensure_initialized() const;

  PathBuilder m_path_builder;
  std::unique_ptr<BillContentReader> m_content_reader;
  std::unique_ptr<BillFileEnumerator> m_file_enumerator;
  std::unique_ptr<BillSerializer> m_serializer;
  std::unique_ptr<ConfigProvider> m_config_provider;
  std::unique_ptr<WorkflowUseCase> m_use_case;
  std::optional<std::string> m_init_error;
};

#endif  // WINDOWS_PRESENTATION_CLI_CONTROLLERS_WORKFLOW_WORKFLOW_CONTROLLER_H_
