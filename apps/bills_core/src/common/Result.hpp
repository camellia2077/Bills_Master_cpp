// common/result.hpp
#ifndef RESULT_HPP
#define RESULT_HPP

#include <expected>
#include <string>

struct Error {
  std::string message_;
  std::string context_;
};

template <typename T>
using Result = std::expected<T, Error>;

using Status = std::expected<void, Error>;

inline auto MakeError(std::string message, std::string context = {}) -> Error {
  return Error{std::move(message), std::move(context)};
}

inline auto FormatError(const Error& error) -> std::string {
  if (error.context_.empty()) {
    return error.message_;
  }
  return error.context_ + ": " + error.message_;
}

#endif  // RESULT_HPP
