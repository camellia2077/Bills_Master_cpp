// modifier/BillModifier.cpp

#include "BillModifier.hpp"
#include "reprocessing/modifier/config_loader/ConfigLoader.hpp"
#include "reprocessing/modifier/processor/BillContentTransformer.hpp"
#include "reprocessing/modifier/raw_format/BillJsonFormatter.hpp" 
#include <vector>
#include <string>

BillModifier::BillModifier(const nlohmann::json& config_json) {
    m_config = ConfigLoader::load(config_json);
}

std::string BillModifier::modify(const std::string& bill_content) {
    // **修改**: 现在 languages 是一个包含多种语言的向量
    const std::vector<std::string>& languages = m_config.language_setting;

    BillContentTransformer processor(m_config);
    std::vector<std::string> metadata_lines;
    std::vector<ParentItem> bill_structure = processor.process(bill_content, metadata_lines);

    BillJsonFormatter formatter;
    // 将整个语言向量传递给 formatter
    return formatter.format(bill_structure, metadata_lines, m_config.display_name_maps, languages);
}