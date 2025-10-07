// reprocessing/validator/BillValidator.cpp
#include "BillValidator.hpp"
#include "common/common_utils.hpp"
#include <iostream>
#include <string>
#include <sstream>

// --- NEW: TxtStructureVerifier implementation ---
bool TxtStructureVerifier::verify(const std::string& bill_content, ValidationResult& result) {
    std::istringstream stream(bill_content);
    std::string line;
    int line_num = 0;

    // Validate date
    if (std::getline(stream, line)) {
        line_num++;
        if (!std::regex_match(line, std::regex(R"(^date:\d{6}$)"))) {
            result.add_error("Error (Line " + std::to_string(line_num) + "): The first line must be 'date:YYYYMM'. Found: '" + line + "'");
            return false;
        }
    } else {
        result.add_error("Error: Bill content is empty or has less than two lines.");
        return false;
    }

    // Validate remark
    if (std::getline(stream, line)) {
        line_num++;
        if (!std::regex_match(line, std::regex(R"(^remark:.*)"))) {
            result.add_error("Error (Line " + std::to_string(line_num) + "): The second line must start with 'remark:'. Found: '" + line + "'");
            return false;
        }
    } else {
        result.add_error("Error: Bill content has less than two lines.");
        return false;
    }
    
    return true;
}

// --- NEW: JsonBillValidator implementation ---
bool JsonBillValidator::verify(const nlohmann::json& bill_json, const BillConfig& config, ValidationResult& result) {
    if (!bill_json.contains("categories") || !bill_json["categories"].is_object()) {
        result.add_error("JSON Validation Error: Missing or invalid 'categories' object.");
        return false;
    }

    const auto& categories = bill_json["categories"];
    for (auto it = categories.begin(); it != categories.end(); ++it) {
        const std::string& parent_title = it.key();
        const auto& parent_node = it.value();

        if (!config.is_parent_title(parent_title)) {
            result.add_error("JSON Content Error: Parent item '" + parent_title + "' is not a valid parent title defined in the configuration.");
        }

        if (!parent_node.contains("transactions") || !parent_node["transactions"].is_array()) {
            result.add_warning("JSON Structure Warning: Parent item '" + parent_title + "' is missing a 'transactions' array.");
            continue;
        }

        for (const auto& transaction : parent_node["transactions"]) {
            if (!transaction.contains("sub_category") || !transaction["sub_category"].is_string()) {
                result.add_error("JSON Content Error in '" + parent_title + "': Transaction is missing a 'sub_category' field.");
                continue;
            }
            std::string sub_title = transaction["sub_category"];
            if (!config.is_valid_sub_title(parent_title, sub_title)) {
                result.add_error("JSON Content Error: Sub-category '" + sub_title + "' is not a valid sub-item for parent '" + parent_title + "'.");
            }
        }
    }

    return !result.has_errors();
}


// --- BillValidator Implementation ---
BillValidator::BillValidator(const nlohmann::json& config_json)
    : m_config(std::make_unique<BillConfig>(config_json))
{
    std::cout << "BillValidator initialized successfully, configuration loaded.\n";
}

bool BillValidator::validate_txt_structure(const std::string& bill_content, ValidationResult& result) {
    return m_txt_verifier.verify(bill_content, result);
}

bool BillValidator::validate_json_content(const nlohmann::json& bill_json, ValidationResult& result) {
    return m_json_verifier.verify(bill_json, *m_config, result);
}