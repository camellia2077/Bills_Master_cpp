// modifier/raw_format/BillJsonFormatter.hpp

#ifndef BILL_JSON_FORMATTER_H
#define BILL_JSON_FORMATTER_H

#include "reprocessing/modifier/_shared_structures/BillDataStructures.hpp"
#include "nlohmann/json.hpp"
#include <map> // <-- 新增

class BillJsonFormatter {
public:
    BillJsonFormatter() = default;

    std::string format(const std::vector<ParentItem>& bill_structure, const std::vector<std::string>& metadata_lines, const std::map<std::string, std::string>& display_names) const; // <-- 修改

private:
    void _parse_content_line(const std::string& line, double& amount, std::string& description, std::string& comment) const;
};

#endif // BILL_JSON_FORMATTER_H