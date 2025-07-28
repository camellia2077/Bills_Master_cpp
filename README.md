# bills_master

对账单文件进行预处理、验证、分析和入数据库，并且导出多种格式(md、tex、typ、rst)。主要使用c++开发。

## ✨ 主要功能

* **账单预处理**: 对txt内容进行验证和修改,可使用json传入合法性验证和修改的配置
* **数据库集成**: 解析账单数据并将其导入 SQLite 数据库。
* **报告生成**: 查询数据库，以 Markdown、LaTeX 和 Typst 等多种格式生成详细的月度和年度报告。
* **数据可视化**: 查询数据库，生成分析图表。

## 🚀 快速开始

1.  **克隆仓库:**
    ```bash
    git clone git@github.com:camellia2077/Bills_Master_cpp.git
    cd Bills_Master
    ```

2.  **编译项目:**
    ```bash
    ./build.sh
    ```

3.  **运行程序:**
    * **交互式菜单**: `./build/bin/bill_matser_app`
    * **命令行界面**: `./build/bin/bill_master_cli`

## 📚 文档

更详细的文档信息，请参阅 `docs` 目录：

* **[依赖项 (Dependencies)](docs/DEPENDENCIES.md)**: 项目使用的开源库。
* **[架构设计 (Architecture)](docs/ARCHITECTURE.md)**: 项目的整体架构图
* **[目录结构 (Directory Structure)](docs/DATABASE_SCHEMA.md)**: 数据库表结构
* **[目录结构 (Directory Structure)](docs/DIRECTORY_STRUCTURE.md)**: 项目源代码的详细目录结构。
* **[图表配置 (Graph Configuration)](docs/GRAPH_CONFIGURATION.md)**: 关于图表生成脚本的指令说明。