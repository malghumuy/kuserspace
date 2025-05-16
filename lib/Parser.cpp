// Malghumuy - Library: kuserspace
#include "../include/Parser.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <chrono>

namespace kuserspace {

Parser& Parser::getInstance() {
    static Parser instance;
    return instance;
}

Parser::ParseResult Parser::parseLine(const std::string& line, const std::string& pattern) {
    try {
        std::smatch matches;
        std::regex regex = getRegex(pattern);
        
        if (std::regex_search(line, matches, regex)) {
            if (matches.size() > 1) {
                return ParseResult(true, matches[1].str());
            }
            return ParseResult(true, matches[0].str());
        }
        
        return ParseResult(false, "", "No match found");
    } catch (const std::regex_error& e) {
        return ParseResult(false, "", std::string("Regex error: ") + e.what());
    } catch (const std::exception& e) {
        return ParseResult(false, "", std::string("Error: ") + e.what());
    }
}

std::vector<Parser::ParseResult> Parser::parseFile(
    const std::filesystem::path& filepath,
    const std::vector<std::string>& patterns) {
    
    std::vector<ParseResult> results;
    results.reserve(patterns.size()); // Pre-allocate space for better performance
    
    std::error_code ec;
    if (!std::filesystem::exists(filepath, ec)) {
        results.emplace_back(false, "", "File does not exist: " + filepath.string());
        return results;
    }
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        results.emplace_back(false, "", "Could not open file: " + filepath.string());
        return results;
    }
    
    // Pre-compile all regex patterns
    std::vector<std::regex> compiledPatterns;
    compiledPatterns.reserve(patterns.size());
    for (const auto& pattern : patterns) {
        compiledPatterns.push_back(getRegex(pattern));
    }
    
    std::string line;
    line.reserve(1024); // Reserve space for typical line length
    
    while (std::getline(file, line)) {
        for (size_t i = 0; i < compiledPatterns.size(); ++i) {
            std::smatch matches;
            if (std::regex_search(line, matches, compiledPatterns[i])) {
                if (matches.size() > 1) {
                    results.emplace_back(true, matches[1].str());
                } else {
                    results.emplace_back(true, matches[0].str());
                }
            }
        }
    }
    
    return results;
}

std::vector<std::string> Parser::extractValues(
    const std::filesystem::path& filepath,
    const std::string& pattern) {
    
    std::vector<std::string> values;
    values.reserve(100); // Pre-allocate space for typical number of values
    
    std::error_code ec;
    if (!std::filesystem::exists(filepath, ec)) {
        throw std::runtime_error("File does not exist: " + filepath.string());
    }
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filepath.string());
    }
    
    std::regex regex = getRegex(pattern);
    std::string line;
    line.reserve(1024);
    
    while (std::getline(file, line)) {
        std::smatch matches;
        if (std::regex_search(line, matches, regex)) {
            if (matches.size() > 1) {
                values.push_back(matches[1].str());
            } else {
                values.push_back(matches[0].str());
            }
        }
    }
    
    return values;
}

std::unordered_map<std::string, std::string> Parser::parseToMap(
    const std::filesystem::path& filepath,
    const std::string& keyPattern,
    const std::string& valuePattern) {
    
    std::unordered_map<std::string, std::string> result;
    result.reserve(100); // Pre-allocate space for typical number of entries
    
    std::error_code ec;
    if (!std::filesystem::exists(filepath, ec)) {
        throw std::runtime_error("File does not exist: " + filepath.string());
    }
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filepath.string());
    }
    
    std::regex keyRegex = getRegex(keyPattern);
    std::regex valueRegex = getRegex(valuePattern);
    std::string line;
    line.reserve(1024);
    
    while (std::getline(file, line)) {
        std::smatch keyMatches, valueMatches;
        if (std::regex_search(line, keyMatches, keyRegex) && 
            std::regex_search(line, valueMatches, valueRegex)) {
            std::string key = keyMatches.size() > 1 ? keyMatches[1].str() : keyMatches[0].str();
            std::string value = valueMatches.size() > 1 ? valueMatches[1].str() : valueMatches[0].str();
            result.emplace(std::move(key), std::move(value));
        }
    }
    
    return result;
}

void Parser::parseWithHandler(
    const std::filesystem::path& filepath,
    const std::function<void(const std::string&, const std::smatch&)>& handler,
    const std::string& pattern) {
    
    std::error_code ec;
    if (!std::filesystem::exists(filepath, ec)) {
        throw std::runtime_error("File does not exist: " + filepath.string());
    }
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filepath.string());
    }
    
    std::regex regex = getRegex(pattern);
    std::string line;
    line.reserve(1024);
    std::smatch matches;
    
    while (std::getline(file, line)) {
        if (std::regex_search(line, matches, regex)) {
            handler(line, matches);
        }
    }
}

void Parser::clearCache() {
    std::lock_guard<std::mutex> lock(cacheMutex);
    regexCache.clear();
}

std::regex Parser::getRegex(const std::string& pattern) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    auto it = regexCache.find(pattern);
    if (it != regexCache.end()) {
        return it->second;
    }
    
    try {
        auto result = regexCache.emplace(pattern, std::regex(pattern));
        return result.first->second;
    } catch (const std::regex_error& e) {
        throw std::runtime_error(std::string("Invalid regex pattern: ") + e.what());
    }
}

} // namespace kuserspace 
