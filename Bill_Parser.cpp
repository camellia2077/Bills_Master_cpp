#include "Bill_Parser.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

// 构造函数：通过成员初始化列表设置对 validator 的引用
Bill_Parser::Bill_Parser(const LineValidator& validator)
    : validator_(validator) {
    reset();
}

void Bill_Parser::reset() {
    lineNumber_ = 0;
    parentCounter_ = 0;
    childCounter_ = 0;
    itemCounter_ = 0;
    currentParentOrder_ = 0;
    currentChildOrder_ = 0;
}

void Bill_Parser::parseFile(const std::string& filename, std::function<void(const ParsedRecord&)> callback) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    reset();

    std::string line;
    while (std::getline(file, line)) {
        parseLine(line, callback);
    }
}

// parseLine 方法完全不变，它只是通过 validator_ 引用来调用 validate 函数
void Bill_Parser::parseLine(const std::string& line, std::function<void(const ParsedRecord&)> callback) {
    lineNumber_++;
    
    ValidationResult result = validator_.validate(line);

    if (result.type == "empty") {
        return;
    }

    if (result.type == "date") {
        parentCounter_ = 0;
        childCounter_ = 0;
        itemCounter_ = 0;
        currentParentOrder_ = 0;
        currentChildOrder_ = 0;
        
        ParsedRecord rec;
        rec.type = "date";
        rec.lineNumber = lineNumber_;
        rec.content = result.matches[0];
        callback(rec);
    } 
    else if (result.type == "remark") {
        ParsedRecord rec;
        rec.type = "remark";
        rec.lineNumber = lineNumber_;
        rec.content = result.matches[0];
        callback(rec);
    } 
    else if (result.type == "item") {
        if (currentParentOrder_ > 0 && currentChildOrder_ > 0) {
            itemCounter_++;
            ParsedRecord rec;
            rec.type = "item";
            rec.lineNumber = lineNumber_;
            rec.order = itemCounter_;
            rec.parentOrder = currentParentOrder_;
            rec.childOrder = currentChildOrder_;
            rec.amount = std::stod(result.matches[0]);
            rec.description = result.matches[1];
            callback(rec);
        } else {
            std::cerr << "Warning: Item on line " << lineNumber_ << " is not under a valid child category. Skipping." << std::endl;
        }
    } 
    else if (result.type == "child") {
        if (currentParentOrder_ > 0) {
            childCounter_++;
            itemCounter_ = 0;
            currentChildOrder_ = childCounter_;
            ParsedRecord rec;
            rec.type = "child";
            rec.lineNumber = lineNumber_;
            rec.order = childCounter_;
            rec.parentOrder = currentParentOrder_;
            rec.content = result.matches[0];
            callback(rec);
        } else {
            std::cerr << "Warning: Child category on line " << lineNumber_ << " is not under a valid parent category. Skipping." << std::endl;
        }
    } 
    else if (result.type == "parent") {
        parentCounter_++;
        childCounter_ = 0;
        itemCounter_ = 0;
        currentParentOrder_ = parentCounter_;
        currentChildOrder_ = 0;
        ParsedRecord rec;
        rec.type = "parent";
        rec.lineNumber = lineNumber_;
        rec.order = parentCounter_;
        rec.content = result.matches[0];
        callback(rec);
    } 
    else if (result.type == "unrecognized") {
        std::cerr << "Warning: Unrecognized line format on line " << lineNumber_ << ": '" << result.matches[0] << "'. Skipping." << std::endl;
    }
}