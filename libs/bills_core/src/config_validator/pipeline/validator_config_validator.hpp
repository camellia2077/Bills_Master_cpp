// config_validator/pipeline/validator_config_validator.hpp
#ifndef VALIDATOR_CONFIG_VALIDATOR_HPP
#define VALIDATOR_CONFIG_VALIDATOR_HPP

#include <string>

#include <toml++/toml.hpp>

class ValidatorConfigValidator {
 public:
  static bool validate(const toml::table& config_toml,
                       std::string& error_message);
};

#endif  // VALIDATOR_CONFIG_VALIDATOR_HPP
