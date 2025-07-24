// query/year/year_format/YearlyReportFormatterFactory.h
#ifndef YEARLY_REPORT_FORMATTER_FACTORY_H
#define YEARLY_REPORT_FORMATTER_FACTORY_H

#include <memory>
#include "IYearlyReportFormatter.h"
#include "query/ReportFormat.h" // 共享的格式化类型枚举

class YearlyReportFormatterFactory {
public:
    /**
     * @brief Creates a formatter based on the requested format.
     * @param format The desired output format.
     * @return A unique_ptr to a formatter that implements the interface.
     */
    static std::unique_ptr<IYearlyReportFormatter> createFormatter(ReportFormat format);
};

#endif // YEARLY_REPORT_FORMATTER_FACTORY_H