// adapters/io/FileBillContentReader.cpp

#include "FileBillContentReader.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

auto FileBillContentReader::Read(const std::string& file_path) -> std::string {
  std::ifstream input_file(file_path);
  if (!input_file.is_open()) {
    throw std::runtime_error("无法打开输入账单文件 '" + file_path + "'");
  }
  std::stringstream buffer;
  buffer << input_file.rdbuf();
  return buffer.str();
}
