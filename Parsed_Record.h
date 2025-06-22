#ifndef PARSED_RECORD_H
#define PARSED_RECORD_H

#include <string> // ParsedRecord 使用了 std::string

// 注意：ParsedRecord 自身不使用 std::vector，
// 但依赖它的模块通常会把它放进 vector 里，
// 所以在这里包含 <vector> 也是一种常见的做法，但不是必须的。

/**
 * @brief 用于存储单条解析记录的结构体。
 * 这是一个在解析器和数据库插入器之间传递数据的数据传输对象(DTO)。
 */
struct ParsedRecord {
    std::string type;       // 类型: "date", "remark", "parent", "child", "item"
    int lineNumber;         // 在文件中的原始行号
    int order = 0;          // 在其类别内的顺序编号 (从1开始)
    int parentOrder = 0;    // 所属父类别的顺序编号
    int childOrder = 0;     // 所属子类别的顺序编号

    // 内容字段
    std::string content;      // 用于 date, remark, parent, child
    double amount = 0.0;      // 用于 item
    std::string description;  // 用于 item
};

#endif // PARSED_RECORD_H