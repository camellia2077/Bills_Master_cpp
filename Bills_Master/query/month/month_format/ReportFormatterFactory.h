// ReportFormatterFactory.h
#ifndef REPORT_FORMATTER_FACTORY_H
#define REPORT_FORMATTER_FACTORY_H

#include <memory>
#include <string>
#include "IMonthReportFormatter.h"
#include "ReportFormat.h" // <-- 1. 包含唯一的枚举头文件

// 2. 从这里移除整个 enum class ReportFormat 的定义
// enum class ReportFormat {
//     Markdown,
//     Latex,
//     Typst
// };

class ReportFormatterFactory {
public:
    static std::unique_ptr<IMonthReportFormatter> createFormatter(ReportFormat format);
};

#endif // REPORT_FORMATTER_FACTORY_H