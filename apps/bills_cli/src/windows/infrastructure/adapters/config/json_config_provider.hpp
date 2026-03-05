// adapters/config/JsonConfigProvider.hpp
#ifndef JSON_CONFIG_PROVIDER_HPP
#define JSON_CONFIG_PROVIDER_HPP

#include "ports/config_provider.hpp"

class JsonConfigProvider final : public ConfigProvider {
 public:
  JsonConfigProvider() = default;

  auto Load(const std::string& config_path) -> Result<ConfigBundle> override;
};

#endif  // JSON_CONFIG_PROVIDER_HPP


