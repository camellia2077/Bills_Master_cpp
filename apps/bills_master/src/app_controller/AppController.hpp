// app_controller/AppController.hpp
#ifndef APP_CONTROLLER_HPP
#define APP_CONTROLLER_HPP

#include <map>
#include <string>
#include <vector>

class AppController {
 public:
  // [修改] 更新默认路径，将它们指向 output/ 目录
  explicit AppController(std::string db_path = "output/bills.sqlite3",
                         const std::string& config_path = "./config",
                         std::string modified_output_dir = "output/txt2josn");

  bool handle_validation(const std::string& path);
  bool handle_modification(const std::string& path);
  bool handle_convert(const std::string& path);
  bool handle_ingest(const std::string& path, bool write_json);
  bool handle_import(const std::string& path);
  bool handle_full_workflow(const std::string& path);
  bool handle_export(const std::string& type,
                     const std::vector<std::string>& values,
                     const std::string& format_str = "md");

  static void display_version();

 private:
  std::string m_db_path;
  std::string m_config_path;
  std::string m_modified_output_dir;
  std::vector<std::string> m_plugin_files;
  std::string m_export_base_dir;
  std::map<std::string, std::string> m_format_folder_names;
};

#endif  // APP_CONTROLLER_HPP
