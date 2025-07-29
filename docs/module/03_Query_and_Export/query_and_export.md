好的，这是一份根据您提供的 C++ 文件和 CMake 构建脚本生成的详细中文技术文档，专门针对**数据库查询与报告导出**功能。

---

## **查询与报告导出模块 - 综合技术文档**

本文档旨在为开发人员提供一个全面、深入的指南，详细介绍账单查询与报告导出系统的架构、设计原则、核心组件及其工作流程。该系统具备高度的模块化和可扩展性，通过动态插件机制支持多种报告格式的生成。

### 1. 核心架构与设计理念

系统的查询与导出功能围绕一个核心外观（Facade）类 `QueryFacade` 构建，它统一了所有对外的接口调用。整体设计遵循**关注点分离 (Separation of Concerns)** 和**依赖倒置原则 (Dependency Inversion Principle)**。

-   **外观层 (`QueryFacade`)**: 作为用户与系统的唯一交互点，封装了内部复杂的子系统调用，如报告生成、插件加载和文件导出。
-   **生成器层 (`MonthlyReportGenerator`, `YearlyReportGenerator`)**: 分别负责月度报告和年度报告的生成流程，协调数据读取和格式化 。
-   **数据读取层 (`MonthQuery`, `YearlyDataReader`)**: 专职从 SQLite 数据库中提取和聚合数据，为生成器提供统一的数据结构。
-   **插件化格式化层**: 这是系统最具扩展性的部分。报告的最终样式（如 Markdown, LaTeX, reStructuredText, Typst）由独立的动态链接库（插件）决定。
    -   **插件管理器 (`MonthlyReportFormatterPluginManager`, `YearlyReportFormatterPluginManager`)**: 负责在运行时动态发现、加载和管理这些格式化器插件。
    -   **格式化器接口 (`IMonthReportFormatter`, `IYearlyReportFormatter`)**: 定义了所有格式化器插件必须遵守的统一接口，实现了对扩展的开放。

### 2. 组件详解

#### 2.1 查询外观 - `QueryFacade`

`QueryFacade` 是整个查询系统的总入口，它简化了客户端代码的调用。

-   **初始化**: 构造函数接收数据库路径和插件路径（可以是单个目录或一个文件列表），并建立与数据库的连接。
-   **核心 API**:
    -   `get_monthly_details_report(year, month, format_name)`: 生成指定年月的详细月度报告。
    -   `get_yearly_summary_report(year, format_name)`: 生成指定年份的年度摘要报告。
    -   `get_all_bill_dates()`: 从数据库获取所有存在账单记录的日期（格式 "YYYYMM"），主要用于“全部导出”功能。
-   **导出功能**:
    -   `export_monthly_report`, `export_yearly_report`, `export_all_reports`: 这些方法调用核心 API 生成报告内容，并将其保存到文件中。文件会根据格式和类型被保存在结构化的目录中，例如 `exported_files/Markdown_bills/months/2023/`。

#### 2.2 报告生成器 - `MonthlyReportGenerator` & `YearlyReportGenerator`

这两类生成器的工作流程高度一致，体现了良好的设计复用。

1.  **数据读取**: 调用相应的数据读取器（`MonthQuery` 或 `YearlyDataReader`）从数据库获取原始数据，并填充到预定义的数据结构中 (`MonthlyReportData` 或 `YearlyReportData`) 。
2.  **格式化器创建**: 向其持有的插件管理器 (`...PluginManager`) 请求一个特定格式的格式化器实例（例如，请求 "md" 格式） 。
3.  **报告格式化**: 如果成功获取到格式化器，则调用其 `format_report` 方法，传入之前获取的数据，生成最终的报告字符串 。
4.  **异常处理**: 如果找不到指定格式的插件，会抛出运行时异常，通知调用方 。

#### 2.3 数据读取器 - `MonthQuery` & `YearlyDataReader`

-   **`MonthQuery`**:
    -   通过 SQL 查询连接 `transactions` 和 `bills` 表，获取指定年月的每一笔交易记录。
    -   在代码层面进行数据聚合，计算每个父类、子类的总金额，并将所有交易记录组织到 `MonthlyReportData` 结构中。
-   **`YearlyDataReader`**:
    -   其 SQL 查询使用 `GROUP BY b.month` 和 `SUM(t.amount)` 来直接从数据库层面计算出每月的总支出。
    -   将结果（年度总支出、每月总支出）填充到 `YearlyReportData` 结构中。

