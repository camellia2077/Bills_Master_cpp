// ReportFormatterFactory.cpp
#include "ReportFormatterFactory.h"
#include "query/month/month_format/month_md/MonthMdFormat.h"
#include "query/month/month_format/month_tex/MonthTexFormat.h"
#include "query/month/month_format/month_typ/MonthTypFormat.h"
#include "query/month/month_format/month_rst/MonthRstFormat.h"

std::unique_ptr<IMonthReportFormatter> ReportFormatterFactory::createFormatter(ReportFormat format) {
    switch (format) {
        case ReportFormat::Markdown:
            return std::make_unique<MonthMdFormat>();
        case ReportFormat::LaTeX:
            return std::make_unique<MonthTexFormat>();
        case ReportFormat::Typst:
            return std::make_unique<MonthTypFormat>();
        
        case ReportFormat::Rst:
            return std::make_unique<MonthRstFormat>();

        default:
            return nullptr; // 如果格式未知，返回空指针
    }
}