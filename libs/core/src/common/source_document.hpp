#ifndef COMMON_SOURCE_DOCUMENT_HPP_
#define COMMON_SOURCE_DOCUMENT_HPP_

#include <string>
#include <vector>

struct SourceDocument {
  std::string display_path;
  std::string text;
};

using SourceDocumentBatch = std::vector<SourceDocument>;

#endif  // COMMON_SOURCE_DOCUMENT_HPP_
