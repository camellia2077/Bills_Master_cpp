// ReportFormatterFactory.h
#ifndef REPORT_FORMATTER_FACTORY_H
#define REPORT_FORMATTER_FACTORY_H

#include <memory>
#include <string>
#include "IMonthReportFormatter.h"
#include "query/ReportFormat.h" // 包含唯一的枚举头文件


class ReportFormatterFactory {
public:
    static std::unique_ptr<IMonthReportFormatter> createFormatter(ReportFormat format);
};

#endif // REPORT_FORMATTER_FACTORY_H