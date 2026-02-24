// adapters/io/FileBillFileEnumerator.hpp
#ifndef FILE_BILL_FILE_ENUMERATOR_HPP
#define FILE_BILL_FILE_ENUMERATOR_HPP

#include "file_handler/FileHandler.hpp"
#include "ports/BillFileEnumerator.hpp"

class FileBillFileEnumerator : public BillFileEnumerator {
 public:
  explicit FileBillFileEnumerator(FileHandler& file_handler);

  auto ListFilesByExtension(std::string_view root_path,
                            std::string_view extension)
      -> std::vector<std::filesystem::path> override;

 private:
  FileHandler& file_handler_;
};

#endif  // FILE_BILL_FILE_ENUMERATOR_HPP
