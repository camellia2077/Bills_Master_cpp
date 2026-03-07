// ports/bills_content_reader.hpp
#ifndef PORTS_BILLS_CONTENT_READER_H_
#define PORTS_BILLS_CONTENT_READER_H_

#include <string>

class BillContentReader {
 public:
  virtual ~BillContentReader() = default;
  virtual auto Read(const std::string& file_path) -> std::string = 0;
};

#endif  // PORTS_BILLS_CONTENT_READER_H_
