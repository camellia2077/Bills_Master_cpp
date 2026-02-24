// conversion/validator/config/BillConfig.hpp
#ifndef BILL_CONFIG_HPP
#define BILL_CONFIG_HPP

#include <set>
#include <string>
#include <unordered_map>

struct BillValidationRules {
  std::unordered_map<std::string, std::set<std::string>> validation_map;
  std::set<std::string> parent_titles;
};

class BillConfig {
 public:
  /**
   * @brief 构造函数，从验证规则构建配置。
   * @param rules 解析后的验证规则。
   */
  explicit BillConfig(BillValidationRules rules);

  bool is_parent_title(const std::string& title) const;
  bool is_valid_sub_title(const std::string& parent_title,
                          const std::string& sub_title) const;

 private:
  std::unordered_map<std::string, std::set<std::string>> validation_map_;
  std::set<std::string> all_parent_titles_;
};

#endif  // BILL_CONFIG_HPP
