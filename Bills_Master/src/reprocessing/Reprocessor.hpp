#ifndef REPROCESSOR_H
#define REPROCESSOR_H

#include <string>
#include <memory> // 为 std::unique_ptr 添加
#include "reprocessing/validator/BillValidator.hpp"
#include "reprocessing/modifier/BillModifier.hpp"

class Reprocessor {
public:
    /**
     * @brief Constructs a Reprocessor instance.
     * @param config_dir_path Path to the directory containing configuration files.
     */
    explicit Reprocessor(const std::string& config_dir_path);

    /**
     * @brief Validates a bill file using the pre-loaded configuration.
     * @param bill_path Path to the bill file to validate.
     * @return True if validation passes (no errors), false otherwise.
     */
    bool validate_bill(const std::string& bill_path);

    /**
     * @brief Modifies a bill file using the pre-loaded configuration.
     * @param input_bill_path Path to the source bill file.
     * @param output_bill_path Path where the modified bill will be saved.
     * @return True on success, false if file operations fail.
     */
    bool modify_bill(const std::string& input_bill_path, const std::string& output_bill_path);

private:
    // **修改**: 存储验证器和修改器的实例，而不是路径
    std::unique_ptr<BillValidator> m_validator;
    std::unique_ptr<BillModifier> m_modifier;
};

#endif // REPROCESSOR_H