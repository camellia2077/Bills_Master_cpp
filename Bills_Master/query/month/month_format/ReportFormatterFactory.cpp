// ReportFormatterFactory.cpp
#include "ReportFormatterFactory.h"
#include "month_md/MonthMdFormat.h"
#include "month_tex/MonthTexFormat.h"
#include "month_typ/MonthTypFormat.h"
#include "month_rst/MonthRstFormat.h"

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