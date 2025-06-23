# 依赖项 (Dependencies)

本项目依赖于以下优秀的开源库：

* **[nlohmann/json](https://github.com/nlohmann/json)**
    * **用途**: 用于项目中所有 JSON 格式数据的解析和序列化（例如加载 `Validator_Config.json` 配置文件）。
    * **许可证**: MIT License

* **[json (Python standard library)](https://docs.python.org/3/library/json.html)**
    * **用途**: 用于加载 `generate_report.json` 配置文件，以及处理项目中涉及到的 JSON 格式数据。
    * **许可证**: Python Software Foundation License (PSF)

* **[sqlite3 (Python standard library)](https://docs.python.org/3/library/sqlite3.html)**
    * **用途**: 用于连接到 `bills.db` SQLite 数据库，并执行 SQL 查询以获取账单数据。
    * **许可证**: Python Software Foundation License (PSF)

* **[SQLite C Library](https://www.sqlite.org/index.html)**
    * **用途**: C++ 部分的代码直接使用 SQLite C API 进行数据库操作，包括打开/关闭数据库连接、执行 SQL 语句、准备和绑定参数等。
    * **许可证**: Public Domain

* **[Matplotlib](https://matplotlib.org/)**
    * **用途**: 用于生成柱状图，可视化父级支出的汇总数据。
    * **许可证**: Matplotlib License (BSD-style)

# 1 Bills_Master
## 1.1 structure
```
/
├── CMakeLists.txt              # 管理整个项目的构建过程，配置编译器和链接器选项。
├── build.sh                    # 一个用于自动化清理和构建流程的 Shell 脚本。
├── main.cpp                    # 程序主入口，处理用户菜单交互并协调各个模块。
│
├── common/                     # 包含在不同模块间共享的数据结构。
│   └── ParsedRecord.h          # 定义了用于在解析器和数据库模块间传递数据的核心结构体。
│
├── database/                   # 管理所有与 SQLite 数据库的交互。
│   ├── DatabaseInserter.h      # 声明负责创建数据库结构和插入数据的 DatabaseInserter 类。
│   └── DatabaseInserter.cpp    # 使用事务和预备语句实现高性能的数据插入逻辑。
│
├── parsing/                    # 处理输入文本文件的格式验证和逻辑解析。
│   ├── LineValidator.h         # 声明用于验证单行文本格式的 LineValidator 类。
│   ├── LineValidator.cpp       # 使用正则表达式实现行验证的具体逻辑。
│   ├── Bill_Parser.h           # 声明用于解析文件整体逻辑结构的 Bill_Parser 类。
│   └── Bill_Parser.cpp         # 实现文件解析逻辑，并调用 LineValidator 进行格式检查。
│
└── queriesg/                  # 包含所有查询数据库和生成用户报告的逻辑。
    ├── Bill_Queries.h          # 声明用于生成所有报告的 BillQueries 类，并定义报告所需的数据结构。
    └── Bill_Queries.cpp        # 实现具体的 SQL 查询和逻辑，以格式化并显示各种分析报告。
```

# 2 graph
## Configuration

The script reads its configuration from a `generate_report.json` file. If this file is not found or is invalid, default settings will be used.

### generate_report.json` Example:

```
{
  "font_sizes": {
    "title": 20,
    "axis_label": 14,
    "tick_label": 12,
    "bar_label": 10
  }
}
```
font_sizes: An object containing font size settings for different chart components:

title: Font size for the main chart title.主图表标题的字体大小。

axis_label: Font size for the X and Y axis labels.X 轴和 Y 轴标签的字体大小。

tick_label: Font size for the axis tick labels.坐标轴刻度标签的字体大小。

bar_label: Font size for the labels displayed on each bar (amount and percentage).显示在每个条形上的标签（金额和百分比）的字体大小。

