// ports/bills_content_reader.hpp
#ifndef BILL_CONTENT_READER_HPP
#define BILL_CONTENT_READER_HPP

#include <string>

class BillContentReader {
 public:
  virtual ~BillContentReader() = default;
  virtual auto Read(const std::string& file_path) -> std::string = 0;
};

#endif  // BILL_CONTENT_READER_HPP
