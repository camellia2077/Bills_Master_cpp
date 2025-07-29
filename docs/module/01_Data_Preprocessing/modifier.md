# 01\_账单内容转换与修改模块

## 模块概述

**账单内容转换与修改模块**负责对原始文本格式的账单内容进行一系列的自动化处理和重格式化。它读取原始账单文本，根据外部配置（`Modifier_Config.json`）执行金额求和、自动续费项添加、内容排序和结构清理等操作，最终将处理后的内容重新格式化输出为新的文本账单。

该模块的目标是：

  * 自动化账单数据录入过程中的重复性工作。
  * 统一账单内容的格式，使其更规范、更易读。
  * 在不改变原始账单意图的前提下，增强数据的结构化和可用性。

## 核心组件

账单内容转换与修改模块主要由以下几个 C++ 类和配置文件组成：

  * **`BillModifier` (模块入口)**

      * **文件：** `modifier/BillModifier.h`, `modifier/BillModifier.cpp`
      * **职责：** 作为整个账单修改流程的统一接口。它负责加载配置，并协调 `BillContentTransformer` 进行内容处理，以及 `BillFormatter` 进行最终的格式化输出。

  * **`ConfigLoader` (配置加载器)**

      * **文件：** `modifier/config_loader/ConfigLoader.h`, `modifier/config_loader/ConfigLoader.cpp`
      * **职责：** 一个静态工具类，专门负责从 `nlohmann::json` 对象中解析 `Modifier_Config.json` 的内容，并将其映射到内部的 `Config` 结构体 中。

  * **`BillContentTransformer` (内容处理器)**

      * **文件：** `modifier/processor/BillContentTransformer.h`, `modifier/processor/BillContentTransformer.cpp`
      * **职责：** 本模块的核心逻辑实现者。它接收原始账单文本，执行所有的数据转换和结构化操作，包括金额求和、自动续费项插入、将文本解析为内部的 `ParentItem` 结构，以及排序和清理逻辑。

  * **`BillFormatter` (文本格式化器)**

      * **文件：** `modifier/raw_format/BillFormatter.h`, `modifier/raw_format/BillFormatter.cpp`
      * **职责：** 负责将 `BillContentTransformer` 处理后形成的结构化数据（`std::vector<ParentItem>`）以及元数据行，根据配置中的格式化规则，重新组合成最终的文本字符串。

  * **`BillDataStructures.h` (共享数据结构)**

      * **文件：** `modifier/_shared_structures/BillDataStructures.h`
      * **职责：** 定义了本模块内部共享的数据结构，包括 `Config`（配置）、`ParentItem`、`SubItem` 和 `ContentItem`（账单结构化表示）等。

## 配置文件 (`Modifier_Config.json`)

`Modifier_Config.json` 是本模块的行为控制中心。它通过 JSON 格式定义了各种操作的开关和规则。

以下是 `Modifier_Config.json` 的示例内容及其说明：

```json
{
  "modification_flags": {
    "enable_summing": true,
    "enable_autorenewal": true,
    "enable_cleanup": true,
    "enable_sorting": true,
    "preserve_metadata_lines": true
  },
  "formatting_rules": {
    "lines_after_parent_section": 4,
    "lines_after_parent_title": 1,
    "lines_between_sub_items": 1
  },
  "auto_renewal_rules": {
    "web_service": [
      {
        "amount": 25.0,
        "description": "迅雷加速器"
      }
    ]
  },
  "metadata_prefixes": [
    "DATE:",
    "REMARK:"
  ]
}
```

  * **`modification_flags` (修改标志)**

      * `enable_summing`: 布尔值，控制是否启用金额求和功能。如果为 `true`，会将如 `10.97+20.97+30` 的表达式计算为 `61.94`。
      * `enable_autorenewal`: 布尔值，控制是否启用自动续费项添加功能。
      * `enable_cleanup`: 布尔值，控制是否启用清理功能（如移除空子项）。
      * `enable_sorting`: 布尔值，控制是否启用内容行排序功能。
      * `preserve_metadata_lines`: 布尔值，控制是否在修改后保留 `DATE:` 和 `REMARK:` 等元数据行。

  * **`formatting_rules` (格式化规则)**

      * `lines_after_parent_section`: 在每个父标题部分（Section）之后插入的空行数量。
      * `lines_after_parent_title`: 在父标题行之后插入的空行数量。
      * `lines_between_sub_items`: 在不同子项之间插入的空行数量。

  * **`auto_renewal_rules` (自动续费规则)**

      * 定义了特定类别下需要自动添加的账单项。
      * 示例中，`web_service` 类别下，如果账单中没有 `迅雷加速器` 这一项，则会自动添加一条 `25.0 迅雷加速器(auto-renewal)` 的记录。

  * **`metadata_prefixes` (元数据前缀)**

      * 定义了被识别为元数据行的前缀列表（例如 `DATE:` 和 `REMARK:`）。

## 运行流程

`BillModifier` 的 `modify` 方法 协调整个账单内容的转换过程：

