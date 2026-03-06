// controllers/export/ExportController.hpp
#ifndef EXPORT_CONTROLLER_HPP
#define EXPORT_CONTROLLER_HPP

#include <map>
#include <string>
#include <vector>

/**
 * @class ExportController
 * @brief 负责处理所有类型的报告导出任务。
 */
class ExportController {
 public:
  explicit ExportController(
      std::string db_path, std::string export_base_dir,
      const std::map<std::string, std::string>& format_folder_names);

  bool handle_export(const std::string& type,
                     const std::vector<std::string>& values,
                     const std::string& format_str,
                     const std::string& export_pipeline);

 private:
  std::string m_db_path;
  std::string m_export_base_dir;
  std::map<std::string, std::string> m_format_folder_names;
};

#endif  // EXPORT_CONTROLLER_HPP
