// Malghumuy - Library: kuserspace
#pragma once

#include "KSpace.h"
#include <string>
#include <vector>
#include <fstream>
#include <memory>
#include <shared_mutex>
#include <chrono>
#include <optional>
#include <system_error>
#include <filesystem>
#include <array>

namespace kuserspace {

class Buffer {
public:
    // Error codes - simplified for read-only operations
    enum class Error {
        None = 0,
        FileNotFound,
        PermissionDenied,
        BufferOverflow,
        InvalidPath,
        Timeout,
        IOError,
        SystemError
    };

    // Simplified modes - only what we need for reading
    enum class Mode {
        Read,
        Binary
    };

    // Simplified policies - only what we need for reading
    enum class Policy {
        Default,
        NoCache
    };

    // Simplified configuration
    struct Config {
        size_t maxBufferSize;
        std::chrono::milliseconds refreshInterval;
        bool autoRefresh;
        Mode mode;
        Policy policy;
        
        // Performance settings
        size_t readAheadSize;        // Size of read-ahead buffer
        bool useMemoryMapping;       // Use memory mapping for large files
    };

    // Simplified state
    struct State {
        std::vector<char> data;
        std::chrono::system_clock::time_point lastUpdate;
        size_t size;
        bool isValid;
        Error lastError;
        std::error_code systemError;
        
        // Performance-related state
        std::array<char, 8192> readBuffer;  // Static read buffer
        bool isMapped;                      // Whether the file is memory mapped
    };

    Buffer();
    explicit Buffer(const std::string& path, Mode mode = Mode::Read);
    ~Buffer();

    // Core read operations
    std::pair<bool, Error> read(const std::string& path);
    void refresh();
    void clear();
    
    // Status queries
    bool isValid() const;
    Error getLastError() const;
    std::error_code getSystemError() const;
    
    // Data access
    std::string getData() const;
    std::vector<std::string> getLines() const;
    std::optional<std::string> getLine(size_t lineNumber) const;
    std::vector<char> getRawData() const;
    
    // Configuration
    void setMaxBufferSize(size_t size);
    void setRefreshInterval(std::chrono::milliseconds interval);
    void setAutoRefresh(bool enable);
    void setMode(Mode mode);
    void setPolicy(Policy policy);
    
    // Performance settings
    void setReadAheadSize(size_t size);
    void setUseMemoryMapping(bool enable);
    
    // Utility methods
    bool exists(const std::string& path) const;
    size_t size() const;
    std::string getCurrentPath() const;
    Mode getMode() const;
    Policy getPolicy() const;
    
    // Thread-safe operations
    std::pair<bool, Error> tryRead(const std::string& path, std::chrono::milliseconds timeout);
    
    // Buffer state queries
    bool isOpen() const;
    bool isReadable() const;
    bool isBinary() const;
    std::chrono::system_clock::time_point getLastUpdateTime() const;
    
    // Error handling
    static std::string errorToString(Error error);
    static Error systemErrorToError(const std::error_code& error);

private:
    // Internal helpers
    bool checkPermissions(const std::string& path) const;
    bool validatePath(const std::string& path) const;
    void updateLastError(Error error, const std::error_code& sysError = std::error_code());

    // Performance helpers
    bool initializeBuffers();
    void cleanupBuffers();
    bool mapFile(const std::string& path);
    void unmapFile();

    // Member variables
    std::string currentPath;
    Config config;
    State state;
    mutable std::shared_mutex mutex;
};

} // namespace kuserspace
