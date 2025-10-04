// command_handler/commands/SimpleCommand.cpp
#include "SimpleCommand.hpp"
#include <stdexcept>

SimpleCommand::SimpleCommand(std::string command_name, Action action)
    : m_command_name(std::move(command_name)), m_action(std::move(action)) {}

bool SimpleCommand::execute(const std::vector<std::string>& args, AppController& controller) {
    if (args.empty()) {
        throw std::runtime_error("Missing path for '" + m_command_name + "' command.");
    }
    // 调用传入的 AppController 成员函数
    return m_action(controller, args[0]);
}