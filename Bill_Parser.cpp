#include "Bill_Parser.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

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
    currentParentName_ = ""; // 重置时清空
    currentFilename_ = ""; // 在 reset 时也清空文件名
}

void Bill_Parser::parseFile(const std::string& filename, std::function<void(const ParsedRecord&)> callback) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    reset();
    currentFilename_ = filename; // 在解析文件前，记录下文件名

    std::string line;
    while (std::getline(file, line)) {
        parseLine(line, callback);
    }
}

void Bill_Parser::parseLine(const std::string& line, std::function<void(const ParsedRecord&)> callback) {
    lineNumber_++;
    
    ValidationResult result = validator_.validate(line);

    if (result.type == "empty") {
        return;
    }

    if (result.type == "date") {
        //...
        currentParentName_ = ""; // 新的日期块开始，重置父类别上下文
        //...（其余代码不变）
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
        // ...（代码不变）
        ParsedRecord rec;
        rec.type = "remark";
        rec.lineNumber = lineNumber_;
        rec.content = result.matches[0];
        callback(rec);
    } 
    else if (result.type == "item") {
        // ...（代码不变）
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
        const std::string& child_name = result.matches[0];
        if (!validator_.is_valid_child_for_parent(currentParentName_, child_name)) {
            throw std::runtime_error("Validation Error in file '" + currentFilename_ + 
                                     "' on line " + std::to_string(lineNumber_) +
                                     ": Child category '" + child_name +
                                     "' is not a valid child for parent category '" + currentParentName_ + "'.");
        }

        if (currentParentOrder_ > 0) {
            childCounter_++;
            itemCounter_ = 0;
            currentChildOrder_ = childCounter_;
            ParsedRecord rec;
            rec.type = "child";
            rec.lineNumber = lineNumber_;
            rec.order = childCounter_;
            rec.parentOrder = currentParentOrder_;
            rec.content = child_name;
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
        currentParentName_ = result.matches[0]; // 存储当前父项目名称
        ParsedRecord rec;
        rec.type = "parent";
        rec.lineNumber = lineNumber_;
        rec.order = parentCounter_;
        rec.content = currentParentName_;
        callback(rec);
    } 
    else if (result.type == "unrecognized") {
        std::cerr << "Warning: Unrecognized line format on line " << lineNumber_ << ": '" << result.matches[0] << "'. Skipping." << std::endl;
    }
}