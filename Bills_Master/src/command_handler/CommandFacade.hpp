// command_handler/CommandFacade.hpp
#ifndef COMMAND_FACADE_HPP
#define COMMAND_FACADE_HPP

#include <vector>
#include <string>

/**
 * @class CommandFacade
 * @brief 使用 Facade 模式封装所有命令行参数的解析和处理逻辑。
 * 这是命令行界面的主要入口点。
 */
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
    /**
     * @brief 解析命令行参数。
     * @param argc 参数数量。
     * @param argv 参数值数组。
     */
    void parse_arguments(int argc, char* argv[]);

    /**
     * @brief 根据解析出的命令和参数执行相应的操作。
     * @return 如果操作成功则返回 true，否则返回 false。
     */
    bool execute_command();

    /**
     * @brief 处理所有类型的导出命令。
     * @return 如果操作成功则返回 true，否则返回 false。
     */
    bool handle_export_command();

private:
    std::vector<std::string> m_command_parts;
    std::string m_format_str = "md";
    std::string m_export_type_filter;
};

#endif // COMMAND_FACADE_HPP