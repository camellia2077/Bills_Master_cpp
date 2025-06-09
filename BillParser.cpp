#include "BillParser.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <algorithm>

// Initialize static regex members
// NOTE: For Chinese characters to be matched correctly, the input file must be in UTF-8.
const std::regex BillParser::dateRegex_(R"(^DATE:(\d{6})$)");
const std::regex BillParser::remarkRegex_(R"(^REMARK:(.*)$)");
// CORRECTED: This regex is more compatible and avoids the "Invalid range" error.
// It works because the parsing logic checks for DATE and REMARK lines first.
const std::regex BillParser::parentRegex_(R"(^([A-Z].*)$)");
const std::regex BillParser::childRegex_(R"(^([a-z_]+)$)");
const std::regex BillParser::itemRegex_(R"(^(\d+(?:\.\d+)?)\s*(.*)$)");

BillParser::BillParser() {
    reset();
}

void BillParser::reset() {
    records_.clear();
    lineNumber_ = 0;
    parentCounter_ = 0;
    childCounter_ = 0;
    itemCounter_ = 0;
    currentParentOrder_ = 0;
    currentChildOrder_ = 0;
}

std::string BillParser::trim(const std::string& s) {
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

void BillParser::parseFile(const std::string& filename) {
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

const std::vector<ParsedRecord>& BillParser::getRecords() const {
    return records_;
}

void BillParser::parseLine(const std::string& line) {
    lineNumber_++;
    std::string trimmedLine = trim(line);

    if (trimmedLine.empty()) {
        return; // Skip empty lines
    }

    std::smatch matches;

    // 1. Match DATE
    if (std::regex_match(trimmedLine, matches, dateRegex_)) {
        // A new DATE resets all counters
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
        return;
    }

    // 2. Match REMARK
    if (std::regex_match(trimmedLine, matches, remarkRegex_)) {
        ParsedRecord rec;
        rec.type = "remark";
        rec.lineNumber = lineNumber_;
        rec.content = trim(matches[1].str());
        records_.push_back(rec);
        return;
    }

    // 3. Match ITEM (checked before child/parent)
    if (std::regex_match(trimmedLine, matches, itemRegex_)) {
        if (currentParentOrder_ == 0 || currentChildOrder_ == 0) {
            std::cerr << "Warning: Item on line " << lineNumber_ << " is not under a valid child category. Skipping." << std::endl;
            return;
        }
        itemCounter_++;

        ParsedRecord rec;
        rec.type = "item";
        rec.lineNumber = lineNumber_;
        rec.order = itemCounter_;
        rec.parentOrder = currentParentOrder_;
        rec.childOrder = currentChildOrder_;
        rec.amount = std::stod(matches[1].str());
        // The description is the second capture group from itemRegex_
        rec.description = trim(matches[2].str());
        records_.push_back(rec);
        return;
    }

    // 4. Match CHILD
    if (std::regex_match(trimmedLine, matches, childRegex_)) {
        if (currentParentOrder_ == 0) {
            std::cerr << "Warning: Child category on line " << lineNumber_ << " is not under a valid parent category. Skipping." << std::endl;
            return;
        }
        childCounter_++;
        itemCounter_ = 0; // Reset item counter for the new child
        currentChildOrder_ = childCounter_;

        ParsedRecord rec;
        rec.type = "child";
        rec.lineNumber = lineNumber_;
        rec.order = childCounter_;
        rec.parentOrder = currentParentOrder_;
        rec.content = matches[1].str();
        records_.push_back(rec);
        return;
    }

    // 5. Match PARENT (checked after other more specific patterns)
    if (std::regex_match(trimmedLine, matches, parentRegex_)) {
        parentCounter_++;
        childCounter_ = 0; // Reset child counter for the new parent
        itemCounter_ = 0;
        currentParentOrder_ = parentCounter_;
        currentChildOrder_ = 0; // Invalidate child context

        ParsedRecord rec;
        rec.type = "parent";
        rec.lineNumber = lineNumber_;
        rec.order = parentCounter_;
        rec.content = matches[1].str();
        records_.push_back(rec);
        return;
    }
    
    // 6. If no patterns matched
    std::cerr << "Warning: Unrecognized line format on line " << lineNumber_ << ": '" << trimmedLine << "'. Skipping." << std::endl;
}