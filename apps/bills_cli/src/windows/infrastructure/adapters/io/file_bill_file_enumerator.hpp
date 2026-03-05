// adapters/io/FileBillFileEnumerator.hpp
#ifndef FILE_BILL_FILE_ENUMERATOR_HPP
#define FILE_BILL_FILE_ENUMERATOR_HPP

#include "ports/bills_file_enumerator.hpp"

class FileBillFileEnumerator : public BillFileEnumerator {
 public:
  FileBillFileEnumerator() = default;

  auto ListFilesByExtension(std::string_view root_path,
                            std::string_view extension)
      -> std::vector<std::filesystem::path> override;
};

#endif  // FILE_BILL_FILE_ENUMERATOR_HPP


