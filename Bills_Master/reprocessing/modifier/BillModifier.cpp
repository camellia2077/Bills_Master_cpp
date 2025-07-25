#include "BillModifier.h"

#include "reprocessing/modifier/config_loader/ConfigLoader.h"
#include "reprocessing/modifier/processor/BillContentTransformer.h"
#include "reprocessing/modifier/raw_format/BillFormatter.h"

#include <vector>

// ===================================================================
// BillModifier 公共接口实现
// 职责：作为总协调者，调用独立的类来完成任务。
// ===================================================================
BillModifier::BillModifier(const nlohmann::json& config_json) {
    // 构造函数使用独立的 ConfigLoader 来加载配置
    m_config = ConfigLoader::load(config_json);
}

std::string BillModifier::modify(const std::string& bill_content) {
    // modify 方法负责协调 Processor 和 Formatter
    
    // 1. 创建处理器，处理账单内容，得到结构化数据
    BillContentTransformer processor(m_config);
    std::vector<std::string> metadata_lines;
    std::vector<ParentItem> bill_structure = processor.process(bill_content, metadata_lines);

    // 2. 创建格式化器，将结构化数据转换为最终字符串
    BillFormatter formatter(m_config);
    return formatter.format(bill_structure, metadata_lines);
}