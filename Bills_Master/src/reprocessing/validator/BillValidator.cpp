#include "common/pch.h"
#include "BillValidator.h"
#include <iostream>

// 构造函数：通过初始化列表创建 BillConfig 对象
BillValidator::BillValidator(const std::string& config_path)
    : config(std::make_unique<BillConfig>(config_path)),
      processor(), // 默认构造
      result()     // 默认构造
{
    // 构造函数体可以为空，因为所有工作都在初始化列表中完成了
    // 如果配置加载失败，BillConfig的构造函数会抛出异常
    std::cout << "BillValidator initialized successfully, configuration loaded.\n";
}

// validate 方法：封装了整个验证流程
bool BillValidator::validate(const std::string& bill_file_path) {
    std::cout << "\nStarting validation for bill file: " << bill_file_path << "...\n";

    // 调用 processor 来执行验证，传入配置和结果对象
    // 注意 config 是一个智能指针，需要解引用 (*) 来获取其引用的对象
    bool is_valid = processor.validate(bill_file_path, *config, result);

    // 验证完成后，调用 result 对象打印报告
    result.print_report();

    // 返回最终的验证状态
    return is_valid;
}
