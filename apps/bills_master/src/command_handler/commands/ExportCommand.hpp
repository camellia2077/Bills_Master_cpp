// command_handler/commands/ExportCommand.hpp
#ifndef EXPORT_COMMAND_HPP
#define EXPORT_COMMAND_HPP

#include "command_handler/commands/interface/ICommand.hpp"

class ExportCommand : public ICommand {
public:
    // 通过构造函数接收全局选项
    ExportCommand(std::string format, std::string type_filter);
    bool execute(const std::vector<std::string>& args, AppController& controller) override;

private:
    bool handle_export_all(AppController& controller);
    bool handle_export_date(const std::vector<std::string>& values, AppController& controller);

    std::string m_format_str;
    std::string m_type_filter;
};

#endif // EXPORT_COMMAND_HPP