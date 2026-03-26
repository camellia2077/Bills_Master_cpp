module;
#include "common/Result.hpp"

export module bill.core.common.result;

export namespace bills::core::modules::common_result {
using Error = ::Error;

template <typename T>
using Result = ::Result<T>;

using Status = ::Status;
using ::FormatError;
using ::MakeError;
}
