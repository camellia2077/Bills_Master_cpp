// controllers/AppController.hpp
#ifndef APP_CONTROLLER_HPP
#define APP_CONTROLLER_HPP

#include <memory>
#include <map>
#include <set>
#include <string>
#include <vector>

class ExportController;
class WorkflowController;

class AppController {
 public:
  // [修改] 更新默认路径，将它们指向 output/ 目录
  explicit AppController(std::string db_path = "output/bills.sqlite3",
                         const std::string& config_path = "./config",
                         std::string modified_output_dir = "output/txt2josn");
  ~AppController();

  bool handle_validation(const std::string& path);
  bool handle_convert(const std::string& path);
  bool handle_ingest(const std::string& path, bool write_json);
  bool handle_import(const std::string& path);
  bool handle_full_workflow(const std::string& path);
  bool handle_export(const std::string& type,
                     const std::vector<std::string>& values,
                     const std::string& format_str = "md",
                     const std::string& export_pipeline = "model-first");

  static void display_version();

 private:
  auto load_enabled_formats(const std::string& config_path) -> std::set<std::string>;
  auto normalize_format(std::string format_name) const -> std::string;
  auto normalize_export_pipeline(std::string pipeline_name) const
      -> std::string;
  auto infer_export_requirements(const std::string& type,
                                 const std::vector<std::string>& values) const
      -> std::pair<bool, bool>;
  auto is_export_format_available(const std::string& type,
                                  const std::vector<std::string>& values,
                                  const std::string& format_str,
                                  const std::string& export_pipeline) const
      -> bool;

  std::string m_db_path;
  std::string m_config_path;
  std::string m_modified_output_dir;
  std::vector<std::string> m_plugin_files;
  std::set<std::string> m_enabled_formats;
  std::set<std::string> m_month_formats_available;
  std::set<std::string> m_year_formats_available;
  std::string m_export_base_dir;
  std::map<std::string, std::string> m_format_folder_names;
  std::unique_ptr<WorkflowController> m_workflow_controller;
  std::unique_ptr<ExportController> m_export_controller;
};

#endif  // APP_CONTROLLER_HPP
