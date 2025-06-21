#ifndef BILL_PARSER_H
#define BILL_PARSER_H

#include <string>
#include <vector>
#include <functional>
#include "Parsed_Record.h"
#include "Line_Validator.h"

class Bill_Parser {
public:
    explicit Bill_Parser(const LineValidator& validator);
    void parseFile(const std::string& filename, std::function<void(const ParsedRecord&)> callback);
    void reset();

private:
    void parseLine(const std::string& line, std::function<void(const ParsedRecord&)> callback);

    const LineValidator& validator_;
    int lineNumber_;
    int parentCounter_;
    int childCounter_;
    int itemCounter_;

    int currentParentOrder_;
    int currentChildOrder_;
    std::string currentParentName_; // 新追踪当前父项目名称
    std::string currentFilename_; // 存储当前文件名
};

#endif // BILL_PARSER_H