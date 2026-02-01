// conversion/Reprocessor.hpp
#ifndef REPROCESSOR_HPP
#define REPROCESSOR_HPP

#include <string>
#include <memory>
#include "common/structures/CommonData.hpp"
#include "conversion/validator/BillValidator.hpp"
#include "conversion/convert/BillConverter.hpp"
#include "nlohmann/json.hpp" // 包含json头文件

class Reprocessor {
public:
    /**
     * @brief 构造函数，使用已验证的配置对象进行初始化。
     * @param validator_config_json 用于 BillValidator 的有效 JSON 配置。
     * @param modifier_config_json 用于 BillConverter 的有效 JSON 配置。
     */
    explicit Reprocessor(const nlohmann::json& validator_config_json, const nlohmann::json& modifier_config_json);

    bool validate_content(const std::string& bill_content,
                          const std::string& source_name);
    bool validate_and_convert_content(const std::string& bill_content,
                                      const std::string& source_name,
                                      ParsedBill& bill_data);
    bool convert_content(const std::string& bill_content, ParsedBill& bill_data);

private:
    std::unique_ptr<BillValidator> m_validator;
    std::unique_ptr<BillConverter> m_converter;
};

#endif // REPROCESSOR_HPP
