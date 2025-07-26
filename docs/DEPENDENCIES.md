# 依赖项 (Dependencies)

本项目依赖于以下优秀的开源库：

* [cite_start]**[nlohmann/json](https://github.com/nlohmann/json)** 
    * [cite_start]**用途**: 用于项目中所有 JSON 格式数据的解析和序列化（例如加载 `Validator_Config.json` 配置文件）。 
    * [cite_start]**许可证**: MIT License 

* [cite_start]**[SQLite C Library](https://www.sqlite.org/index.html)** 
    * [cite_start]**用途**: C++ 部分的代码直接使用 SQLite C API 进行数据库操作，包括打开/关闭数据库连接、执行 SQL 语句、准备和绑定参数等。 
    * [cite_start]**许可证**: Public Domain 

* [cite_start]**[Matplotlib](https://matplotlib.org/)** 
    * [cite_start]**用途**: 用于生成柱状图，可视化父级支出的汇总数据。 
    * [cite_start]**许可证**: Matplotlib License (BSD-style)