// ports/bills_file_enumerator.hpp
#ifndef PORTS_BILLS_FILE_ENUMERATOR_H_
#define PORTS_BILLS_FILE_ENUMERATOR_H_

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

#endif  // PORTS_BILLS_FILE_ENUMERATOR_H_
