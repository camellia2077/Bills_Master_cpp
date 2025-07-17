#ifndef BILL_VALIDATOR_H
#define BILL_VALIDATOR_H

#include "BillConfig.h"
#include "BillProcessor.h"
#include "ValidationResult.h"
#include <string>
#include <memory> // 用于 std::unique_ptr

/**
 * @class BillValidator
 * @brief 一个高层次的封装类（Facade），用于简化账单验证过程。
 *
 * 此类封装了 BillConfig, BillProcessor, 和 ValidationResult 的所有复杂性，
 * 为用户提供一个简单、统一的接口。
 */
class BillValidator {
public:
    /**
     * @brief 构造函数，加载并准备验证所需的配置。
     * @param config_path JSON 配置文件的路径。
     * @throws std::runtime_error 如果配置文件加载或解析失败。
     */
    explicit BillValidator(const std::string& config_path);

    /**
     * @brief 验证给定的账单文件，并自动打印报告。
     * @param bill_file_path 要验证的账单文件的路径。
     * @return 如果没有发现任何“错误”，则返回 true，否则返回 false。
     */
    bool validate(const std::string& bill_file_path);

private:
    // 使用智能指针管理 BillConfig 的生命周期，清晰地表达所有权
    std::unique_ptr<BillConfig> config;
    
    // Processor 和 Result 可以是普通成员对象
    BillProcessor processor;
    ValidationResult result;
};

#endif // BILL_VALIDATOR_H
