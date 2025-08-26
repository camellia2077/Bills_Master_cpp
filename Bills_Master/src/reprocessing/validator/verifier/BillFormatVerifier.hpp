#ifndef BILL_PROCESSOR_H
#define BILL_PROCESSOR_H

#include <string>
#include <unordered_map>
#include <vector>

// 前向声明，避免在头文件中包含完整的类定义，减少编译依赖
class BillConfig;
class ValidationResult;

/**
 * @class BillFormatVerifier
 * @brief 负责执行账单文件的核心验证逻辑。
 *
 * 此类包含状态机，逐行读取和解析文件，并使用 BillConfig 进行验证，
 * 将结果存入 ValidationResult。
 */
class BillFormatVerifier {
public:
    /**
     * @brief 对给定的账单文件执行完整的验证流程。
     * @param bill_file_path 要验证的账单文件的路径。
     * @param config 包含验证规则的配置对象。
     * @param result 用于存储错误和警告的结果对象。
     * @return 如果验证过程中未发现错误，则返回 true。
     */
    bool validate(const std::string& bill_file_path, const BillConfig& config, ValidationResult& result);

private:
    // 内部状态机定义
    enum class State {
        EXPECT_PARENT,
        EXPECT_SUB,
        EXPECT_CONTENT
    };

    // 用于后期检查的数据结构：父标题 -> (子标题 -> 内容行数)
    std::unordered_map<std::string, std::unordered_map<std::string, int>> bill_structure;

    // --- 验证步骤的私有辅助函数 ---
    void _reset_state();
    bool _validate_date_and_remark(std::ifstream& file, int& line_num, ValidationResult& result);
    void _process_line(const std::string& line, int line_num, State& current_state, std::string& current_parent, std::string& current_sub, const BillConfig& config, ValidationResult& result);
    void _post_validation_checks(ValidationResult& result);

    // --- 状态处理函数 ---
    void _handle_parent_state(const std::string& line, int line_num, State& current_state, std::string& current_parent, const BillConfig& config, ValidationResult& result);
    void _handle_sub_state(const std::string& line, int line_num, State& current_state, std::string& current_parent, std::string& current_sub, const BillConfig& config, ValidationResult& result);
    void _handle_content_state(const std::string& line, int line_num, State& current_state, std::string& current_parent, std::string& current_sub, const BillConfig& config, ValidationResult& result);
};

#endif // BILL_PROCESSOR_H
