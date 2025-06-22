// Bill_Parser.h
#ifndef BILL_PARSER_H
#define BILL_PARSER_H

#include <string>
#include <vector>
#include <functional>
#include "Line_Validator.h"
#include "ParsedRecord.h"


class Bill_Parser {
public:
    explicit Bill_Parser(LineValidator& validator);

    /**
     * @brief 解析一个账单文件，收集所有记录和错误。
     * @param file_path 要解析的文件的路径。
     * @param handler 一个函数，用于处理解析出的有效记录。
     * @return 一个字符串向量，包含文件中发现的所有校验错误。如果为空，则文件有效。
     */
    std::vector<std::string> parseFile(const std::string& file_path,
                                       std::function<void(const ParsedRecord&)> handler);

    /**
     * @brief 重置解析器的内部状态。
     */
    void reset();

private:
    // 定义解析器的状态
    enum class ParserState {
        EXPECTING_DATE,         // 正在等待DATE行
        EXPECTING_REMARK,       // 在DATE行之后，正在等待REMARK行
        PROCESSING_CONTENT      // 在REMARK行之后，正在处理账单内容
    };

    LineValidator& m_validator;
    // 用于维持解析状态的状态机变量
    std::string m_current_date;
    std::string m_current_parent;
    std::string m_current_child;
    int m_line_number;
    ParserState m_state; // 使用状态机替代原有的布尔标志
};

#endif // BILL_PARSER_H