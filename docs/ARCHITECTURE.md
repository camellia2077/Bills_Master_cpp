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

