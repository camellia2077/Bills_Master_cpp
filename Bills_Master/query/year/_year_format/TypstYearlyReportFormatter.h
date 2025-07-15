#ifndef TYPST_YEARLY_REPORT_FORMATTER_H
#define TYPST_YEARLY_REPORT_FORMATTER_H

#include <string>
#include "year/_year_data/YearlyReportData.h"

/**
 * @class TypstYearlyReportFormatter
 * @brief 将年度报告数据格式化为 Typst 字符串。
 */
class TypstYearlyReportFormatter {
public:
    /**
     * @brief 接收年度数据并返回格式化的 Typst 字符串。
     * @param data 包含所有年度报告数据的 YearlyReportData 结构体。
     * @return 一个包含完整 Typst 源码的字符串。
     */
    std::string format_report(const YearlyReportData& data);
};

#endif // TYPST_YEARLY_REPORT_FORMATTER_H