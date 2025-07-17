// YearRstFormat.h
#ifndef YEAR_RST_FORMAT_H
#define YEAR_RST_FORMAT_H

#include <string>
#include "year/_year_data/YearlyReportData.h"

/**
 * @class YearRstFormat
 * @brief 将年度报告数据格式化为 reStructuredText (RST) 字符串。
 */
class YearRstFormat {
public:
    /**
     * @brief 接收年度数据并返回格式化的 RST 字符串。
     * @param data 包含所有年度报告数据的 YearlyReportData 结构体。
     * @return 一个包含完整 RST 源码的字符串。
     */
    std::string format_report(const YearlyReportData& data);
};

#endif // YEAR_RST_FORMAT_H