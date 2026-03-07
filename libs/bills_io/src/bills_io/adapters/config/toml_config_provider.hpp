// bills_io/adapters/config/toml_config_provider.hpp
#ifndef BILLS_IO_ADAPTERS_CONFIG_TOML_CONFIG_PROVIDER_H_
#define BILLS_IO_ADAPTERS_CONFIG_TOML_CONFIG_PROVIDER_H_

#include "ports/config_provider.hpp"

class TomlConfigProvider final : public ConfigProvider {
 public:
  TomlConfigProvider() = default;

  auto Load(const std::string& config_path) -> Result<ConfigBundle> override;
};

#endif  // BILLS_IO_ADAPTERS_CONFIG_TOML_CONFIG_PROVIDER_H_

