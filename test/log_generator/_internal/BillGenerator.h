#ifndef BILL_GENERATOR_H
#define BILL_GENERATOR_H

#include <filesystem>
#include <random>
#include <nlohmann/json.hpp>

class BillGenerator {
public:
    // Constructor initializes the random engine with a fixed seed for reproducibility.
    BillGenerator();

    // Loads configuration from the specified file. Returns false on failure.
    bool load_config(const std::string& config_path);

    // Generates a single bill file for a given year and month.
    void generate_bill_file(int year, int month, const std::filesystem::path& dir_path) const;

private:
    // Generates a pseudo-random double.
    double random_double(double min, double max) const;
    
    // Generates a pseudo-random integer.
    int random_int(int min, int max) const;

    nlohmann::json config_;
    // Mutable allows the random_engine_ to be modified by const methods.
    mutable std::mt19937 random_engine_; 
};

#endif // BILL_GENERATOR_H