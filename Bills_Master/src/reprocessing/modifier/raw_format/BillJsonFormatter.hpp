// modifier/raw_format/BillJsonFormatter.hpp

#ifndef BILL_JSON_FORMATTER_H
#define BILL_JSON_FORMATTER_H

#include "reprocessing/modifier/_shared_structures/BillDataStructures.hpp"
#include "nlohmann/json.hpp"

class BillJsonFormatter {
public:
    BillJsonFormatter() = default;

    /**
     * @brief Converts structured bill data into a JSON string.
     * @param bill_structure The structured data from BillContentTransformer.
     * @param metadata_lines Lines containing metadata like DATE and REMARK.
     * @return A formatted JSON string.
     */
    std::string format(const std::vector<ParentItem>& bill_structure, const std::vector<std::string>& metadata_lines) const;

private:
    /**
     * @brief Parses an amount, description, and comment from a content line.
     * @param line The raw content line, e.g., "9.9 Coffee (Morning boost)".
     * @param amount Reference to store the parsed amount.
     * @param description Reference to store the parsed description.
     * @param comment Reference to store the parsed comment.
     */
    void _parse_content_line(const std::string& line, double& amount, std::string& description, std::string& comment) const;
};

#endif // BILL_JSON_FORMATTER_H