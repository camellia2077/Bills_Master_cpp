#ifndef CONFIG_LOADING_RUNTIME_CONFIG_LOADER_HPP_
#define CONFIG_LOADING_RUNTIME_CONFIG_LOADER_HPP_

#include <filesystem>

#include <toml++/toml.hpp>

#include "billing/conversion/modifier/_shared_structures/bills_data_structures.hpp"
#include "billing/conversion/validator/config/bills_config.hpp"
#include "common/Result.hpp"

struct RuntimeConfigBundle {
  BillConfig validator_config = BillConfig(BillValidationRules{});
  Config modifier_config;
};

class RuntimeConfigLoader {
 public:
  [[nodiscard]] static auto ReadTomlFile(const std::filesystem::path& file_path)
      -> Result<toml::table>;

  [[nodiscard]] static auto LoadFromConfigDir(
      const std::filesystem::path& config_dir) -> Result<RuntimeConfigBundle>;

  [[nodiscard]] static auto LoadFromFiles(
      const std::filesystem::path& validator_config_path,
      const std::filesystem::path& modifier_config_path)
      -> Result<RuntimeConfigBundle>;

  [[nodiscard]] static auto LoadValidatorConfig(const toml::table& validator_toml)
      -> Result<BillConfig>;

  [[nodiscard]] static auto LoadModifierConfig(const toml::table& modifier_toml)
      -> Result<Config>;
};

#endif  // CONFIG_LOADING_RUNTIME_CONFIG_LOADER_HPP_
