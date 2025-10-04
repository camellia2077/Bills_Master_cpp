// command_handler/CommandFacade.hpp
#ifndef COMMAND_FACADE_HPP
#define COMMAND_FACADE_HPP

#include <vector>
#include <string>
#include <map>
#include <memory>
#include "command_handler/commands/interface/ICommand.hpp"

class CommandFacade {
public:
    CommandFacade() = default;

    /**
     * @brief 运行命令行处理器。
     * @param argc 参数数量，来自 main 函数。
     * @param argv 参数值数组，来自 main 函数。
     * @return 程序的退出码 (0 表示成功, 1 表示失败)。
     */
    int run(int argc, char* argv[]);

private:
    void register_commands(const std::string& format, const std::string& type_filter);
    
    // 命令注册表
    std::map<std::string, std::unique_ptr<ICommand>> m_commands;
};

#endif // COMMAND_FACADE_HPP