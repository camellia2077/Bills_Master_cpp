// app_controller/WorkflowController.hpp
#ifndef WORKFLOW_CONTROLLER_HPP
#define WORKFLOW_CONTROLLER_HPP

#include <string>
#include "nlohmann/json.hpp"
#include "PathBuilder.hpp"
#include "file_handler/FileHandler.hpp" // 直接包含

class WorkflowController {
public:
    explicit WorkflowController(const std::string& config_path, const std::string& modified_output_dir);

    bool handle_validation(const std::string& path);
    bool handle_modification(const std::string& path);
    bool handle_import(const std::string& path, const std::string& db_path);
    bool handle_full_workflow(const std::string& path, const std::string& db_path);

private:
    FileHandler m_file_handler; // 添加 FileHandler 成员
    nlohmann::json m_validator_config;
    nlohmann::json m_modifier_config;
    PathBuilder m_path_builder;
};

#endif // WORKFLOW_CONTROLLER_HPP