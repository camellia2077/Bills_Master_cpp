#ifndef COMMON_TEXT_NORMALIZER_H_
#define COMMON_TEXT_NORMALIZER_H_

#include <string>
#include <string_view>

#include "common/Result.hpp"

auto NormalizeBillText(std::string_view raw_bytes) -> Result<std::string>;

#endif  // COMMON_TEXT_NORMALIZER_H_
