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
    std::string type; // 类型: "date", "remark", "parent", "child", "item", "empty", 或 "unrecognized"
    std::vector<std::string> matches; // 从正则表达式中匹配到的捕获组
};

/**
 * @class LineValidator
 * @brief 一个专门用于验证账单文件单行格式的类。
 * 此类使用正则表达式来确定行的类型并提取其组件。
 */
class LineValidator {
public:
    /**
     * @brief 构造函数，加载配置文件。
     * @param config_path JSON 配置文件的路径。
     */
    explicit LineValidator(const std::string& config_path);

    /**
     * @brief 虚析构函数，允许通过基类指针安全地删除派生类对象。
     */
    virtual ~LineValidator() = default;

    /**
     * @brief 验证单行文本。
     * @param line 要验证的字符串。
     * @return 一个 ValidationResult 结构体，包含行的类型和任何捕获到的数据。
     */
    virtual ValidationResult validate(const std::string& line) const;

    /**
     * @brief 检查给定的子类别对于父类别是否有效。
     * @param parent 父类别的名称。
     * @param child 子类别的名称。
     * @return 如果关系有效，则为 true；否则为 false。
     */
    bool is_valid_child_for_parent(const std::string& parent, const std::string& child) const;

private:
    /**
     * @brief 从文件加载和解析 JSON 配置。
     * @param config_path JSON 配置文件的路径。
     */
    void load_config(const std::string& config_path);

    /**
     * @brief 辅助函数，用于去除字符串两端的空白字符。
     * @param s 输入字符串。
     * @return 去除空白后的字符串。
     */
    std::string trim(const std::string& s) const;

    // 用于存储从 JSON 加载的类别规则
    std::map<std::string, std::vector<std::string>> m_category_rules;
};

#endif // LINE_VALIDATOR_H