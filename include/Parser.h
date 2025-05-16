// Malghumuy - Library: kuserspace
#pragma once

#include <string>
#include <regex>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>
#include <functional>
#include <filesystem>
#include <system_error>

namespace kuserspace {

/**
 * @class Parser
 * @brief Thread-safe parser for system information files with regex pattern matching
 * 
 * This class provides efficient parsing of system information files using pre-compiled
 * regex patterns. It includes caching of compiled patterns and thread-safe operations.
 */
class Parser {
public:
    /**
     * @brief Get the singleton instance of the Parser
     * @return Reference to the Parser instance
     */
    static Parser& getInstance();

    // Delete copy and move constructors
    Parser(const Parser&) = delete;
    Parser(Parser&&) = delete;
    Parser& operator=(const Parser&) = delete;
    Parser& operator=(Parser&&) = delete;

    /**
     * @brief Common regex patterns for system information parsing
     */
    struct Patterns {
        // CPU info patterns
        static constexpr const char* CPU_PROCESSOR = R"(processor\s+:\s+(\d+))";
        static constexpr const char* CPU_PHYSICAL_ID = R"(physical id\s+:\s+(\d+))";
        static constexpr const char* CPU_MODEL_NAME = R"(model name\s+:\s+(.+))";
        static constexpr const char* CPU_VENDOR_ID = R"(vendor_id\s+:\s+(.+))";
        
        // Memory info patterns
        static constexpr const char* MEM_TOTAL = R"(MemTotal:\s+(\d+))";
        static constexpr const char* MEM_FREE = R"(MemFree:\s+(\d+))";
        static constexpr const char* MEM_CACHED = R"(Cached:\s+(\d+))";
        static constexpr const char* MEM_BUFFERS = R"(Buffers:\s+(\d+))";
        static constexpr const char* MEM_ACTIVE = R"(Active:\s+(\d+))";
        static constexpr const char* MEM_INACTIVE = R"(Inactive:\s+(\d+))";
        static constexpr const char* MEM_ACTIVE_ANON = R"(Active\(anon\):\s+(\d+))";
        static constexpr const char* MEM_INACTIVE_ANON = R"(Inactive\(anon\):\s+(\d+))";
        static constexpr const char* MEM_ACTIVE_FILE = R"(Active\(file\):\s+(\d+))";
        static constexpr const char* MEM_INACTIVE_FILE = R"(Inactive\(file\):\s+(\d+))";
        static constexpr const char* MEM_UNEVICTABLE = R"(Unevictable:\s+(\d+))";
        static constexpr const char* MEM_MLOCKED = R"(Mlocked:\s+(\d+))";
        static constexpr const char* MEM_HIGH_TOTAL = R"(HighTotal:\s+(\d+))";
        static constexpr const char* MEM_HIGH_FREE = R"(HighFree:\s+(\d+))";
        static constexpr const char* MEM_LOW_TOTAL = R"(LowTotal:\s+(\d+))";
        static constexpr const char* MEM_LOW_FREE = R"(LowFree:\s+(\d+))";
        static constexpr const char* MEM_HUGE_PAGES_TOTAL = R"(HugePages_Total:\s+(\d+))";
        static constexpr const char* MEM_HUGE_PAGES_FREE = R"(HugePages_Free:\s+(\d+))";
        static constexpr const char* MEM_HUGE_PAGES_RSVD = R"(HugePages_Rsvd:\s+(\d+))";
        static constexpr const char* MEM_HUGE_PAGES_SURP = R"(HugePages_Surp:\s+(\d+))";
        static constexpr const char* MEM_HUGE_PAGE_SIZE = R"(Hugepagesize:\s+(\d+))";
        static constexpr const char* MEM_DIRECT_MAP_4K = R"(DirectMap4k:\s+(\d+))";
        static constexpr const char* MEM_DIRECT_MAP_2M = R"(DirectMap2M:\s+(\d+))";
        static constexpr const char* MEM_DIRECT_MAP_1G = R"(DirectMap1G:\s+(\d+))";

        // Memory zone patterns
        static constexpr const char* MEM_ZONE_FREE = R"(free\s+(\d+))";
        static constexpr const char* MEM_ZONE_MIN = R"(min\s+(\d+))";
        static constexpr const char* MEM_ZONE_LOW = R"(low\s+(\d+))";
        static constexpr const char* MEM_ZONE_HIGH = R"(high\s+(\d+))";

        // NUMA patterns
        static constexpr const char* MEM_NUMA_TOTAL = R"(Node\s+\d+\s+MemTotal:\s+(\d+))";
        static constexpr const char* MEM_NUMA_FREE = R"(Node\s+\d+\s+MemFree:\s+(\d+))";
        static constexpr const char* MEM_NUMA_USED = R"(Node\s+\d+\s+MemUsed:\s+(\d+))";
    };

    /**
     * @brief Structure to hold parsing results
     */
    struct ParseResult {
        bool success;           ///< Whether the parsing was successful
        std::string value;      ///< The parsed value
        std::string error;      ///< Error message if parsing failed
        
        ParseResult(bool s, const std::string& v = "", const std::string& e = "")
            : success(s), value(v), error(e) {}
    };

    /**
     * @brief Parse a single line with a specific pattern
     * @param line The line to parse
     * @param pattern The regex pattern to match
     * @return ParseResult containing the parsing result
     */
    ParseResult parseLine(const std::string& line, const std::string& pattern);
    
    /**
     * @brief Parse a file and extract all matching patterns
     * @param filepath Path to the file to parse
     * @param patterns Vector of regex patterns to match
     * @return Vector of ParseResults for all matches
     */
    std::vector<ParseResult> parseFile(const std::filesystem::path& filepath, 
                                     const std::vector<std::string>& patterns);
    
    /**
     * @brief Parse a file and extract values for a specific pattern
     * @param filepath Path to the file to parse
     * @param pattern The regex pattern to match
     * @return Vector of matched values
     * @throw std::runtime_error if file cannot be opened
     */
    std::vector<std::string> extractValues(const std::filesystem::path& filepath,
                                         const std::string& pattern);
    
    /**
     * @brief Parse a file and map results to a specific pattern
     * @param filepath Path to the file to parse
     * @param keyPattern Pattern for the key
     * @param valuePattern Pattern for the value
     * @return Map of key-value pairs
     * @throw std::runtime_error if file cannot be opened
     */
    std::unordered_map<std::string, std::string> parseToMap(
        const std::filesystem::path& filepath,
        const std::string& keyPattern,
        const std::string& valuePattern);
    
    /**
     * @brief Parse a file with a custom handler
     * @param filepath Path to the file to parse
     * @param handler Function to handle each match
     * @param pattern The regex pattern to match
     * @throw std::runtime_error if file cannot be opened
     */
    void parseWithHandler(const std::filesystem::path& filepath,
                         const std::function<void(const std::string&, const std::smatch&)>& handler,
                         const std::string& pattern);

    /**
     * @brief Clear the regex cache
     */
    void clearCache();

private:
    Parser() = default;
    ~Parser() = default;

    /**
     * @brief Get or create a compiled regex
     * @param pattern The regex pattern to compile
     * @return Compiled regex
     * @throw std::runtime_error if pattern is invalid
     */
    std::regex getRegex(const std::string& pattern);

    // Cache of compiled regex patterns
    std::unordered_map<std::string, std::regex> regexCache;
    mutable std::mutex cacheMutex;
};

} // namespace kuserspace
