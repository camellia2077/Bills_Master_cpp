// app_controller/WorkflowController.hpp
#ifndef WORKFLOW_CONTROLLER_HPP
#define WORKFLOW_CONTROLLER_HPP

#include <string>

/**
 * @class WorkflowController
 * @brief 负责处理数据预处理和导入数据库的完整工作流。
 */
class WorkflowController {
public:
    explicit WorkflowController(const std::string& config_path, const std::string& modified_output_dir);

    bool handle_validation(const std::string& path);
    bool handle_modification(const std::string& path);
    bool handle_import(const std::string& path);
    bool handle_full_workflow(const std::string& path);

private:
    std::string m_config_path;
    std::string m_modified_output_dir;
};

#endif // WORKFLOW_CONTROLLER_HPP