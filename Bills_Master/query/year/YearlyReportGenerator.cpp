// YearlyReportGenerator.cpp
#include "YearlyReportGenerator.h"
#include <stdexcept> // For throwing exceptions

// The constructor now only needs to initialize the reader.
YearlyReportGenerator::YearlyReportGenerator(sqlite3* db_connection)
    : m_reader(db_connection) {}

// The generate method is now much cleaner and uses the factory.
std::string YearlyReportGenerator::generate(int year, ReportFormat format) {
    // Step 1: Get the data using the reader.
    YearlyReportData data = m_reader.read_yearly_data(year);

    // Step 2: Create the correct formatter using the factory.
    auto formatter = YearlyReportFormatterFactory::createFormatter(format);

    // Always check if the factory returned a valid formatter.
    if (!formatter) {
        throw std::runtime_error("An unsupported report format was specified.");
    }

    // Step 3: Use the formatter to generate and return the report.
    return formatter->format_report(data);
}