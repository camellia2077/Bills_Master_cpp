module;
#include <string>
#include "nlohmann/json.hpp"

export module bill.core.config.validator:types;

export namespace bills::core::modules::config_validator {
using ValidatorJson = nlohmann::json;
using ValidatorErrorMessage = std::string;
}  // namespace bills::core::modules::config_validator

