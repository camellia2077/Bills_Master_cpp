// windows/infrastructure/adapters/io/file_bill_file_enumerator.hpp
#ifndef WINDOWS_INFRASTRUCTURE_ADAPTERS_IO_FILE_BILL_FILE_ENUMERATOR_H_
#define WINDOWS_INFRASTRUCTURE_ADAPTERS_IO_FILE_BILL_FILE_ENUMERATOR_H_

#include "ports/bills_file_enumerator.hpp"

class FileBillFileEnumerator : public BillFileEnumerator {
 public:
  FileBillFileEnumerator() = default;

  auto ListFilesByExtension(std::string_view root_path,
                            std::string_view extension)
      -> std::vector<std::filesystem::path> override;
};

#endif  // WINDOWS_INFRASTRUCTURE_ADAPTERS_IO_FILE_BILL_FILE_ENUMERATOR_H_


