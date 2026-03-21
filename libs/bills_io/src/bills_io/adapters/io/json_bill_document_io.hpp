#ifndef BILLS_IO_ADAPTERS_IO_JSON_BILL_DOCUMENT_IO_HPP_
#define BILLS_IO_ADAPTERS_IO_JSON_BILL_DOCUMENT_IO_HPP_

#include <filesystem>
#include <string>
#include <vector>

#include "common/Result.hpp"
#include "common/source_document.hpp"
#include "domain/bill/bill_record.hpp"

class JsonBillDocumentIo {
 public:
  [[nodiscard]] static auto Serialize(
      const std::vector<std::pair<std::string, ParsedBill>>& bills)
      -> SourceDocumentBatch;

  [[nodiscard]] static auto Deserialize(const SourceDocumentBatch& documents)
      -> Result<std::vector<std::pair<std::string, ParsedBill>>>;
};

#endif  // BILLS_IO_ADAPTERS_IO_JSON_BILL_DOCUMENT_IO_HPP_
