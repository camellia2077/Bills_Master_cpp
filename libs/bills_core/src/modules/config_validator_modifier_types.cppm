module;
#include <string>
#include "nlohmann/json.hpp"

export module bill.core.config.modifier:types;

export namespace bills::core::modules::config_validator {
using ModifierJson = nlohmann::json;
using ModifierErrorMessage = std::string;
}  // namespace bills::core::modules::config_validator

