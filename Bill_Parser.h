#ifndef BILL_PARSER_H
#define BILL_PARSER_H

#include <string>
#include <vector>
#include <functional>
#include "ParsedRecord.h"
#include "LineValidator.h"

/**
 * @class Bill_Parser
 * @brief 一个用于解析账单文件逻辑结构的类，使用 LineValidator 进行格式检查。
 */
class Bill_Parser {
public:
    /**
     * @brief 构造函数，通过依赖注入接收一个验证器。
     * @param validator 一个外部创建的、生命周期长于此解析器的验证器实例。
     */
    explicit Bill_Parser(const LineValidator& validator);

    /**
     * @brief 解析指定的账单文件，并对每条有效记录调用回调函数。
     * @param filename 要解析的文件路径。
     * @param callback 一个用于处理每条解析出的 ParsedRecord 的函数。
     * @throws std::runtime_error 如果文件无法打开。
     */
    void parseFile(const std::string& filename, std::function<void(const ParsedRecord&)> callback);

    /**
     * @brief 重置解析器的内部状态，以便解析新文件。
     */
    void reset();

private:
    /**
     * @brief 解析单行文本，判断其在账单结构中的角色，如果有效则调用回调。
     * @param line 要解析的字符串。
     * @param callback 要调用的回调函数。
     */
    void parseLine(const std::string& line, std::function<void(const ParsedRecord&)> callback);

    // 持有外部验证器的引用，而不是自己创建实例
    const LineValidator& validator_;

    // 用于追踪结构的内部状态
    int lineNumber_;
    int parentCounter_;
    int childCounter_;
    int itemCounter_;

    // 上下文追踪
    int currentParentOrder_;
    int currentChildOrder_;
};

#endif // BILL_PARSER_H