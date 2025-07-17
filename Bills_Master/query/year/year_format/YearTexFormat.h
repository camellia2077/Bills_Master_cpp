#ifndef YEAR_TEX_FORMAT_H
#define YEAR_TEX_FORMAT_H

#include <string>
#include "year/_year_data/YearlyReportData.h"

/**
 * @class YearTexFormat
 * @brief 将年度报告数据格式化为 LaTeX 字符串。
 */
class YearTexFormat {
public:
    /**
     * @brief 接收年度数据并返回格式化的 LaTeX 字符串。
     * @param data 包含所有年度报告数据的 YearlyReportData 结构体。
     * @return 一个包含完整 LaTeX 源码的字符串。
     */
    std::string format_report(const YearlyReportData& data);
};

#endif // YEAR_TEX_FORMAT_H