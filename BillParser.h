#ifndef BILL_PARSER_H
#define BILL_PARSER_H

#include <string>
#include <vector>
#include <regex>

/**
 * @brief 用于存储单条解析记录的结构体。
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

/**
 * @class BillParser
 * @brief 一个用于解析特定格式账单文件的解析器。
 */
class BillParser {
public:
    /**
     * @brief 构造函数。
     */
    BillParser();

    /**
     * @brief 解析指定的账单文件。
     * @param filename 要解析的文件路径。
     * @throws std::runtime_error 如果文件无法打开。
     */
    void parseFile(const std::string& filename);

    /**
     * @brief 获取解析后的所有记录。
     * @return 一个包含 ParsedRecord 的常量向量引用。
     */
    const std::vector<ParsedRecord>& getRecords() const;

    /**
     * @brief 重置解析器的内部状态，以便解析新文件。
     */
    void reset();

private:
    /**
     * @brief 解析单行文本。
     * @param line 要解析的字符串。
     */
    void parseLine(const std::string& line);

    /**
     * @brief 辅助函数，用于去除字符串两端的空白字符。
     * @param s 输入字符串。
     * @return 去除空白后的字符串。
     */
    std::string trim(const std::string& s);

    // 内部状态
    std::vector<ParsedRecord> records_;
    int lineNumber_;
    int parentCounter_;
    int childCounter_;
    int itemCounter_;

    // 上下文追踪
    int currentParentOrder_;
    int currentChildOrder_;

    // 正则表达式定义 (设为静态以节省资源)
    static const std::regex dateRegex_;
    static const std::regex remarkRegex_;
    static const std::regex parentRegex_; // 注意: 需要UTF-8和编译器支持
    static const std::regex childRegex_;
    static const std::regex itemRegex_;
};

#endif // BILL_PARSER_H