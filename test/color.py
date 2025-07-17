def print_color_demo():
    # 定义重置颜色
    RESET = "\033[0m"

    colors = {
        "BLACK_COLOR": "\033[30m",
        "RED_COLOR": "\033[31m",
        "GREEN_COLOR": "\033[32m",
        "YELLOW_COLOR": "\033[33m",
        "BLUE_COLOR": "\033[34m",
        "MAGENTA_COLOR": "\033[35m",
        "CYAN_COLOR": "\033[36m",
        "WHITE_COLOR": "\033[37m",

        "BOLD_BLACK_COLOR": "\033[1;30m",
        "BOLD_RED_COLOR": "\033[1;31m",
        "BOLD_GREEN_COLOR": "\033[1;32m",
        "BOLD_YELLOW_COLOR": "\033[1;33m",
        "BOLD_BLUE_COLOR": "\033[1;34m",
        "BOLD_MAGENTA_COLOR": "\033[1;35m",
        "BOLD_CYAN_COLOR": "\033[1;36m",
        "BOLD_WHITE_COLOR": "\033[1;37m",

        "BG_BLACK_COLOR": "\033[40m",
        "BG_RED_COLOR": "\033[41m",
        "BG_GREEN_COLOR": "\033[42m",
        "BG_YELLOW_COLOR": "\033[43m",
        "BG_BLUE_COLOR": "\033[44m",
        "BG_MAGENTA_COLOR": "\033[45m",
        "BG_CYAN_COLOR": "\033[46m",
        "BG_WHITE_COLOR": "\033[47m",

        "BRIGHT_BLACK_COLOR": "\033[90m",
        "BRIGHT_RED_COLOR": "\033[91m",
        "BRIGHT_GREEN_COLOR": "\033[92m",
        "BRIGHT_YELLOW_COLOR": "\033[93m",
        "BRIGHT_BLUE_COLOR": "\033[94m",
        "BRIGHT_MAGENTA_COLOR": "\033[95m",
        "BRIGHT_CYAN_COLOR": "\033[96m",
        "BRIGHT_WHITE_COLOR": "\033[97m",

        "BG_BRIGHT_BLACK_COLOR": "\033[100m",
        "BG_BRIGHT_RED_COLOR": "\033[101m",
        "BG_BRIGHT_GREEN_COLOR": "\033[102m",
        "BG_BRIGHT_YELLOW_COLOR": "\033[103m",
        "BG_BRIGHT_BLUE_COLOR": "\033[104m",
        "BG_BRIGHT_MAGENTA_COLOR": "\033[105m",
        "BG_BRIGHT_CYAN_COLOR": "\033[106m",
        "BG_BRIGHT_WHITE_COLOR": "\033[107m",
    }

    phrase = "The quick fox jumps over lazy dog"

    print("--- ANSI Color Code Demo (Simplified) ---")
    print("Each line below shows a color macro name and the phrase in that color:")
    print("-----------------------------------------------------------------")

    # 循环遍历所有颜色，直接应用并重置
    for name, code in colors.items():
        print(f"{name}: {code}{phrase}{RESET}") # 使用单独的 RESET 变量

    print("\n-----------------------------------------------------------------")
    print(f"RESET_COLOR: {RESET} (This code resets text formatting)") # 单独打印 RESET_COLOR 的说明
    print("注意：某些背景色可能会使文本难以阅读，具体取决于您的终端设置。")
    print("并非所有终端都支持所有 ANSI 转义代码或以相同方式显示它们。")


if __name__ == "__main__":
    print_color_demo()