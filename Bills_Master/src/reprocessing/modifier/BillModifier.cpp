// modifier/BillModifier.cpp

#include "BillModifier.hpp"
#include "reprocessing/modifier/config_loader/ConfigLoader.hpp"
#include "reprocessing/modifier/processor/BillContentTransformer.hpp"
#include "reprocessing/modifier/raw_format/BillJsonFormatter.hpp" 
#include <vector>

BillModifier::BillModifier(const nlohmann::json& config_json) {
    m_config = ConfigLoader::load(config_json);
}

std::string BillModifier::modify(const std::string& bill_content) {
    BillContentTransformer processor(m_config);
    std::vector<std::string> metadata_lines;
    std::vector<ParentItem> bill_structure = processor.process(bill_content, metadata_lines);

    BillJsonFormatter formatter;
    // 修改: 将显示名称的映射表传递给 formatter
    return formatter.format(bill_structure, metadata_lines, m_config.parent_item_display_names);
}