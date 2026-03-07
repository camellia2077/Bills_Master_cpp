// bills_io/adapters/io/file_bill_file_enumerator.hpp
#ifndef BILLS_IO_ADAPTERS_IO_FILE_BILL_FILE_ENUMERATOR_H_
#define BILLS_IO_ADAPTERS_IO_FILE_BILL_FILE_ENUMERATOR_H_

#include "ports/bills_file_enumerator.hpp"

class FileBillFileEnumerator : public BillFileEnumerator {
 public:
  auto ListFilesByExtension(std::string_view root_path,
                            std::string_view extension)
      -> std::vector<std::filesystem::path> override;
};

#endif  // BILLS_IO_ADAPTERS_IO_FILE_BILL_FILE_ENUMERATOR_H_
