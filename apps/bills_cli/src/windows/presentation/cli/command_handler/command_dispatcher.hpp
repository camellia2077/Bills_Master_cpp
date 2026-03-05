// command_handler/CommandDispatcher.hpp
#ifndef COMMAND_DISPATCHER_HPP
#define COMMAND_DISPATCHER_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>

class ICommand;
struct ICommandDeleter {
  auto operator()(ICommand* command) const -> void;
};

class CommandDispatcher {
 public:
  CommandDispatcher() = default;

  /**
   * @brief 运行命令行处理器。
   * @param argc 参数数量，来自 main 函数。
   * @param argv 参数值数组，来自 main 函数。
   * @return 程序的退出码 (0 表示成功, 1 表示失败)。
   */
  int run(int argc, char* argv[]);

 private:
  using CommandPtr = std::unique_ptr<ICommand, ICommandDeleter>;

  void register_commands(const std::string& format,
                         const std::string& type_filter,
                         const std::string& export_pipeline);

  // 命令注册表
  std::map<std::string, CommandPtr> m_commands;
};

#endif  // COMMAND_DISPATCHER_HPP
