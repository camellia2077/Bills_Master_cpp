#ifndef BILL_MODIFIER_H
#define BILL_MODIFIER_H

#include <string>
#include "modifier/_shared_structures/BillDataStructures.h" // 包含共用的数据结构
#include "nlohmann/json.hpp"

class BillModifier {
public:
    explicit BillModifier(const nlohmann::json& config_json);
    std::string modify(const std::string& bill_content);

private:
    // BillModifier 现在只持有一个配置对象
    Config m_config;
};

#endif // BILL_MODIFIER_H