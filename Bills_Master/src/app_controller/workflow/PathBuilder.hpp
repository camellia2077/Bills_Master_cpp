// app_controller/PathBuilder.hpp
#ifndef PATH_BUILDER_HPP
#define PATH_BUILDER_HPP

#include <string>
#include <filesystem>

namespace fs = std::filesystem;

// Forward declaration
class FileHandler;

class PathBuilder {
public:
    PathBuilder(const std::string& base_output_dir, FileHandler& file_handler);
    fs::path build_output_path(const fs::path& input_file) const;

private:
    fs::path m_base_output_dir;
    FileHandler& m_file_handler; // 改为引用成员
};

#endif // PATH_BUILDER_HPP