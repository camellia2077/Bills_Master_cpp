## **账单数据处理与数据库插入模块 - 技术文档**

本文档详细说明了账单处理模块的设计、功能与使用方法。该模块负责解析特定格式的文本账单文件，并将解析后的结构化数据高效、安全地存入 SQLite 数据库。

### 1\. 模块设计与架构

本模块采用三层分离的设计，将数据结构、文件解析和数据库操作解耦，以实现高内聚、低耦合和易于维护的目标。

  * [cite\_start]**数据结构层 (`bill_structures`)**: 定义了整个模块通用的核心数据结构，如交易记录 (`Transaction`) 和已解析的账单 (`ParsedBill`) [cite: 6]。
  * [cite\_start]**解析层 (`parser`)**: 负责读取原始账单文本文件，并根据预定义的格式规则将其转换为 `ParsedBill` 数据结构 [cite: 5]。
  * [cite\_start]**插入层 (`inserter`)**: 负责与 SQLite 数据库交互，包括创建表、管理事务以及将 `ParsedBill` 对象中的数据持久化到数据库中 [cite: 4]。
  * [cite\_start]**处理器 (`DataProcessor`)**: 作为模块的统一入口，封装了从文件解析到数据库插入的完整流程，为上层应用提供了简洁易用的接口 [cite: 1, 2]。

### 2.核心组件详解

#### 2.1 数据结构 - `BillStructures.h`

这是模块的基石，定义了两个核心 `struct`：

  * [cite\_start]**`Transaction`**: 代表一笔具体的交易记录 [cite: 6]。

      * `parent_category` (string): 交易所属的父类别 (如 "MEAL吃饭")。
      * `sub_category` (string): 交易所属的子类别 (如 "meal\_low")。
      * `amount` (double): 交易金额。
      * `description` (string): 交易的详细描述 (如 "饭")。

  * [cite\_start]**`ParsedBill`**: 代表一个被完整解析的账单文件 [cite: 6]。

      * `date` (string): 格式为 "YYYYMM" 的日期字符串。
      * `year` (int): 从 `date` 解析出的年份。
      * `month` (int): 从 `date` 解析出的月份。
      * `remark` (string): 账单的备注信息。
      * `transactions` (vector\<Transaction\>): 该账单包含的所有交易记录的列表。

#### 2.2 文件解析器 - `BillParser`

[cite\_start]`BillParser` 的核心职责是实现 `parse` 方法，将一个账单文件的路径作为输入，返回一个 `ParsedBill` 对象 [cite: 5]。

**解析逻辑**:

1.  [cite\_start]**打开文件**: 尝试打开指定路径的文件，如果失败则抛出运行时错误 [cite: 3]。
2.  **逐行读取**: 遍历文件的每一行。
3.  **识别元数据**:
      * [cite\_start]以 "DATE:" 开头的行被识别为日期行，并从中提取 "YYYYMM" 格式的日期、年份和月份 [cite: 3]。
      * [cite\_start]以 "REMARK:" 开头的行被识别为备注行 [cite: 3]。
4.  **识别类别**:
      * [cite\_start]**父类别**: 通过 `is_parent_title` 辅助函数判断。该函数检查行的第一个字符是否为大写字母 [cite: 3]。如果判定为父类别行，则提取并更新当前的父类别上下文。
      * [cite\_start]**子类别**: 通过 `is_sub_title` 辅助函数判断。该函数检查行的第一个字符是否为小写字母 [cite: 3]。如果判定为子类别行，则提取并更新当前的子类别上下文。
5.  **解析交易**:
      * 不属于以上任何情况的非空行被视为交易记录。
      * [cite\_start]行首的浮点数被解析为 `amount` (金额) [cite: 3]。
      * [cite\_start]行中余下的部分被解析为 `description` (描述) [cite: 3]。
      * [cite\_start]使用当前上下文中的父类别和子类别，连同解析出的金额和描述，共同构造一个 `Transaction` 对象，并将其添加到 `ParsedBill` 的 `transactions` 列表中 [cite: 3]。
6.  [cite\_start]**返回结果**: 文件读取完毕后，返回包含所有解析数据的 `ParsedBill` 对象 [cite: 3]。

#### 2.3 数据库插入器 - `BillInserter`

`BillInserter` 负责所有数据库操作，确保数据能够正确、原子性地写入 SQLite 数据库。

**核心功能**:

1.  **数据库连接**:

      * [cite\_start]构造函数接收一个数据库文件路径，并尝试打开连接。如果失败，则抛出异常 [cite: 7]。
      * [cite\_start]析构函数负责安全地关闭数据库连接 [cite: 7]。
      * [cite\_start]**关键设置**: 在连接建立后，会立即执行 `PRAGMA foreign_keys = ON;` 来启用外键约束，保证了 `bills` 表和 `transactions` 表之间关联的完整性 [cite: 7]。

