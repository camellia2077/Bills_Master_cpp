#ifndef TYPST_REPORT_FORMATTER_H
#define TYPST_REPORT_FORMATTER_H

#include <string>
#include "month/_month_data/ReportData.h"

/**
 * @class TypstReportFormatter
 * @brief 将月度报告数据格式化为 Typst 字符串。
 *
 * 该类接收一个 MonthlyReportData 结构体，并生成一个
 * 可以被 Typst 编译器直接编译成 PDF 的源文本字符串。
 */
class TypstReportFormatter {
public:
    /**
     * @brief 接收数据结构并返回格式化的 Typst 字符串。
     * @param data 包含所有月度报告数据的 MonthlyReportData 结构体。
     * @return 一个包含完整 Typst 源码的字符串。
     */
    std::string format_report(const MonthlyReportData& data);

private:
    /**
     * @brief 转义 Typst 特殊字符 (虽然需求较少，但以防万一)。
     * @param input 需要转义的原始字符串。
     * @return 转义后的安全字符串。
     */
    std::string escape_typst(const std::string& input);
};

#endif // TYPST_REPORT_FORMATTER_H