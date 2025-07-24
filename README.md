# Bills_Master

Bills_Master 是一款 C++ 应用程序，设计用于对账单文件进行预处理、验证、分析和入库，并能生成月度和年度的分析报告。

## ✨ 主要功能

* **账单预处理**: 基于 JSON 配置对原始账单文件进行验证和修改。
* **数据库集成**: 解析账单数据并将其导入 SQLite 数据库。
* **报告生成**: 查询数据库，以 Markdown、LaTeX 和 Typst 等多种格式生成详细的月度和年度报告。
* **数据可视化**: 基于报告数据生成分析图表。

## 🚀 快速开始

1.  **克隆仓库:**
    ```bash
    git clone <your-repository-url>
    cd Bills_Master
    ```

2.  **编译项目:**
    ```bash
    ./build.sh
    ```

3.  **运行程序:**
    * **交互式菜单**: `./build/bill_matser_app`
    * **命令行界面**: `./build/bill_master_cli`

## 📚 文档

更详细的文档信息，请参阅 `docs` 目录：

* **[依赖项 (Dependencies)](docs/DEPENDENCIES.md)**: 项目所使用的开源库列表。
* **[架构设计 (Architecture)](docs/ARCHITECTURE.md)**: 项目的整体架构和模块交互说明。
* **[目录结构 (Directory Structure)](docs/DIRECTORY_STRUCTURE.md)**: 项目源代码的目录结构详解。
* **[图表配置 (Graph Configuration)](docs/GRAPH_CONFIGURATION.md)**: 关于图表生成脚本的配置说明。