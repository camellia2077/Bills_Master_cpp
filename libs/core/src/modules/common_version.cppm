module;
#include "common/version.hpp"

export module bill.core.common.version;

export namespace bills::core::modules::common_version {
inline constexpr auto kVersion = ::bills::core::version::kVersion;
inline constexpr auto kLastUpdated = ::bills::core::version::kLastUpdated;
}
