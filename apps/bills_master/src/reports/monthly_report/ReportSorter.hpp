// reports/monthly_report/ReportSorter.hpp
#ifndef REPORT_SORTER_HPP
#define REPORT_SORTER_HPP

#include "ReportData.hpp"
#include <vector>
#include <string>

/**
 * @class ReportSorter
 * @brief 一个工具类，提供对报表数据进行排序的静态方法。
 *
 * 这个类的目的是将排序逻辑从各个格式化器中分离出来，
 * 以便复用并遵循单一职责原则。
 */
class ReportSorter {
public:
    /**
     * @brief 对月度报告数据按特定规则进行排序。
     *
     * 排序规则如下：
     * 1. 对每个子类别内部的交易记录（transactions）按金额从高到低排序。
     * 2. 对所有父类别（parent categories）按其总金额（parent_total）从高到低排序。
     *
     * @param data 从数据库查询出的原始聚合数据。
     * @return 返回一个 std::vector，其中包含了按总金额排好序的父类别键值对。
     *         这个结构非常便于格式化器进行遍历和渲染。
     */
    static std::vector<std::pair<std::string, ParentCategoryData>> sort_report_data(const MonthlyReportData& data);
};

#endif // REPORT_SORTER_HPP