// reprocessing/validator/result/ValidationResult.hpp
#ifndef VALIDATION_RESULT_HPP
#define VALIDATION_RESULT_HPP

#include <string>
#include <vector>
#include <iostream>

/**
 * @class ValidationResult
 * @brief 负责收集、存储和报告验证过程中发现的错误和警告。
 */
class ValidationResult {
public:
    /**
     * @brief 添加一条错误信息。
     * @param message 错误描述。
     */
    void add_error(const std::string& message);

    /**
     * @brief 添加一条警告信息。
     * @param message 警告描述。
     */
    void add_warning(const std::string& message);

    /**
     * @brief 检查是否存在任何错误。
     * @return 如果有错误，则返回 true。
     */
    bool has_errors() const;

    /**
     * @brief 将所有收集到的错误和警告打印到控制台。
     * 错误输出到 stderr，警告输出到 stdout。
     */
    void print_report() const;

    /**
     * @brief 清空所有错误和警告，用于重用此对象。
     */
    void clear();

private:
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

#endif // VALIDATION_RESULT_HPP
