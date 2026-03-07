// ports/config_provider.hpp
#ifndef PORTS_CONFIG_PROVIDER_H_
#define PORTS_CONFIG_PROVIDER_H_

#include <string>

#include "common/Result.hpp"
#include "billing/conversion/modifier/_shared_structures/bills_data_structures.hpp"
#include "billing/conversion/validator/config/bills_config.hpp"

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

#endif  // PORTS_CONFIG_PROVIDER_H_

