// pch.hpp
#pragma once
#ifndef PCH_HPP
#define PCH_HPP

// ===================================================================
//  1. C++ 标准库 (Standard Library)
//  最稳定、最庞大、使用最频繁的部分，PCH的核心价值所在。
// ===================================================================
#include <string>                             // 使用次数: 50
#include <vector>                             // 使用次数: 28
#include <sstream>                            // 使用次数: 17
#include <iostream>                           // 使用次数: 17
#include <stdexcept>                          // 使用次数: 16
#include <iomanip>                            // 使用次数: 14
#include <map>                                // 使用次数: 10
#include <fstream>                            // 使用次数: 7
#include <algorithm>                          // 使用次数: 6
#include <filesystem>                         // 使用次数: 6
#include <set>                                // 使用次数: 5
#include <print>                              // 使用次数: 4 (C++23)
#include <memory>                             // 使用次数: 4
#include <regex>                              // 使用次数: 3
#include <cstdio>                             // 使用次数: 2
#include <cstdint>                            // 使用次数: 2
#include <unordered_map>                      // 使用次数: 2
#include <limits>                             // 使用次数: 1

// ===================================================================
//  2. 平台与第三方库 (Platform & Third-Party)
//  这些库的内容不会由您修改，是PCH的完美候选。
// ===================================================================
#include <sqlite3.h>                          // 使用次数: 8
#include "nlohmann/json.hpp"                  // 使用次数: 8
#ifdef _WIN32
    #include <windows.h>                      // 使用次数: 4
#endif

// ===================================================================
//  3. 项目内全局通用且稳定的头文件
//  这些是项目的基础设施，一旦成型，很少改动。
//  (建议根据项目结构手动调整此部分顺序和分组)
// ===================================================================
#include "common/common_utils.hpp"            // 使用次数: 12
#include "reports/plugins/month_formatters/IMonthReportFormatter.hpp" // 使用次数: 7
#include "reports/plugins/year_formatters/IYearlyReportFormatter.hpp" // 使用次数: 3
#include "reports/plugins/common/PluginLoader.hpp" // 使用次数: 3





#endif //PCH_H