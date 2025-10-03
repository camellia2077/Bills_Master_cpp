// app_controller/WorkflowController.hpp
#ifndef WORKFLOW_CONTROLLER_HPP
#define WORKFLOW_CONTROLLER_HPP

#include <string>
#include "nlohmann/json.hpp" // 包含json头文件

class WorkflowController {
public:
    /**
     * @brief 构造函数，加载并验证所有需要的配置文件。
     * @param config_path 指向配置目录的路径。
     * @param modified_output_dir 修改后的文件输出目录。
     * @throws std::runtime_error 如果配置文件加载或验证失败。
     */
    explicit WorkflowController(const std::string& config_path, const std::string& modified_output_dir);

    bool handle_validation(const std::string& path);
    bool handle_modification(const std::string& path);
    bool handle_import(const std::string& path, const std::string& db_path);
    bool handle_full_workflow(const std::string& path, const std::string& db_path);

private:
    std::string m_modified_output_dir;
    
    // **新增**: 存储已加载并验证的配置
    nlohmann::json m_validator_config;
    nlohmann::json m_modifier_config;
};

#endif // WORKFLOW_CONTROLLER_HPP