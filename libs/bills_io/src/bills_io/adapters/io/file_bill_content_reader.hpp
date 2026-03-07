// bills_io/adapters/io/file_bill_content_reader.hpp
#ifndef BILLS_IO_ADAPTERS_IO_FILE_BILL_CONTENT_READER_H_
#define BILLS_IO_ADAPTERS_IO_FILE_BILL_CONTENT_READER_H_

#include "ports/bills_content_reader.hpp"

class FileBillContentReader : public BillContentReader {
 public:
  auto Read(const std::string& file_path) -> std::string override;
};

#endif  // BILLS_IO_ADAPTERS_IO_FILE_BILL_CONTENT_READER_H_
