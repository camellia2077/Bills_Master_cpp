#ifndef LINE_VALIDATOR_H
#define LINE_VALIDATOR_H

#include <string>
#include <vector>

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
     * @brief 构造函数。
     */
    LineValidator() = default;

    /**
     * @brief 验证单行文本。
     * @param line 要验证的字符串。
     * @return 一个 ValidationResult 结构体，包含行的类型和任何捕获到的数据。
     */
    ValidationResult validate(const std::string& line) const;

private:
    /**
     * @brief 辅助函数，用于去除字符串两端的空白字符。
     * @param s 输入字符串。
     * @return 去除空白后的字符串。
     */
    std::string trim(const std::string& s) const;
};

#endif // LINE_VALIDATOR_H