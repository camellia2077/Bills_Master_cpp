// common/ParsedRecord.h
#ifndef PARSED_RECORD_H
#define PARSED_RECORD_H

#include <string>
#include <vector>

/**
 * @brief 定义了用于在解析器和数据库模块间传递数据的核心结构体。
 * 这是一个在不同模块间共享的数据传输对象(DTO)。
 */
struct ParsedRecord {
    std::string date;
    std::string parent_category;
    std::string child_category;
    std::string item_description;
    double amount;
};

#endif // PARSED_RECORD_H