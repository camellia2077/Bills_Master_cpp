#include "common/text_normalizer.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace {

constexpr unsigned char kUtf8BomByte1 = 0xEF;
constexpr unsigned char kUtf8BomByte2 = 0xBB;
constexpr unsigned char kUtf8BomByte3 = 0xBF;
constexpr std::size_t kUtf8BomSize = 3U;
constexpr const char* kTextNormalizerContext = "TextNormalizer";

auto MakeUtf8Error(std::size_t byte_offset, std::string_view detail) -> Error {
  return MakeError(
      "Input text must be valid UTF-8. " + std::string(detail) +
          " at byte offset " + std::to_string(byte_offset) + ".",
      kTextNormalizerContext);
}

auto IsContinuationByte(unsigned char byte) -> bool {
  return (byte & 0xC0U) == 0x80U;
}

auto ValidateUtf8(std::string_view text) -> Status {
  std::size_t index = 0;
  while (index < text.size()) {
    const unsigned char lead =
        static_cast<unsigned char>(text[index]);
    if (lead <= 0x7FU) {
      ++index;
      continue;
    }

    if (lead >= 0xC2U && lead <= 0xDFU) {
      if (index + 1U >= text.size()) {
        return std::unexpected(MakeUtf8Error(index, "Truncated UTF-8 sequence"));
      }
      const unsigned char next =
          static_cast<unsigned char>(text[index + 1U]);
      if (!IsContinuationByte(next)) {
        return std::unexpected(
            MakeUtf8Error(index, "Invalid UTF-8 continuation byte"));
      }
      index += 2U;
      continue;
    }

    if (lead >= 0xE0U && lead <= 0xEFU) {
      if (index + 2U >= text.size()) {
        return std::unexpected(MakeUtf8Error(index, "Truncated UTF-8 sequence"));
      }
      const unsigned char next1 =
          static_cast<unsigned char>(text[index + 1U]);
      const unsigned char next2 =
          static_cast<unsigned char>(text[index + 2U]);
      if (!IsContinuationByte(next1) || !IsContinuationByte(next2)) {
        return std::unexpected(
            MakeUtf8Error(index, "Invalid UTF-8 continuation byte"));
      }
      if (lead == 0xE0U && next1 < 0xA0U) {
        return std::unexpected(MakeUtf8Error(index, "Overlong UTF-8 sequence"));
      }
      if (lead == 0xEDU && next1 >= 0xA0U) {
        return std::unexpected(
            MakeUtf8Error(index, "UTF-8 surrogate code point is not allowed"));
      }
      index += 3U;
      continue;
    }

    if (lead >= 0xF0U && lead <= 0xF4U) {
      if (index + 3U >= text.size()) {
        return std::unexpected(MakeUtf8Error(index, "Truncated UTF-8 sequence"));
      }
      const unsigned char next1 =
          static_cast<unsigned char>(text[index + 1U]);
      const unsigned char next2 =
          static_cast<unsigned char>(text[index + 2U]);
      const unsigned char next3 =
          static_cast<unsigned char>(text[index + 3U]);
      if (!IsContinuationByte(next1) || !IsContinuationByte(next2) ||
          !IsContinuationByte(next3)) {
        return std::unexpected(
            MakeUtf8Error(index, "Invalid UTF-8 continuation byte"));
      }
      if (lead == 0xF0U && next1 < 0x90U) {
        return std::unexpected(MakeUtf8Error(index, "Overlong UTF-8 sequence"));
      }
      if (lead == 0xF4U && next1 > 0x8FU) {
        return std::unexpected(
            MakeUtf8Error(index, "UTF-8 code point exceeds U+10FFFF"));
      }
      index += 4U;
      continue;
    }

    return std::unexpected(MakeUtf8Error(index, "Invalid UTF-8 lead byte"));
  }

  return {};
}

auto NormalizeLineEndings(std::string_view text) -> std::string {
  std::string normalized;
  normalized.reserve(text.size());

  for (std::size_t index = 0; index < text.size(); ++index) {
    const char current = text[index];
    if (current == '\r') {
      if (index + 1U < text.size() && text[index + 1U] == '\n') {
        continue;
      }
      normalized.push_back('\n');
      continue;
    }
    normalized.push_back(current);
  }

  return normalized;
}

}  // namespace

auto NormalizeBillText(std::string_view raw_bytes) -> Result<std::string> {
  if (raw_bytes.size() >= kUtf8BomSize &&
      static_cast<unsigned char>(raw_bytes[0]) == kUtf8BomByte1 &&
      static_cast<unsigned char>(raw_bytes[1]) == kUtf8BomByte2 &&
      static_cast<unsigned char>(raw_bytes[2]) == kUtf8BomByte3) {
    raw_bytes.remove_prefix(kUtf8BomSize);
  }

  const Status validation = ValidateUtf8(raw_bytes);
  if (!validation) {
    return std::unexpected(validation.error());
  }

  return NormalizeLineEndings(raw_bytes);
}
