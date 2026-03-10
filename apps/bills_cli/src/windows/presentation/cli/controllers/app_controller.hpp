// windows/presentation/cli/controllers/app_controller.hpp
#ifndef WINDOWS_PRESENTATION_CLI_CONTROLLERS_APP_CONTROLLER_H_
#define WINDOWS_PRESENTATION_CLI_CONTROLLERS_APP_CONTROLLER_H_

#include <memory>
#include <map>
#include <set>
#include <string>
#include <vector>

class ExportController;
class WorkflowController;

struct RecordTemplateOptions {
  std::string period;
  std::string start_period;
  std::string end_period;
  std::string start_year;
  std::string end_year;
  std::string output_dir;
};

class AppController {
 public:
  explicit AppController(std::string db_path = "",
                         const std::string& config_path = "./config",
                         std::string modified_output_dir = "");
  ~AppController();

  bool handle_validation(const std::string& path);
  bool handle_convert(const std::string& path);
  bool handle_ingest(const std::string& path, bool write_json);
  bool handle_import(const std::string& path);
  bool handle_full_workflow(const std::string& path);
  bool handle_record_template(const RecordTemplateOptions& options);
  bool handle_record_preview(const std::string& path);
  bool handle_record_list(const std::string& path);
  bool handle_config_inspect();
  bool handle_export(const std::string& type,
                     const std::vector<std::string>& values,
                     const std::string& format_str = "md",
                     const std::string& export_pipeline = "model-first");
  [[nodiscard]] auto list_enabled_export_formats() const
      -> std::vector<std::string>;

  static void display_version();
  static auto display_notices(bool raw_json) -> bool;

 private:
  auto load_enabled_formats(const std::string& config_path) -> std::set<std::string>;
  auto normalize_format(std::string format_name) const -> std::string;
  auto normalize_export_pipeline(std::string pipeline_name) const
      -> std::string;
  auto is_export_format_available(const std::string& type,
                                  const std::vector<std::string>& values,
                                  const std::string& format_str,
                                  const std::string& export_pipeline) const
      -> bool;

  std::string m_db_path;
  std::string m_config_path;
  std::string m_modified_output_dir;
  std::set<std::string> m_enabled_formats;
  std::string m_export_base_dir;
  std::map<std::string, std::string> m_format_folder_names;
  std::unique_ptr<WorkflowController> m_workflow_controller;
  std::unique_ptr<ExportController> m_export_controller;
};

#endif  // WINDOWS_PRESENTATION_CLI_CONTROLLERS_APP_CONTROLLER_H_
