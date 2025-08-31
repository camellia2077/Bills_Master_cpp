// modifier/BillModifier.cpp

#include "BillModifier.hpp"

#include "reprocessing/modifier/config_loader/ConfigLoader.hpp"
#include "reprocessing/modifier/processor/BillContentTransformer.hpp"

// 包含新的 JSON 格式化器
#include "reprocessing/modifier/raw_format/BillJsonFormatter.hpp" 

#include <vector>

// ===================================================================
// BillModifier 公共接口实现
// ===================================================================

// 构造函数保持不变
BillModifier::BillModifier(const nlohmann::json& config_json) {
    m_config = ConfigLoader::load(config_json);
}

// 修改 modify 方法以使用新的 Formatter
std::string BillModifier::modify(const std::string& bill_content) {
    // 1. 创建处理器，处理账单内容，得到结构化数据 (此部分不变)
    BillContentTransformer processor(m_config);
    std::vector<std::string> metadata_lines;
    std::vector<ParentItem> bill_structure = processor.process(bill_content, metadata_lines);

    // 2. 创建新的 JSON 格式化器，将结构化数据转换为最终的 JSON 字符串
    BillJsonFormatter formatter; // <--- 使用新的 BillJsonFormatter
    return formatter.format(bill_structure, metadata_lines);
}