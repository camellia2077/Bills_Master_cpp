// query/year/year_format/YearlyReportFormatterFactory.cpp
#include "YearlyReportFormatterFactory.h"

// Include all the concrete formatter headers
#include "query\year\year_format\year_md/YearMdFormat.h"
#include "query\year\year_format\year_tex\YearTexFormat.h"
#include "query\year\year_format\year_rst\YearRstFormat.h"
#include "query\year\year_format\year_typ\YearTypFormat.h"

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