1.  **加载配置：** `BillModifier` 在构造时，会通过 `ConfigLoader::load()` 静态方法加载并解析 `Modifier_Config.json` 文件，将其转换为 `Config` 结构体，供后续操作使用。
2.  **内容处理 (由 `BillContentTransformer` 完成)：**
      * **初始修改：** 原始账单文本被分割成行，然后根据 `modification_flags` 执行初始修改：
          * **金额求和：** 如果 `enable_summing` 为 `true`，则扫描每一行。如果行中包含金额表达式（例如 `10.97+20.97+30德克士`），则计算其总和（例如 `61.94德克士`），并用求和结果替换原表达式。
          * **自动续费项添加：** 如果 `enable_autorenewal` 为 `true`，则检查 `auto_renewal_rules`。如果某个配置的自动续费项（例如 `web_service` 下的 `迅雷加速器`）在当前账单中不存在，则自动将其添加到对应的子类别下。
      * **结构化解析：** 处理后的行被解析成 `ParentItem`、`SubItem` 和 `ContentItem` 的结构化表示。同时，元数据行（如 `DATE:`, `REMARK:`）会根据 `preserve_metadata_lines` 标志被提取并保留。
      * **排序：** 如果 `enable_sorting` 为 `true`，则会对每个子类别内的内容行按照金额大小进行降序排序。
      * **清理：** 如果 `enable_cleanup` 为 `true`，则会移除账单结构中没有任何内容行的子类别和没有任何子类别的父类别。
3.  **文本格式化 (由 `BillFormatter` 完成)：**
      * `BillFormatter` 接收经过 `BillContentTransformer` 处理后的结构化账单数据和元数据行。
      * 根据 `formatting_rules` 中定义的空行数量，将结构化的账单数据重新组装成规范的文本字符串。

## 具体修改功能

本模块实现了以下自动化修改功能，其行为均由 `Modifier_Config.json` 中的标志和规则控制：

  * **金额求和 (`enable_summing`)**

      * **描述：** 自动识别内容行中包含 `+` 符号的金额表达式，并计算其总和。
      * **示例：** 原始行 `10.97+20.97+30德克士` 将被转换为 `61.94德克士`。

  * **自动续费项添加 (`enable_autorenewal`)**

      * **描述：** 根据 `auto_renewal_rules` 配置，在指定类别下（例如 `web_service`）检查是否存在某些固定消费项。如果不存在，则自动将其添加为新的内容行，并标记为 `(auto-renewal)`。
      * **示例：** 如果 `Modifier_Config.json` 配置 `web_service` 下有 `25.0 迅雷加速器`，而原始账单中没有，则会自动插入 `25.0 迅雷加速器(auto-renewal)`。

  * **内容行排序 (`enable_sorting`)**

      * **描述：** 对每个子类别内部的所有内容行，根据金额大小进行降序排序。如果金额相同，则按描述进行字典序升序排序。
      * **示例：** `meal_high` 下的 `10.97+20.97+30德克士` (转换为 `61.94德克士`) 和 `85.43肯德基`，排序后变为：
        ```
        85.43肯德基
        61.94德克士
        ```

  * **结构清理 (`enable_cleanup`)**

      * **描述：** 自动移除那些没有任何内容行的子类别，以及没有任何子类别的父类别，保持账单结构的简洁性。

  * **元数据行保留 (`preserve_metadata_lines`)**

      * **描述：** 控制 `DATE:` 和 `REMARK:` 等文件头部元数据行是否在修改后被保留在最终输出中。

  * **输出格式化 (`formatting_rules`)**

      * **描述：** 根据配置中的规则，控制输出账单中父标题、子标题和内容行之间的空行数量，以确保格式统一和美观。

## 使用示例

```cpp
#include "modifier/BillModifier.h"
#include <iostream>
#include <fstream>
#include "nlohmann/json.hpp" // 用于解析 JSON 配置

// 假设有一个函数可以读取文件内容到字符串
std::string read_file_to_string(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        throw std::runtime_error("无法打开文件: " + file_path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main() {
    try {
        // 1. 读取 Modifier_Config.json 配置
        std::string config_content = read_file_to_string("Modifier_Config.json"); // 替换为 Modifier_Config.json 实际路径
        nlohmann::json config_json = nlohmann::json::parse(config_content);

        // 2. 创建 BillModifier 实例
        BillModifier modifier(config_json);

        // 3. 读取原始账单内容
        std::string original_bill_content = read_file_to_string("原始数据202501.txt"); // 替换为原始账单文件路径

        // 4. 执行修改
        std::string modified_bill_content = modifier.modify(original_bill_content);

        // 5. 打印或保存修改后的账单
        std::cout << "--- 修改后的账单内容 ---\n";
        std::cout << modified_bill_content << std::endl;
        std::cout << "--------------------------\n";

        // 也可以保存到新文件
        // std::ofstream output_file("修改后的账单202501.txt");
        // output_file << modified_bill_content;
        // output_file.close();
        // std::cout << "修改后的账单已保存到 '修改后的账单202501.txt'\n";

    } catch (const std::exception& e) {
        std::cerr << "处理账单时发生错误: " << e.what() << std::endl;
    }
    return 0;
}
```