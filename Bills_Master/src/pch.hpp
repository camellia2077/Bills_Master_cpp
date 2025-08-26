#ifndef PCH_H
#define PCH_H

// ===================================================================
//  1. C++ 标准库 (Standard Library)
//  最稳定、最庞大、使用最频繁的部分，PCH的核心价值所在。
// ===================================================================
#include <string>       // 使用次数: 42
#include <vector>       // 使用次数: 26
#include <iostream>     // 使用次数: 19
#include <sstream>      // 使用次数: 16
#include <stdexcept>    // 使用次数: 15
#include <iomanip>      // 使用次数: 13
#include <map>          // 使用次数: 9
#include <filesystem>   // 使用次数: 6
#include <fstream>      // 使用次数: 6
#include <algorithm>    // 使用次数: 5
#include <set>          // 使用次数: 4
#include <memory>       // 使用次数: 3
#include <print>        // 使用次数: 3 (C++23)

// ===================================================================
//  2. 平台与第三方库 (Platform & Third-Party)
//  这些库的内容不会由您修改，是PCH的完美候选。
// ===================================================================
#ifdef _WIN32
#include <windows.h>    // 使用次数: 3
#endif
#include <sqlite3.h>    // 使用次数: 8
#include "nlohmann/json.hpp" // 使用次数: 5

// ===================================================================
//  3. 项目内全局通用且稳定的头文件
//  这些是项目的基础设施，一旦成型，很少改动。
// ===================================================================
#include "common/common_utils.hpp" // 使用次数: 10
#include "app_controller/ProcessStats.hpp" // 使用次数: 3

// --- 插件系统的核心接口 (非常稳定) ---
#include "query/plugins/month_formatters/IMonthReportFormatter.hpp" // 使用次数: 7
#include "query/plugins/year_formatters/IYearlyReportFormatter.hpp"  // 使用次数: 7
#include "query/plugins/year_formatters/BaseYearlyReportFormatter.hpp" // 使用次数: 4

// --- 共享的数据结构 (相对稳定) ---
#include "reprocessing/modifier/_shared_structures/BillDataStructures.hpp" // 使用次数: 4
#include "query/components/monthly_report/ReportData.hpp" // 使用次数: 2
#include "query/components/yearly_report/YearlyReportData.hpp" // 使用次数: 2

#endif //PCH_H