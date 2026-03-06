// adapters/config/TomlConfigProvider.hpp
#ifndef TOML_CONFIG_PROVIDER_HPP
#define TOML_CONFIG_PROVIDER_HPP

#include "ports/config_provider.hpp"

class TomlConfigProvider final : public ConfigProvider {
 public:
  TomlConfigProvider() = default;

  auto Load(const std::string& config_path) -> Result<ConfigBundle> override;
};

#endif  // TOML_CONFIG_PROVIDER_HPP

