# 依赖项 (Dependencies)

本项目依赖于以下优秀的开源库：

* **[nlohmann/json](https://github.com/nlohmann/json)** 
    * **用途**: 用于项目中所有 JSON 格式数据的解析和序列化（例如加载 `Validator_Config.json` 配置文件）。 
    * **许可证**: MIT License 

* **[SQLite C Library](https://www.sqlite.org/index.html)** 
    * **用途**: C++ 部分的代码直接使用 SQLite C API 进行数据库操作，包括打开/关闭数据库连接、执行 SQL 语句、准备和绑定参数等。 
    * **许可证**: Public Domain 

* **[Matplotlib](https://matplotlib.org/)** 
    * **用途**: 用于生成柱状图，可视化父级支出的汇总数据。 
    * **许可证**: Matplotlib License (BSD-style)