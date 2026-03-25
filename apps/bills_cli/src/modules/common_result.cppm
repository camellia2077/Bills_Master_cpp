module;
#include "common/Result.hpp"

export module bill.cli.deps.common_result;

export namespace bills::cli::deps::common_result {
using ::Error;

template <typename T>
using Result = ::Result<T>;

using ::FormatError;
using ::MakeError;
using ::Status;
}
