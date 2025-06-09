#include "Bill_Parser.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <algorithm>

// Initialize static regex members
// NOTE: For Chinese characters to be matched correctly, the input file must be in UTF-8.
const std::regex Bill_Parser::dateRegex_(R"(^DATE:(\d{6})$)");
const std::regex Bill_Parser::remarkRegex_(R"(^REMARK:(.*)$)");
// CORRECTED: This regex is more compatible and avoids the "Invalid range" error.
// It works because the parsing logic checks for DATE and REMARK lines first.
const std::regex Bill_Parser::parentRegex_(R"(^([A-Z].*)$)");
const std::regex Bill_Parser::childRegex_(R"(^([a-z_]+)$)");
const std::regex Bill_Parser::itemRegex_(R"(^(\d+(?:\.\d+)?)\s*(.*)$)");

Bill_Parser::Bill_Parser() {
    reset();
}

void Bill_Parser::reset() {
    records_.clear();
    lineNumber_ = 0;
    parentCounter_ = 0;
    childCounter_ = 0;
    itemCounter_ = 0;
    currentParentOrder_ = 0;
    currentChildOrder_ = 0;
}

std::string Bill_Parser::trim(const std::string& s) {
    auto start = s.begin();
    while (start != s.end() && std::isspace(static_cast<unsigned char>(*start))) {
        start++;
    }
    auto end = s.end();
    if (start != end) {
        do {
            end--;
        } while (std::distance(start, end) > 0 && std::isspace(static_cast<unsigned char>(*end)));
    }
    return std::string(start, end == s.end() ? end : end + 1);
}

void Bill_Parser::parseFile(const std::string& filename) {
    // Use std::ifstream to read the file
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    reset(); // Reset state before starting

    std::string line;
    while (std::getline(file, line)) {
        parseLine(line);
    }
}

const std::vector<ParsedRecord>& Bill_Parser::getRecords() const {
    return records_;
}

void Bill_Parser::parseLine(const std::string& line) {
    lineNumber_++;
    std::string trimmedLine = trim(line);

    if (trimmedLine.empty()) {
        return; // 跳过空行
    }

    std::smatch matches;

    // 使用 if-else if 结构将所有匹配逻辑链接起来
    // 一旦有一个分支匹配成功，后续的 else if 和 else 都将被跳过

    // 1. Match DATE
    if (std::regex_match(trimmedLine, matches, dateRegex_)) {
        parentCounter_ = 0;
        childCounter_ = 0;
        itemCounter_ = 0;
        currentParentOrder_ = 0;
        currentChildOrder_ = 0;

        ParsedRecord rec;
        rec.type = "date";
        rec.lineNumber = lineNumber_;
        rec.content = matches[1].str();
        records_.push_back(rec);
    }
    // 2. 改为 else if
    else if (std::regex_match(trimmedLine, matches, remarkRegex_)) {
        ParsedRecord rec;
        rec.type = "remark";
        rec.lineNumber = lineNumber_;
        rec.content = trim(matches[1].str());
        records_.push_back(rec);
    }
    // 3. 改为 else if
    else if (std::regex_match(trimmedLine, matches, itemRegex_)) {
        if (currentParentOrder_ == 0 || currentChildOrder_ == 0) {
            std::cerr << "Warning: Item on line " << lineNumber_ << " is not under a valid child category. Skipping." << std::endl;
        } else {
            itemCounter_++;
            ParsedRecord rec;
            rec.type = "item";
            rec.lineNumber = lineNumber_;
            rec.order = itemCounter_;
            rec.parentOrder = currentParentOrder_;
            rec.childOrder = currentChildOrder_;
            rec.amount = std::stod(matches[1].str());
            rec.description = trim(matches[2].str());
            records_.push_back(rec);
        }
    }
    // 4. 改为 else if
    else if (std::regex_match(trimmedLine, matches, childRegex_)) {
        if (currentParentOrder_ == 0) {
            std::cerr << "Warning: Child category on line " << lineNumber_ << " is not under a valid parent category. Skipping." << std::endl;
        } else {
            childCounter_++;
            itemCounter_ = 0;
            currentChildOrder_ = childCounter_;

            ParsedRecord rec;
            rec.type = "child";
            rec.lineNumber = lineNumber_;
            rec.order = childCounter_;
            rec.parentOrder = currentParentOrder_;
            rec.content = matches[1].str();
            records_.push_back(rec);
        }
    }
    // 5. 改为 else if
    else if (std::regex_match(trimmedLine, matches, parentRegex_)) {
        parentCounter_++;
        childCounter_ = 0;
        itemCounter_ = 0;
        currentParentOrder_ = parentCounter_;
        currentChildOrder_ = 0;

        ParsedRecord rec;
        rec.type = "parent";
        rec.lineNumber = lineNumber_;
        rec.order = parentCounter_;
        rec.content = matches[1].str();
        records_.push_back(rec);
    }
    // 6. 最后的警告放在 else 块中，确保只有在所有条件都不满足时才执行
    else {
        std::cerr << "Warning: Unrecognized line format on line " << lineNumber_ << ": '" << trimmedLine << "'. Skipping." << std::endl;
    }
}