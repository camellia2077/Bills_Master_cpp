// ports/bills_file_enumerator.hpp
#ifndef BILL_FILE_ENUMERATOR_HPP
#define BILL_FILE_ENUMERATOR_HPP

#include <filesystem>
#include <string_view>
#include <vector>

class BillFileEnumerator {
 public:
  virtual ~BillFileEnumerator() = default;
  virtual auto ListFilesByExtension(std::string_view root_path,
                                    std::string_view extension)
      -> std::vector<std::filesystem::path> = 0;
};

#endif  // BILL_FILE_ENUMERATOR_HPP
