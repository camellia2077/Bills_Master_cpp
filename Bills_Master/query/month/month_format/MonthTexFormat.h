#ifndef TEX_FORMAT_H
#define TEX_FORMAT_H

#include <string>
#include "month/_month_data/ReportData.h"

/**
 * @class MonthTexFormat
 * @brief 将月度报告数据格式化为 LaTeX 字符串。
 *
 * 该类接收一个 MonthlyReportData 结构体，并生成一个完整的、
 * 可以被编译的 LaTeX 文档字符串，用于创建 PDF 报告。
 */
class MonthTexFormat {
public:
    /**
     * @brief 接收数据结构并返回格式化的 LaTeX 字符串。
     * @param data 包含所有月度报告数据的 MonthlyReportData 结构体。
     * @return 一个包含完整 LaTeX 源码的字符串。
     */
    std::string format_report(const MonthlyReportData& data);

private:
    /**
     * @brief 转义 LaTeX 特殊字符。
     * @param input 需要转义的原始字符串。
     * @return 转义后的安全字符串。
     */
    std::string escape_latex(const std::string& input);
};

#endif // TEX_FORMAT_H