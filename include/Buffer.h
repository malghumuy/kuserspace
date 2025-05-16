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

namespace kuserspace {

class Buffer {
public:
    // Error codes
    enum class Error {
        None = 0,
        FileNotFound,
        PermissionDenied,
        BufferOverflow,
        InvalidPath,
        Timeout,
        IOError,
        InvalidOperation,
        BufferInvalid,
        SystemError
    };

    // Buffer modes
    enum class Mode {
        Read,
        Write,
        Append,
        ReadWrite,
        Binary
    };

    // Buffer policies
    enum class Policy {
        Default,
        NoCache,
        Immediate,
        Lazy,
        Periodic
    };

    // Buffer configuration
    struct Config {
        size_t maxBufferSize;
        std::chrono::milliseconds refreshInterval;
        bool autoRefresh;
        Mode mode;
        Policy policy;
        bool createIfNotExists;
        bool truncateOnWrite;
        std::filesystem::perms permissions;
    };

    // Buffer state
    struct State {
        std::vector<char> data;
        std::chrono::system_clock::time_point lastUpdate;
        size_t size;
        bool isValid;
        Error lastError;
        std::error_code systemError;
    };

    Buffer();
    explicit Buffer(const std::string& path, Mode mode = Mode::Read);
    ~Buffer();

    // File operations with error handling
    std::pair<bool, Error> read(const std::string& path);
    std::pair<bool, Error> write(const std::string& path, const std::string& data);
    std::pair<bool, Error> append(const std::string& path, const std::string& data);
    std::pair<bool, Error> create(const std::string& path);
    std::pair<bool, Error> remove(const std::string& path);
    
    // Buffer operations
    void refresh();
    void clear();
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
    void setCreateIfNotExists(bool enable);
    void setTruncateOnWrite(bool enable);
    void setPermissions(std::filesystem::perms perms);
    
    // Utility methods
    bool exists(const std::string& path) const;
    size_t size() const;
    std::string getCurrentPath() const;
    Mode getMode() const;
    Policy getPolicy() const;
    std::filesystem::perms getPermissions() const;
    
    // Thread-safe operations
    std::pair<bool, Error> tryRead(const std::string& path, std::chrono::milliseconds timeout);
    std::pair<bool, Error> tryWrite(const std::string& path, const std::string& data, std::chrono::milliseconds timeout);
    
    // File system operations
    std::pair<bool, Error> copy(const std::string& source, const std::string& destination);
    std::pair<bool, Error> move(const std::string& source, const std::string& destination);
    std::pair<bool, Error> rename(const std::string& newPath);
    
    // Buffer state queries
    bool isOpen() const;
    bool isReadable() const;
    bool isWritable() const;
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

    // Member variables
    std::string currentPath;
    Config config;
    State state;
    mutable std::shared_mutex mutex;
};

} // namespace kuserspace
