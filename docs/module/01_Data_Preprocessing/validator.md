# 01\_验证模块

## 模块概述

**验证模块**是本账单管理系统接收原始账单数据后的第一道关卡。其核心职责是**读取、解析并验证原始的文本账单文件**，确保其格式符合预设规范，并对数据进行初步的清洗和计算（如金额求和），为后续的数据库插入和报告生成奠定基础。

该模块旨在：

  * 验证账单文件的结构和内容合法性。
  * 将文本格式的金额（包括需要求和的表达式）转换为标准数值。
  * 识别和验证账单中的分类信息（父标题和子标题）。
  * 收集并报告验证过程中发现的任何错误和警告。

## 核心组件

数据预处理模块主要由以下几个 C++ 类组成：

  * **`BillValidator` (验证器入口)**

      * **文件：** `validator/BillValidator.h`, `validator/BillValidator.cpp`
      * **职责：** 作为整个预处理验证流程的高层封装接口。它负责加载验证配置，协调 `BillFormatVerifier` 执行验证，并最终通过 `ValidationResult` 打印验证报告。这是外部调用预处理功能的入口点。

  * **`BillConfig` (配置加载器)**

      * **文件：** `validator/config/BillConfig.h`, `validator/config/BillConfig.cpp`
      * **职责：** 从 JSON 配置文件 (`Validator_Config.json`) 中加载并解析所有预定义的账单分类规则（包括有效的父标题及其对应的子标题）。它提供方法供其他组件查询某个标题是否合法。
      * **配置文件：** `Validator_Config.json` 包含了所有允许的账单类别，例如 `MEAL吃饭` 下包含 `meal_low`、`meal_high` 等子类别。

  * **`BillFormatVerifier` (格式验证器)**

      * **文件：** `validator/verifier/BillFormatVerifier.h`, `validator/verifier/BillFormatVerifier.cpp`
      * **职责：** 执行核心的账单文件格式验证逻辑。它采用状态机模式逐行读取账单文件，根据预设规则（由 `BillConfig` 提供）进行严格的格式检查。这是实现 `格式化验证.txt`  中大部分规则的地方。

  * **`ValidationResult` (验证结果收集器)**

      * **文件：** `validator/result/ValidationResult.h`, `validator/result/ValidationResult.cpp`
      * **职责：** 用于收集、存储和管理验证过程中发现的所有错误和警告信息。它也提供了打印详细验证报告的功能。

## 输入示例

本模块处理的原始账单文件遵循特定的文本格式。以下是一个典型的输入账单文件 `原始数据202501.txt`  的示例，它将在预处理模块中被读取和验证：

```
DATE:202501
REMARK:
MEAL吃饭
meal_low
415.53饭
meal_high
10.97+20.97+30德克士
85.43肯德基
meal_drink
95.20瑞幸
46.30蜜雪冰城
meal_snacks
16.99甜点
89.70冰激凌
meal_fruits
47.77菠萝
10.97西瓜

PURCHASE购物
purchase_toys
570.38乐高

DAILY日常
daily_fees
27.35哈喽
daily_consumables
6.53牙膏

WEB网络
web_service
29.80迅雷
web_game
30.00游戏月卡
```

## 运行流程

数据预处理模块的运行流程如下：

1.  **初始化 `BillValidator`：** 当系统需要验证一个账单文件时，会创建 `BillValidator` 实例，并传入 `Validator_Config.json` 的路径。`BillValidator` 会内部加载此配置文件到 `BillConfig` 对象中。
2.  **调用 `validate` 方法：** 调用 `BillValidator::validate(bill_file_path)`，传入待验证的原始账单文件路径（例如 `原始数据202501.txt` ）。
3.  **文件读取与头部验证：**
      * `BillFormatVerifier` 打开账单文件，并首先验证文件的前两行：
          * 第一行必须是 `DATE:YYYYMM` 格式，其中 `YYYYMM` 必须是六位数字 。
          * 第二行必须以 `REMARK:` 开头 。
      * 如果头部验证失败，将记录错误，并通常终止对该文件的后续处理。
4.  **逐行解析与状态机验证：**
      * `BillFormatVerifier` 使用一个内部**状态机**（`EXPECT_PARENT`, `EXPECT_SUB`, `EXPECT_CONTENT`）逐行读取文件内容。
      * **状态切换：** 根据当前行的内容和预期的格式，状态机在“期望父标题”、“期望子标题”、“期望内容行”之间切换。
      * **父标题验证：**
          * 识别全大写中文作为父标题的行 (例如 `MEAL吃饭`) 。
          * 使用 `BillConfig` 检查该父标题是否在 `Validator_Config.json` 中定义为合法类别 。
          * 如果当前状态期望父标题但遇到其他内容，或父标题不合法，则记录错误。
      * **子标题验证：**
          * 识别小写英文和下划线组成的英文作为子标题的行 (例如 `meal_low`) 。
          * 使用 `BillConfig` 检查该子标题是否在其所属的父标题下合法 。
          * 如果当前状态期望子标题但遇到其他内容，或子标题不合法，则记录错误。
      * **内容行处理：**
          * **格式验证：** 验证内容行是否符合“金额 + 描述”的格式，金额在前，描述在后，且描述中**不包含数字** 。
          * **金额求和与计算：** 如果金额部分包含 `+` 符号（例如 `10.97+20.97+30德克士` ），预处理逻辑会解析此表达式并计算其总和（例如 `61.94德克士`）。这一步骤是数据清洗和标准化，确保所有金额都是单一的数值。
          * **内容计数：** 记录每个子标题下有多少内容行，用于后续检查子标题是否为空。
