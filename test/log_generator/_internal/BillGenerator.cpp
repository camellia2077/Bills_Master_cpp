#include "BillGenerator.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <algorithm> // For std::shuffle

using json = nlohmann::json;

BillGenerator::BillGenerator() : random_engine_(0) {} // Fixed seed for reproducibility 

bool BillGenerator::load_config(const std::string& config_path) {
    std::ifstream config_file(config_path);
    if (!config_file.is_open()) {
        std::cerr << "Error: Could not find or open configuration file '" << config_path << "'." << std::endl;
        return false;
    }

    try {
        config_ = json::parse(config_file);
    } catch (json::parse_error& e) {
        std::cerr << "Error: Failed to parse " << config_path << ".\n" << e.what() << std::endl;
        return false;
    }
    config_file.close();
    return true;
}

void BillGenerator::generate_bill_file(int year, int month, const std::filesystem::path& dir_path) const {
    std::stringstream ss_year_month;
    ss_year_month << year << std::setw(2) << std::setfill('0') << month;
    std::string year_month = ss_year_month.str();
    std::filesystem::path file_path = dir_path / (year_month + ".txt");

    std::ofstream outfile(file_path);
    if (!outfile) {
        std::cerr << "Cannot generate file: " << file_path << std::endl;
        return;
    }

    outfile << "DATE:" << year_month << std::endl;
    outfile << "REMARK:" << std::endl;
    
    // Add two blank lines after the REMARK line
    outfile << std::endl;
    outfile << std::endl;

    // Use an iterator-based loop to check if we are on the last parent category
    for (auto parent_it = config_.begin(); parent_it != config_.end(); ++parent_it) {
        // Print the parent category name
        outfile << (*parent_it)["name"].get<std::string>() << std::endl;

        const auto& sub_items = (*parent_it)["sub_items"];
        for (const auto& sub_config : sub_items) {
            // Add a blank line before each sub-category block
            outfile << std::endl;
            outfile << sub_config["name"].get<std::string>() << std::endl;

            std::vector<json> details_vector;
            if (sub_config.contains("details")) {
                for (const auto& detail_item : sub_config["details"]) {
                    details_vector.push_back(detail_item);
                }
            }
            if (details_vector.empty()) continue;

            int num_to_gen = random_int(1, details_vector.size());
            std::shuffle(details_vector.begin(), details_vector.end(), random_engine_);

            for (int i = 0; i < num_to_gen; ++i) {
                const auto& item = details_vector[i];
                double cost = random_double(item["min_cost"], item["max_cost"]);
                outfile << std::fixed << std::setprecision(2) << cost << item["description"].get<std::string>() << std::endl;
            }
        }

        // After all sub-categories of a parent are done, add two blank lines,
        // but only if it's not the very last parent category in the file.
        if (std::next(parent_it) != config_.end()) {
            outfile << std::endl;
            outfile << std::endl;
        }
    }
    outfile.close();
}

double BillGenerator::random_double(double min, double max) const {
    std::uniform_real_distribution<> dist(min, max);
    return dist(random_engine_);
}

int BillGenerator::random_int(int min, int max) const {
    std::uniform_int_distribution<> dist(min, max);
    return dist(random_engine_);
}