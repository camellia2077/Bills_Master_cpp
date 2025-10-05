// command_handler/commands/SimpleCommand.hpp
#ifndef SIMPLE_COMMAND_HPP
#define SIMPLE_COMMAND_HPP

#include "command_handler/commands/interface/ICommand.hpp"
#include <functional>

// 一个用于处理简单命令（如 validate, modify, import）的通用类
class SimpleCommand : public ICommand {
public:
    using Action = std::function<bool(AppController&, const std::string&)>;

    SimpleCommand(std::string command_name, Action action);
    bool execute(const std::vector<std::string>& args, AppController& controller) override;

private:
    std::string m_command_name;
    Action m_action;
};

#endif // SIMPLE_COMMAND_HPP