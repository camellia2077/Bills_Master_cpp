#include "io/adapters/io/json_bill_document_io.hpp"

#include <utility>
#include <vector>

#include "ingest/json/bills_json_serializer.hpp"

namespace {
constexpr const char* kContext = "JsonBillDocumentIo";
}  // namespace

auto JsonBillDocumentIo::Serialize(
    const std::vector<std::pair<std::string, ParsedBill>>& bills)
    -> SourceDocumentBatch {
  SourceDocumentBatch documents;
  documents.reserve(bills.size());
  for (const auto& [display_path, bill] : bills) {
    documents.push_back(SourceDocument{
        .display_path = display_path,
        .text = BillJsonSerializer::serialize(bill),
    });
  }
  return documents;
}

auto JsonBillDocumentIo::Deserialize(const SourceDocumentBatch& documents)
    -> Result<std::vector<std::pair<std::string, ParsedBill>>> {
  std::vector<std::pair<std::string, ParsedBill>> bills;
  bills.reserve(documents.size());
  try {
    for (const auto& document : documents) {
      bills.emplace_back(document.display_path,
                         BillJsonSerializer::deserialize(document.text));
    }
  } catch (const std::exception& error) {
    return std::unexpected(MakeError(error.what(), kContext));
  }
  return bills;
}
