// ports/ConfigProvider.hpp
#ifndef CONFIG_PROVIDER_HPP
#define CONFIG_PROVIDER_HPP

#include <string>

#include "common/Result.hpp"
#include "conversion/modifier/_shared_structures/BillDataStructures.hpp"
#include "conversion/validator/config/BillConfig.hpp"

struct ConfigBundle {
  BillConfig validator_config;
  Config modifier_config;
};

class ConfigProvider {
 public:
  virtual ~ConfigProvider() = default;
  [[nodiscard]] virtual auto Load(const std::string& config_path)
      -> Result<ConfigBundle> = 0;
};

#endif  // CONFIG_PROVIDER_HPP
