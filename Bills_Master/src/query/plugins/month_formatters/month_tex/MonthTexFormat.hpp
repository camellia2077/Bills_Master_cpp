// query/plugins/month_formatters/month_tex/MonthTexFormat.hpp

#ifndef MONTH_TEX_FORMAT_HPP
#define MONTH_TEX_FORMAT_HPP

#include "query/plugins/month_formatters/IMonthReportFormatter.hpp"
#include "MonthTexReport.hpp" // 包含新的配置头文件

class MonthTexFormat : public IMonthReportFormatter {
public:
    /**
     * @brief 构造函数，接收一个配置对象。
     * @param config 包含了所有LaTeX格式化选项的配置结构体。
     *               提供了一个默认构造的实例，使得不带参数的创建成为可能。
     */
    explicit MonthTexFormat(const MonthTexReport& config = MonthTexReport{}); // <-- 2. 修改构造函数

    std::string format_report(const MonthlyReportData& data) const override;

private:
    std::string escape_latex(const std::string& input) const; 

    // 3. 添加一个私有成员变量来存储配置
    MonthTexReport m_config; 
};

#endif // MONTH_TEX_FORMAT_HPP