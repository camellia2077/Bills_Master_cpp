# 架构设计 (Architecture)

## 整体架构

项目的核心流程由 `AppController` 驱动，它负责调度各个功能模块以响应用户请求。主要模块包括预处理、数据库插入和查询。

```mermaid
graph TD
    subgraph "核心流程"
        A["程序入口 (main.cpp)"] --> B["应用控制器 (AppController)"];
        B --> C["预处理模块 (Reprocessing)"];
        B --> D["数据库插入模块 (DB Insert)"];
        B --> E["数据库查询模块 (Query)"];
    end

    subgraph "支持模块与资源"
        F["配置文件 (Config)"] --> C;
        G["通用工具 (Common)"];
        H[("数据库 (SQLite)")]
    end
    
    C --> G;
    D --> G;
    E --> G;

    D -- "写入数据" --> H;
    E -- "读取数据" --> H;

    classDef core fill:#e6f3ff,stroke:#367dcc,stroke-width:2px;
    classDef support fill:#f0f0f0,stroke:#666,stroke-width:1px,stroke-dasharray: 5 5;
    
    class A,B,C,D,E core;
    class F,G,H support;
````

**程序入口 (`main.cpp`, `main_command.cpp`)**: 应用程序的起点，分别用于交互式菜单和命令行界面。 
**应用控制器 (`AppController`)**: 作为业务逻辑的核心，协调其他模块完成特定任务。 
**预处理模块 (`Reprocessing`)**: 负责账单的验证和修改。 
**数据库插入模块 (`DB Insert`)**: 将预处理后的数据解析并存入 SQLite 数据库。 
**数据库查询模块 (`Query`)**: 从数据库中检索数据并生成指定格式的报告。 
**支持模块**:
**配置文件 (`Config`)**: 为预处理模块提供规则。 
**通用工具 (`Common`)**: 提供贯穿整个项目的工具函数和定义。 
**数据库 (`SQLite`)**: 作为项目的数据持久化存储。 
