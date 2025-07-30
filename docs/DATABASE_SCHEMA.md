# 数据库结构 (Database Schema)

本文档详细描述了 `bills.sqlite3` 数据库的表结构、字段和表之间的关系。

## 实体关系图 (ERD)
暂无


## 表结构详情

### `bills` Table

该表存储每个账单的摘要信息，如日期和备注。每个年/月组合应该只有一条记录。

| Column Name | Data Type | Constraints             | Description                               |
|-------------|-----------|-------------------------|-------------------------------------------|
| `id`        | `INTEGER` | `PRIMARY KEY`, `AUTOINCREMENT` | 表的唯一标识符。                              |
| `bill_date` | `TEXT`    | `NOT NULL`, `UNIQUE`    | 账单的日期字符串 (例如, "2024-01")。        |
| `year`      | `INTEGER` | `NOT NULL`              | 账单所属的年份。                              |
| `month`     | `INTEGER` | `NOT NULL`              | 账单所属的月份。                              |
| `remark`    | `TEXT`    |                         | 用户为该月账单添加的备注。                      |

### `transactions` Table

该表存储每一笔具体的交易记录。
| Column Name       | Data Type | Constraints       | Description                                    |
|-------------------|-----------|-------------------|------------------------------------------------|
| `id`              | `INTEGER` | `PRIMARY KEY`, `AUTOINCREMENT` | 交易记录的唯一标识符。                         |
| `bill_id`         | `INTEGER` | `NOT NULL`, `FOREIGN KEY` | 关联到 `bills` 表的 `id`，指明该交易属于哪个账单。 |
| `parent_category` | `TEXT`    | `NOT NULL`        | 交易的父级分类 (例如, "餐饮")。                  |
| `sub_category`    | `TEXT`    | `NOT NULL`        | 交易的子级分类 (例如, "午餐")。                  |
| `amount`          | `REAL`    | `NOT NULL`        | 交易金额。                                     |
| `description`     | `TEXT`    |                   | 交易的详细描述。    