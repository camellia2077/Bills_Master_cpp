// MonthlyReportGenerator.cpp
#include "MonthlyReportGenerator.h"
#include "query/month/_month_data/ReportData.h"
#include "query/month/month_format/IMonthReportFormatter.h" // Include the interface for polymorphism
#include <stdexcept> // To throw an exception for unsupported formats

// The constructor now only initializes the reader.
MonthlyReportGenerator::MonthlyReportGenerator(sqlite3* db_connection)
    : m_reader(db_connection) {}

// The generate method now uses the factory.
std::string MonthlyReportGenerator::generate(int year, int month, ReportFormat format) {
    // Step 1: Use the internal reader to get data from the database.
    MonthlyReportData data = m_reader.read_monthly_data(year, month);

    // Step 2: Use the factory to create the appropriate formatter.
    auto formatter = ReportFormatterFactory::createFormatter(format);

    // It's good practice to handle cases where the factory might return a null pointer.
    if (!formatter) {
        throw std::runtime_error("Unsupported report format specified.");
    }

    // Step 3: Use the created formatter to generate the report and return it.
    // Thanks to polymorphism, we call format_report on the interface pointer.
    return formatter->format_report(data);
}