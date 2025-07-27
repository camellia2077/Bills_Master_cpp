# 目录结构 (Directory Structure)

```
Bills_Master/
├── main.cpp                              # 应用程序的交互式菜单主程序入口 
├── main_command.cpp                      # 应用程序的命令行接口主程序入口 
├── build.sh                              # 用于编译和构建项目的 Shell 脚本 
├── CMakeLists.txt  
│
├── app_controller/                       # 应用程序控制器模块，负责协调各个功能模块，处理用户请求 
│   ├── AppController.cpp                 # AppController 类的实现，包含业务逻辑 
│   ├── AppController.h                   # AppController 类的头文件，定义了应用程序的核心接口 
│   └── ProcessStats.h                    # 用于跟踪和汇总处理操作（成功/失败）的统计数据结构 
│
├── common/                               # 通用工具和定义，供整个项目共享 
│   ├── common_utils.h                    # 包含通用工具函数和宏定义，如控制台颜色代码 
│   └── version.h                         # 定义应用程序的版本信息 
│
├── config/                               # 存放应用程序的配置文件 
│   ├── Modifier_Config.json              # 账单修改器的配置，定义修改规则和格式化设置 
│   └── Validator_Config.json             # 账单验证器的配置，定义账单分类和验证规则 
│
├── db_insert/                            # 数据库插入模块，处理账单数据到 SQLite 数据库的导入 
│   ├── DataProcessor.cpp                 # DataProcessor 类的实现，协调账单解析和数据库插入 
│   ├── DataProcessor.h                   # DataProcessor 类的头文件 
│   ├── bill_structures/                  # 数据库插入模块的数据结构定义 
│   │   └── BillStructures.h              # 定义了用于数据库插入的账单相关数据结构
│   ├── insertor/                         # 数据库插入器 
│   │   ├── BillInserter.cpp              # BillInserter 类的实现，处理数据库连接和数据插入操作 
│   │   └── BillInserter.h                # BillInserter 类的头文件 
│   └── parser/                           # 账单解析器 
│       ├── BillParser.cpp                # BillParser 类的实现，负责解析原始账单文件 
│       └── BillParser.h                  # BillParser 类的头文件 
│
├── file_handler/                         # 文件处理模块，用于查找和管理文件 
│   ├── FileHandler.cpp                   # FileHandler 类的实现，包含文件查找逻辑 
│   └── FileHandler.h                     # FileHandler 类的头文件 
│
├── query
│   ├── export_bills
│   │   ├── ReportExporter.cpp
│   │   └── ReportExporter.h
│   ├── month
│   │   ├── _month_data
│   │   │   └── ReportData.h
│   │   ├── month_format
│   │   │   ├── IMonthReportFormatter.h
│   │   │   ├── month_md
│   │   │   │   ├── MonthMdFormat.cpp
│   │   │   │   └── MonthMdFormat.h
│   │   │   ├── month_rst
│   │   │   │   ├── MonthRstFormat.cpp
│   │   │   │   └── MonthRstFormat.h
│   │   │   ├── month_tex
│   │   │   │   ├── MonthTexFormat.cpp
│   │   │   │   ├── MonthTexFormat.h
│       │   │   └── MonthTexReport.h # 格式的结构体
│   │   │   ├── month_typ
│   │   │   │   ├── MonthTypFormat.cpp
│   │   │   │   └── MonthTypFormat.h
│   │   │   ├── ReportFormatterFactory.cpp 
│   │   │   └── ReportFormatterFactory.h
│   │   ├── month_query
│   │   │   ├── MonthQuery.cpp
│   │   │   └── MonthQuery.h
│   │   ├── MonthlyReportGenerator.cpp
│   │   └── MonthlyReportGenerator.h
│   ├── QueryDb.cpp
│   ├── QueryDb.h
│   ├── ReportFormat.h
│   └── year/
│       ├── _year_data/
│       │   └── YearlyReportData.h
│       ├── year_format/
│       │   ├── IYearlyReportFormatter.h
│       │   ├── year_md/ # Markdown格式
│       │   │   ├── YearMdFormat.cpp
│       │   │   └── YearMdFormat.h
│       │   ├── year_rst/ # reST格式
│       │   │   ├── YearRstFormat.cpp
│       │   │   └── YearRstFormat.h
│       │   ├── year_tex/ # LaTeX格式
│       │   │   ├── YearTexFormat.cpp
│       │   │   ├── YearTexFormat.h
│       │   │   └── YearTexReport.h # 格式的结构体
│       │   ├── year_typ/ # typst格式
│       │   │   ├── YearTypFormat.cpp
│       │   │   └── YearTypFormat.h
│       │   ├── YearlyReportFormatterFactory.cpp
│       │   └── YearlyReportFormatterFactory.h
│       ├── year_query/
│       │   ├── YearlyDataReader.cpp
│       │   └── YearlyDataReader.h
│       ├── YearlyReportGenerator.cpp
│       └── YearlyReportGenerator.h
│  
└── reprocessing/
    ├── CMakeLists.txt # py封装的cmake
    ├── compile_ucrt64.bat
    ├── modifier/
    │   ├── _shared_structures/
    │   │   └── BillDataStructures.h
    │   ├── BillModifier.cpp
    │   ├── BillModifier.h
    │   ├── config_loader/
    │   │   ├── ConfigLoader.cpp
    │   │   └── ConfigLoader.h
    │   ├── processor/
    │   │   ├── BillContentTransformer.cpp # 内容转换
    │   │   └── BillContentTransformer.h
    │   └── raw_format/
    │       ├── BillFormatter.cpp
    │       └── BillFormatter.h
    ├── Reprocessor.cpp
    ├── Reprocessor.h
    ├── validator/
    │   ├── _internal/
    │   │   ├── BillConfig.cpp
    │   │   ├── BillConfig.h
    │   │   ├── BillFormatVerifier.cpp
    │   │   ├── BillFormatVerifier.h
    │   │   ├── ValidationResult.cpp
    │   │   └── ValidationResult.h
    │   ├── BillValidator.cpp
    │   └── BillValidator.h
    └── wrapper.cpp # py封装
```
