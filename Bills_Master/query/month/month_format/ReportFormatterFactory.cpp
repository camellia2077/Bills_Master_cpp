// ReportFormatterFactory.cpp
#include "ReportFormatterFactory.h"
#include "month_md/MonthMdFormat.h"
#include "month_tex/MonthTexFormat.h"
#include "month_typ/MonthTypFormat.h"

std::unique_ptr<IMonthReportFormatter> ReportFormatterFactory::createFormatter(ReportFormat format) {
    switch (format) {
        case ReportFormat::Markdown:
            return std::make_unique<MonthMdFormat>();
        case ReportFormat::LaTeX:
            return std::make_unique<MonthTexFormat>();
        case ReportFormat::Typst:
            return std::make_unique<MonthTypFormat>();
        default:
            return nullptr; // 或者抛出异常，表示不支持的格式
    }
}