// adapters/config/JsonConfigProvider.hpp
#ifndef JSON_CONFIG_PROVIDER_HPP
#define JSON_CONFIG_PROVIDER_HPP

#include "file_handler/FileHandler.hpp"
#include "ports/ConfigProvider.hpp"

class JsonConfigProvider final : public ConfigProvider {
 public:
  explicit JsonConfigProvider(FileHandler& file_handler);

  auto Load(const std::string& config_path) -> Result<ConfigBundle> override;

 private:
  FileHandler& file_handler_;
};

#endif  // JSON_CONFIG_PROVIDER_HPP
