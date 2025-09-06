// src/reprocessing/modifier/raw_format/BillJsonFormatter.cpp

#include "BillJsonFormatter.hpp"
#include <regex>
#include <string>
#include <iomanip>
#include <sstream>

std::string BillJsonFormatter::format(
    const std::vector<ParentItem>& bill_structure, 
    const std::vector<std::string>& metadata_lines) const 
{
    nlohmann::ordered_json root;

    for (const auto& meta_line : metadata_lines) {
        if (meta_line.rfind("DATE:", 0) == 0) root["date"] = meta_line.substr(5);
        else if (meta_line.rfind("REMARK:", 0) == 0) root["remark"] = meta_line.substr(7);
    }

    nlohmann::ordered_json categories_obj = nlohmann::ordered_json::object();
    double total_amount = 0.0;

    for (const auto& parent : bill_structure) {
        nlohmann::ordered_json parent_node;
        nlohmann::ordered_json transactions_array = nlohmann::ordered_json::array();
        double parent_sub_total = 0.0;

        for (const auto& sub : parent.sub_items) {
            for (const auto& content_line : sub.contents) {
                nlohmann::ordered_json transaction_node;
                double amount = 0.0;
                std::string description, comment;
                _parse_content_line(content_line, amount, description, comment);
                
                transaction_node["sub_category"] = sub.title;
                transaction_node["description"] = description;
                transaction_node["amount"] = amount;
                transaction_node["source"] = "manually_add";
                transaction_node["transaction_type"] = (parent.title == "income") ? "Income" : "Expense";
                transaction_node["comment"] = comment.empty() ? nullptr : nlohmann::json(comment);
                
                transactions_array.push_back(transaction_node);
                parent_sub_total += amount;
            }
        }
        
        parent_node["display_name"] = parent.title;
        
        std::stringstream ss_sub_total;
        ss_sub_total << std::fixed << std::setprecision(2) << parent_sub_total;
        parent_node["sub_total"] = nlohmann::json::parse(ss_sub_total.str());
        parent_node["transactions"] = transactions_array;
        
        categories_obj[parent.title] = parent_node;
        total_amount += parent_sub_total;
    }

    std::stringstream ss_total_amount;
    ss_total_amount << std::fixed << std::setprecision(2) << total_amount;
    root["total_amount"] = nlohmann::json::parse(ss_total_amount.str());
    root["categories"] = categories_obj;

    return root.dump(4);
}

void BillJsonFormatter::_parse_content_line(const std::string& line, double& amount, std::string& description, std::string& comment) const {
    std::smatch match;
    std::regex re(R"(^(\d+(?:\.\d+)?)\s*(.*))");
    std::string full_description_part;

    if (std::regex_match(line, match, re) && match.size() == 3) {
        try {
            amount = std::stod(match[1].str());
            full_description_part = match[2].str();
        } catch (const std::exception&) {
            amount = 0.0;
            full_description_part = line;
        }
    } else {
        amount = 0.0;
        full_description_part = line;
    }

    size_t comment_pos = full_description_part.find("//");
    if (comment_pos != std::string::npos) {
        comment = full_description_part.substr(comment_pos + 2);
        description = full_description_part.substr(0, comment_pos);
    } else {
        description = full_description_part;
        comment = "";
    }
    
    // Trim trailing whitespace from description and leading from comment
    description.erase(description.find_last_not_of(" \t\n\r") + 1);
    comment.erase(0, comment.find_first_not_of(" \t\n\r"));
}