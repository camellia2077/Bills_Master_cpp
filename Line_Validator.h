// Line_Validator.h
#ifndef LINE_VALIDATOR_H
#define LINE_VALIDATOR_H

#include <string>
#include <vector>
#include <map>

/**
 * @struct ValidationResult
 * @brief 保存单行验证的结果，包含其类型和捕获到的数据。
 */
struct ValidationResult {
    // 类型: "date", "remark", "parent", "child", "item", "empty", "unrecognized", 或 "invalid_parent"
    std::string type; 
    std::vector<std::string> matches; // 从正则表达式中匹配到的捕获组
};

/**
 * @class LineValidator
 * @brief 一个专门用于验证账单文件单行格式的类。
 * 此类使用正则表达式来确定行的类型并提取其组件。
 */
class LineValidator {
public:
    explicit LineValidator(const std::string& config_path);
    virtual ~LineValidator() = default;
    virtual ValidationResult validate(const std::string& line) const;

    /**
     * @brief 检查给定的父类别是否在配置文件中定义。
     * @param parent 要检查的父类别的名称。
     * @return 如果父类别有效，则为 true；否则为 false。
     */
    bool is_valid_parent(const std::string& parent) const;
    
    bool is_valid_child_for_parent(const std::string& parent, const std::string& child) const;

private:
    void load_config(const std::string& config_path);
    std::string trim(const std::string& s) const;
    std::map<std::string, std::vector<std::string>> m_category_rules;
};

#endif // LINE_VALIDATOR_H