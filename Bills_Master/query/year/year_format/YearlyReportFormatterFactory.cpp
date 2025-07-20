// query/year/year_format/YearlyReportFormatterFactory.cpp
#include "YearlyReportFormatterFactory.h"

// Include all the concrete formatter headers
#include "YearMdFormat.h"
#include "YearTexFormat.h"
#include "YearRstFormat.h"
#include "YearTypFormat.h"

std::unique_ptr<IYearlyReportFormatter> YearlyReportFormatterFactory::createFormatter(ReportFormat format) {
    switch (format) {
        case ReportFormat::Markdown:
            return std::make_unique<YearMdFormat>();
        case ReportFormat::LaTeX:
            return std::make_unique<YearTexFormat>();
        case ReportFormat::Rst:
            return std::make_unique<YearRstFormat>();
        case ReportFormat::Typst:
            return std::make_unique<YearTypFormat>();
        default:
            // Return nullptr if the format is unknown
            return nullptr;
    }
}