#ifndef BILLS_IO_FILE_BILL_FILE_ENUMERATOR_HPP
#define BILLS_IO_FILE_BILL_FILE_ENUMERATOR_HPP

#include "ports/bills_file_enumerator.hpp"

class FileBillFileEnumerator : public BillFileEnumerator {
 public:
  auto ListFilesByExtension(std::string_view root_path,
                            std::string_view extension)
      -> std::vector<std::filesystem::path> override;
};

#endif  // BILLS_IO_FILE_BILL_FILE_ENUMERATOR_HPP