5.  **后期验证检查：**
      * 文件读取完成后，`BillFormatVerifier` 会执行额外的检查，例如确认所有父标题和子标题下都有实际的内容行，避免出现空分类。
6.  **报告生成：** 所有的验证结果（错误和警告）都被 `ValidationResult` 对象收集。验证流程结束后，`BillValidator` 会调用 `ValidationResult::print_report()` 方法，将详细的验证结果（包括所有错误和警告）输出到控制台。
7.  **返回验证状态：** `BillValidator::validate` 方法最终返回一个布尔值，指示文件是否通过了所有“错误”级别的验证（即 `ValidationResult` 中没有错误）。

## 合法性检验细节

该模块执行的合法性检验严格遵循 `格式化验证.txt`  中的定义：

  * **文件头部信息验证：**
      * **日期行：** 严格检查第一行是否为 `DATE:YYYYMM` 格式，`YYYYMM` 必须是 6 位数字 。
      * **备注行：** 第二行必须以 `REMARK:` 开头 。
  * **账单结构层级验证：**
      * 强制遵循 `父标题 -> 子标题 -> 内容行` 的层级结构 。任何不符合此顺序的行都会被标记为错误。
  * **父标题检查：**
      * **定义：** 父标题必须是**全大写中文** (例如 `MEAL吃饭`) 。
      * **有效性检查：** 程序会查阅 `Validator_Config.json` 中预定义的分类列表，确保检测到的父标题是其中之一 。
  * **子标题检查：**
      * **定义：** 子标题必须是**小写英文和下划线** (`_`) 组成的格式 (例如 `meal_low`) 。
      * **有效性检查：** 对于每个检测到的子标题，程序会验证它是否属于其**当前父标题**在 `Validator_Config.json` 中定义的合法子类别 。
  * **内容行检查：**
      * **格式定义：** 内容行必须是“金额 + 描述”的格式，金额在前，描述在后，描述中**不包含数字** 。
      * **金额求和：** 如果金额部分包含 `+` 符号（例如 `10.97+20.97+30德克士` ），预处理逻辑会解析此表达式并计算其总和（例如 `61.94德克士`）。这是一个关键的数据清洗和标准化步骤，确保所有金额都是单一的数值。
      * **位置检查：** 内容行必须紧跟在一个合法的子标题行之后 。
  * **空分类检查：** 验证父标题和子标题下方是否至少有一条内容行。如果子标题下没有内容行，会被记录为警告；如果父标题下没有子标题或子标题下都没有内容行，也可能记录错误或警告。

## 配置 (`Validator_Config.json`)

`Validator_Config.json` 是预处理模块的核心配置。它定义了系统接受的所有合法账单类别和其层级关系。`BillConfig` 类负责加载和解析这个 JSON 文件，将其内容转换为内部的数据结构（`std::unordered_map` 和 `std::set`），以便高效地进行标题和子标题的合法性检查。

## 错误处理

`ValidationResult` 类负责收集验证过程中生成的所有错误和警告。

  * **错误 (Errors)：** 表示数据格式或结构存在严重问题，通常意味着文件不符合规范，无法可靠地进行后续处理。如果检测到任何错误，`BillValidator::validate` 将返回 `false`。
  * **警告 (Warnings)：** 表示数据中存在一些小问题或潜在的异常情况，这些问题可能不会阻止后续处理，但值得注意。
  * 所有错误和警告都会在验证过程结束时通过 `ValidationResult::print_report()` 方法打印到控制台，方便用户查看。

## 使用示例

要使用数据预处理模块验证一个账单文件，通常的流程如下：

```cpp
#include "validator/BillValidator.h"
#include <iostream>

int main() {
    try {
        // 假设您的 Validator_Config.json 在项目根目录或可访问路径
        BillValidator validator("Validator_Config.json"); 

        std::string bill_file_path = "原始数据202501.txt"; // 替换为实际的账单文件路径
        bool is_valid = validator.validate(bill_file_path);

        if (is_valid) {
            std::cout << "账单文件验证通过！\n";
        } else {
            std::cout << "账单文件验证未通过，请查看报告。\n";
        }
    } catch (const std::runtime_error& e) {
        std::cerr << "初始化验证器失败: " << e.what() << std::endl;
    }
    return 0;
}
```