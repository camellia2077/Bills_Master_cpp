// reprocessing/modifier/processor/BillProcessor.hpp

#ifndef BILL_PROCESSOR_H
#define BILL_PROCESSOR_H

#include "reprocessing/modifier/_shared_structures/BillDataStructures.hpp"
#include <vector>
#include <string>

/**
 * @class BillProcessor
 * @brief 负责对原始账单文本行进行预处理。
 *
 * 包括计算行内表达式、应用自动续费规则等。
 */
class BillProcessor {
public:
    explicit BillProcessor(const Config& config);

    /**
     * @brief 执行所有预处理修改。
     * @param lines 原始的文本行向量，将被就地修改。
     */
    void process(std::vector<std::string>& lines);

private:
    const Config& m_config;

    void _apply_auto_renewal(std::vector<std::string>& lines);
    void _sum_up_line(std::string& line);
    // is_title 是一个辅助函数，在 BillContentTransformer 中也有，这里为了独立也保留一份
    static bool _is_title(const std::string& line);
};

#endif // BILL_PROCESSOR_H