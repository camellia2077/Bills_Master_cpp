// config_validator/pipeline/modifier_config_validator.hpp
#ifndef MODIFIER_CONFIG_VALIDATOR_HPP
#define MODIFIER_CONFIG_VALIDATOR_HPP

#include <string>

#include <toml++/toml.hpp>

class ModifierConfigValidator {
 public:
  static bool validate(const toml::table& config_toml,
                       std::string& error_message);
};

#endif  // MODIFIER_CONFIG_VALIDATOR_HPP
