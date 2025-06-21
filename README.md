# 1 Bills_Master_cpp
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
├── parsing/                    # 处理输入文本文件的格式验证和逻辑解析。
│   ├── LineValidator.h         # 声明用于验证单行文本格式的 LineValidator 类。
│   ├── LineValidator.cpp       # 使用正则表达式实现行验证的具体逻辑。
│   ├── Bill_Parser.h           # 声明用于解析文件整体逻辑结构的 Bill_Parser 类。
│   └── Bill_Parser.cpp         # 实现文件解析逻辑，并调用 LineValidator 进行格式检查。
│
├── database/                   # 管理所有与 SQLite 数据库的交互。
│   ├── DatabaseInserter.h      # 声明负责创建数据库结构和插入数据的 DatabaseInserter 类。
│   └── DatabaseInserter.cpp    # 使用事务和预备语句实现高性能的数据插入逻辑。
│
└── reporting/                  # 包含所有查询数据库和生成用户报告的逻辑。
    ├── BillReporter.h          # 声明用于生成所有报告的 BillReporter 类，并定义报告所需的数据结构。
    └── BillReporter.cpp        # 实现具体的 SQL 查询和逻辑，以格式化并显示各种分析报告。
```
# 2 程序结构
## 2.1 main.cpp
```
C++ 账单管理系统 (main.cpp)
 │
 ├─ 核心依赖与数据结构
 │   ├─ ParsedRecord.h  (定义了在解析器和数据库插入器之间传递的数据结构)
 │   └─ sqlite3.h       (外部依赖：SQLite 数据库 C 语言接口)
 │
 ├─ 功能分支 1: 数据导入 (用户选择 0)
 │   │
 │   ├─ 1. 创建 LineValidator (LineValidator.h/.cpp)
 │   │   └─ 职责: 使用正则表达式验证单行文本的格式 (如 DATE, PARENT, item 等)。
 │   │
 │   ├─ 2. 创建 Bill_Parser (Bill_Parser.h/.cpp)
 │   │   │
 │   │   ├─ 依赖注入: (uses -->) LineValidator
 │   │   │   └─ Bill_Parser 在构造时接收一个 LineValidator 的引用，将验证逻辑委托给它。
 │   │   │
 │   │   └─ 工作流程:
 │   │       ├─ a. 读取 .txt 文件。
 │   │       ├─ b. 对每一行调用 LineValidator::validate()。
 │   │       └─ c. 将验证通过的行转换成一个 `ParsedRecord` 对象。
 │   │
 │   └─ 3. 创建 DatabaseInserter (DatabaseInserter.h/.cpp)
 │       │
 │       ├─ 数据流: (receives -->) std::vector<ParsedRecord>
 │       │   └─ 从 Bill_Parser 接收解析好的记录列表。
 │       │
 │       └─ 工作流程:
 │           ├─ a. 创建数据库表结构 (如果不存在)。
 │           ├─ b. 开始一个数据库事务。
 │           ├─ c. 使用高性能的预备语句 (Prepared Statements) 将 `ParsedRecord` 批量插入到 SQLite 数据库中。
 │           └─ d. 提交或回滚事务。
 │
 └─ 功能分支 2: 查询与报告 (用户选择 1-4)
     │
     └─ 创建 BillReporter (BillReporter.h/.cpp)
         │
         ├─ 依赖: (uses -->) SQLite 数据库 (bills.db)
         │   └─ 直接连接并查询数据库以获取数据。
         │
         ├─ 内部数据结构:
         │   ├─ ParentData
         │   ├─ ChildData
         │   └─ ItemData
         │       └─ 用于将从数据库中查询到的扁平化数据重构成层级结构，便于报告。
         │
         └─ 工作流程:
             ├─ a. 根据用户选择 (query_1, query_2, ...)，构造相应的 SQL 查询语句。
             ├─ b. 执行 SQL 并获取结果。
             ├─ c. 将查询结果填充到内部的层级数据结构中。
             └─ d. 将格式化后的报告输出到控制台。
```
