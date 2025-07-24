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
│   │   └── BillStructures.h              # 定义了用于数据库插入的账单相关数据结构（如 Transaction, ParsedBill） 
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
├── query/                         # 查询和报告模块，用于从数据库查询数据并生成报告 
│   ├── QueryDb.cpp                # QueryFacade 类的实现，作为数据库查询的门面 
│   ├── QueryDb.h                  # QueryFacade 类的头文件 
│   ├── ReportFormat.h             # 定义报告的输出格式类型枚举（Markdown, LaTeX, Typst） 
│   ├── export/                    # 导出报告 
│   │   ├── ReportExporter.cpp  
│   │   └── ReportExporter.h  
│   ├── month/                     # 月度报告相关文件 
│   └── year/                      # 年度报告相关文件 
│  
└── reprocessing/                         # 预处理模块，包含账单的验证和修改功能 
├── Reprocessor.cpp                   # Reprocessor 类的实现，封装了验证和修改的流程 
├── Reprocessor.h                     # Reprocessor 类的头文件 
├── modifier/                         # 账单修改器 
└── validator/                        # 账单验证器 
```
