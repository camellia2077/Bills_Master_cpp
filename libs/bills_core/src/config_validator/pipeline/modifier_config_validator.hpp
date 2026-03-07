// config_validator/pipeline/modifier_config_validator.hpp
#ifndef CONFIG_VALIDATOR_PIPELINE_MODIFIER_CONFIG_VALIDATOR_H_
#define CONFIG_VALIDATOR_PIPELINE_MODIFIER_CONFIG_VALIDATOR_H_

#include <string>

#include <toml++/toml.hpp>

class ModifierConfigValidator {
 public:
  static bool validate(const toml::table& config_toml,
                       std::string& error_message);
};

#endif  // CONFIG_VALIDATOR_PIPELINE_MODIFIER_CONFIG_VALIDATOR_H_