#### 2.4 数据排序器 - `ReportSorter`

这是一个静态工具类，用于对月度报告的数据进行统一排序，将排序逻辑从格式化器中解耦 。

-   **排序规则**:
    1.  对每个子类别内的交易，按金额从高到低排序 。
    2.  对所有父类别，按其总金额从高到低排序 。
-   **应用**: 所有月度报告格式化器（如 `MonthMdFormat`, `MonthTexFormat` 等）在格式化数据前，都会先调用 `ReportSorter::sort_report_data` 以获得一致的、有序的数据视图 。

### 3. 动态插件系统

这是本模块最具扩展性的核心。它允许开发者通过简单地添加新的动态库（`.dll` 或 `.so`）来增加新的报告格式，而无需修改或重新编译主程序。

#### 3.1 插件管理器 (`...PluginManager`)

-   **插件发现**: 管理器可以根据构造时传入的路径，扫描整个目录来加载所有符合命名规范的插件。它也可以被配置为仅加载一个明确指定的插件文件列表。
-   **命名约定**:
    -   插件文件名必须遵循 `format_` + `type_` + `formatter` 的格式，例如 `md_month_formatter.dll` 或 `tex_year_formatter.so`。管理器通过检查文件名中的 `_month_formatter` 或 `_year_formatter` 后缀来识别插件类型。
-   **加载机制**:
    1.  使用平台特定的 API（Windows 上的 `LoadLibraryA`，Linux/macOS 上的 `dlopen`）加载动态库。
    2.  根据文件名推断出期望的**工厂函数**名，例如从 `md_month_formatter.dll` 推断出 `create_md_month_formatter` 。
    3.  使用 `GetProcAddress` 或 `dlsym` 在库中查找该工厂函数。
    4.  如果找到，就将库的句柄和函数指针存储在一个 `std::map` 中，以格式名（如 "md"）为键。
-   **实例创建**: 当 `createFormatter` 方法被调用时，管理器在 map 中查找对应的工厂函数，执行它，然后返回一个指向 `I...ReportFormatter` 接口的 `std::unique_ptr`。

#### 3.2 插件的实现

每个插件都是一个独立的动态库，其内部实现遵循以下模式：

1.  **实现接口**: 创建一个类（如 `MonthMdFormat`），并公有继承自相应的接口（`IMonthReportFormatter`）。
2.  **实现 `format_report`**: 实现接口中定义的纯虚函数 `format_report`，编写将数据结构转换为目标格式字符串的逻辑 。
3.  **提供工厂函数**:
    -   在源文件的末尾，提供一个 `extern "C"` 的、具有唯一名称的工厂函数（如 `create_md_month_formatter`） 。
    -   此函数体内仅有一行代码：`return new YourFormattterClass();`。
    -   使用平台特定的宏（如 `__declspec(dllexport)`）将此函数导出，使其对外部可见 。
4.  **可配置性 (可选但推荐)**:
    -   为格式化器定义一个配置结构体（如 `MonthMdConfig`），将所有可变的标签、符号、样式参数放入其中 。
    -   格式化器类持有一个该配置结构体的实例，并在 `format_report` 中使用它 。
    -   这使得用户可以在不触及 C++ 代码的情况下，仅通过修改配置对象来定制报告的外观。

#### 3.3 插件的构建 (`CMakeLists.txt`)

所有插件的 `CMakeLists.txt` 文件都遵循相似的配置 ：

-   `add_library(target_name SHARED ...)`: 将插件构建为共享库 。
-   `target_include_directories(...)`: 添加主项目的根目录作为包含路径，以找到接口头文件等 。
-   `set_target_properties(... PROPERTIES PREFIX "")`: 这是一个关键设置，它移除了库文件名的标准前缀（如 Linux 上的 `lib`），确保在所有平台上都能生成一致的文件名（例如 `md_month_formatter.dll` 而不是 `libmd_month_formatter.so`），这对于插件管理器的加载逻辑至关重要 。

### 4. 总结

本查询与导出模块是一个设计精良、高度解耦的系统。其核心优势在于强大的插件化架构，它不仅使得添加新报告格式变得异常简单，也保证了核心逻辑的稳定与清晰。通过将数据获取、排序、格式化等不同职责清晰地分离到各自的组件中，整个系统表现出优秀的维护性和可扩展性。