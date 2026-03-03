// adapters/io/FileBillContentReader.hpp
#ifndef FILE_BILL_CONTENT_READER_HPP
#define FILE_BILL_CONTENT_READER_HPP

#include "ports/bills_content_reader.hpp"

class FileBillContentReader : public BillContentReader {
 public:
  auto Read(const std::string& file_path) -> std::string override;
};

#endif  // FILE_BILL_CONTENT_READER_HPP
