// reprocessing/validator/verifier/BillFormatVerifier.hpp

#ifndef BILL_FORMAT_VERIFIER_HPP
#define BILL_FORMAT_VERIFIER_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include <fstream> //
// 前向声明
class BillConfig;
class ValidationResult;

class BillFormatVerifier {
public:
    bool validate(const std::string& bill_file_path, const BillConfig& config, ValidationResult& result);

private:
    enum class State { EXPECT_PARENT, EXPECT_SUB, EXPECT_CONTENT };

    // --- 成员变量: 用于在验证过程中保存状态 ---
    State m_current_state;
    std::string m_current_parent;
    std::string m_current_sub;
    int m_line_num;
    const BillConfig* m_config;
    ValidationResult* m_result;
    std::unordered_map<std::string, std::unordered_map<std::string, int>> m_bill_structure;

    // --- 高层逻辑块 (从 validate 中提炼) ---
    void _initialize_validation(const BillConfig& config, ValidationResult& result);
    bool _process_file_header(std::ifstream& file);
    void _process_file_body(std::ifstream& file);
    void _finalize_validation();

    // --- 中层逻辑 (原有的辅助函数) ---
    void _process_line(const std::string& line);
    
    // --- 底层状态处理 (原有的状态处理函数) ---
    void _handle_parent_state(const std::string& line);
    void _handle_sub_state(const std::string& line);
    void _handle_content_state(const std::string& line);
};

#endif // BILL_FORMAT_VERIFIER_HPP