2.  **数据库初始化 (`initialize_database`)**:

      * 此私有方法在构造时被调用。
      * [cite\_start]它会执行 `CREATE TABLE IF NOT EXISTS` SQL语句来创建 `bills` 和 `transactions` 两张表 [cite: 7]。
      * [cite\_start]**`bills` 表**: 存储每个账单的元数据，`bill_date` 字段被设置为 `UNIQUE`，以防止同一月份的账单重复记录 [cite: 7]。
      * [cite\_start]**`transactions` 表**: 存储具体的交易条目。`bill_id` 作为外键关联到 `bills` 表的 `id`，并设置了 `ON DELETE CASCADE` 级联删除。这意味着当 `bills` 表中的一条记录被删除时，所有与之关联的交易记录也会被自动删除 [cite: 7]。

3.  **数据插入 (`insert_bill`)**:

      * [cite\_start]这是 `BillInserter` 的核心公共方法，它以一个 `ParsedBill` 对象为输入 [cite: 7]。
      * [cite\_start]**事务管理**: 整个插入过程被一个 SQL 事务包裹 (`BEGIN TRANSACTION;` ... `COMMIT;`)。这确保了操作的原子性：要么所有数据都成功插入，要么在任何一步发生错误时回滚所有更改 (`ROLLBACK;`)，避免了数据不一致的状态 [cite: 7]。
      * **执行流程**:
        1.  [cite\_start]**删除旧数据 (`delete_bill_by_date`)**: 在插入新数据之前，会先根据账单日期删除可能已存在的旧记录。这使得接口具有幂等性，即无论执行多少次，结果都相同 [cite: 7]。
        2.  [cite\_start]**插入父记录 (`insert_bill_record`)**: 将账单的元数据（日期、年份、月份、备注）插入 `bills` 表，并获取新生成的 `bill_id` [cite: 7]。
        3.  [cite\_start]**插入子记录 (`insert_transactions_for_bill`)**: 遍历 `ParsedBill` 中的所有 `Transaction` 对象，使用上一步获取的 `bill_id` 作为外键，将每一笔交易插入到 `transactions` 表中 [cite: 7]。

#### 2.4 数据处理器 - `DataProcessor`

`DataProcessor` 是为外部调用者设计的顶层封装。它隐藏了 `parser` 和 `inserter` 的实现细节。

  * **`process_and_insert` 方法**:
    1.  [cite\_start]接收账单文件路径和数据库路径作为参数 [cite: 1]。
    2.  [cite\_start]在内部实例化 `BillParser` 并调用其 `parse` 方法来解析文件 [cite: 2]。
    3.  [cite\_start]接着，实例化 `BillInserter` 并调用其 `insert_bill` 方法将解析出的数据存入数据库 [cite: 2]。
    4.  [cite\_start]通过 `try...catch` 块捕获整个过程中可能发生的任何异常，并向控制台输出详细的进度和错误信息，最后返回一个布尔值表示操作是否成功 [cite: 2]。

### 3\. 工作流程

1.  **启动**: 外部应用（例如 `main` 函数）实例化 `DataProcessor`。
2.  **调用**: 外部应用调用 `dataProcessor.process_and_insert("path/to/bill.txt", "path/to/database.db")`。
3.  **解析**: `DataProcessor` 创建 `BillParser`，读取 "bill.txt" 文件，将其内容解析成一个 `ParsedBill` 对象。
4.  **插入**: `DataProcessor` 创建 `BillInserter`，连接到 "database.db"。
5.  **执行**: `BillInserter` 启动一个数据库事务，先删除数据库中与该账单月份相同的所有旧数据，然后插入新的账单和交易记录。
6.  **完成**: 事务成功提交，`process_and_insert` 方法返回 `true`。如果任何步骤失败，事务将回滚，方法返回 `false`。

### 4\. 如何使用

要使用此模块，只需包含 `DataProcessor.h` 头文件，并调用其 `process_and_insert` 方法。

```cpp
#include "db_insert/DataProcessor.h"
#include <iostream>

int main() {
    DataProcessor processor;
    const std::string bill_file = "path/to/your/202501.txt";
    const std::string db_file = "bills.db";

    std::cout << "开始处理文件: " << bill_file << std::endl;

    if (processor.process_and_insert(bill_file, db_file)) {
        std::cout << "文件处理和数据插入成功！" << std::endl;
    } else {
        std::cerr << "文件处理失败。" << std::endl;
        return 1;
    }

    return 0;
}
```

### 5\. 外部依赖

  * **SQLite3**: 本模块的数据库操作依赖于 `sqlite3` 库。在编译链接时，必须确保链接到该库（例如，在 GCC/Clang 中使用 `-lsqlite3` 标志）。