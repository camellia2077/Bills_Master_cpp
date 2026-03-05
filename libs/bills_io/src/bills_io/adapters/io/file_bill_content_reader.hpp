#ifndef BILLS_IO_FILE_BILL_CONTENT_READER_HPP
#define BILLS_IO_FILE_BILL_CONTENT_READER_HPP

#include "ports/bills_content_reader.hpp"

class FileBillContentReader : public BillContentReader {
 public:
  auto Read(const std::string& file_path) -> std::string override;
};

#endif  // BILLS_IO_FILE_BILL_CONTENT_READER_HPP
