#include "../include/Buffer.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <thread>
#include <chrono>
#include <errno.h>

namespace kuserspace {

// Static error string conversion
std::string Buffer::errorToString(Buffer::Error error) {
    switch (error) {
        case Buffer::Error::None: return "No error";
        case Buffer::Error::FileNotFound: return "File not found";
        case Buffer::Error::PermissionDenied: return "Permission denied";
        case Buffer::Error::BufferOverflow: return "Buffer overflow";
        case Buffer::Error::InvalidPath: return "Invalid path";
        case Buffer::Error::Timeout: return "Operation timed out";
        case Buffer::Error::IOError: return "I/O error";
        case Buffer::Error::InvalidOperation: return "Invalid operation";
        case Buffer::Error::BufferInvalid: return "Buffer invalid";
        case Buffer::Error::SystemError: return "System error";
        default: return "Unknown error";
    }
}

// Static error conversion from system error
Buffer::Error Buffer::systemErrorToError(const std::error_code& error) {
    if (!error) return Buffer::Error::None;
    
    switch (error.value()) {
        case ENOENT: return Buffer::Error::FileNotFound;
        case EACCES: return Buffer::Error::PermissionDenied;
        case EINVAL: return Buffer::Error::InvalidPath;
        case EIO: return Buffer::Error::IOError;
        default: return Buffer::Error::SystemError;
    }
}

Buffer::Buffer() 
    : config{
        .maxBufferSize = 1024 * 1024,  // 1MB default
        .refreshInterval = std::chrono::milliseconds(1000),
        .autoRefresh = false,
        .mode = Mode::Read,
        .policy = Policy::Default,
        .createIfNotExists = false,
        .truncateOnWrite = false,
        .permissions = std::filesystem::perms::owner_read | std::filesystem::perms::owner_write
    },
    state{
        .data = {},
        .lastUpdate = std::chrono::system_clock::now(),
        .size = 0,
        .isValid = false,
        .lastError = Buffer::Error::None,
        .systemError = std::error_code()
    } {
}

Buffer::Buffer(const std::string& path, Mode mode) : Buffer() {
    config.mode = mode;
    read(path);
}

Buffer::~Buffer() {
    clear();
}

void Buffer::updateLastError(Error error, const std::error_code& sysError) {
    state.lastError = error;
    state.systemError = sysError;
}

bool Buffer::checkPermissions(const std::string& path) const {
    try {
        auto perms = std::filesystem::status(path).permissions();
        return (perms & std::filesystem::perms::owner_read) != std::filesystem::perms::none;
    } catch (const std::filesystem::filesystem_error& e) {
        return false;
    }
}

bool Buffer::validatePath(const std::string& path) const {
    try {
        return std::filesystem::exists(path) || config.createIfNotExists;
    } catch (const std::filesystem::filesystem_error& e) {
        return false;
    }
}

std::pair<bool, Buffer::Error> Buffer::read(const std::string& path) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    
    if (!validatePath(path)) {
        updateLastError(Buffer::Error::FileNotFound);
        return {false, Buffer::Error::FileNotFound};
    }
    
    if (!checkPermissions(path)) {
        updateLastError(Buffer::Error::PermissionDenied);
        return {false, Buffer::Error::PermissionDenied};
    }
    
    try {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            updateLastError(Buffer::Error::IOError);
            return {false, Buffer::Error::IOError};
        }
        
        // Get file size
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        
        if (fileSize > config.maxBufferSize) {
            updateLastError(Buffer::Error::BufferOverflow);
            return {false, Buffer::Error::BufferOverflow};
        }
        
        // Read file
        state.data.resize(fileSize);
        file.read(state.data.data(), fileSize);
        
        if (!file) {
            updateLastError(Buffer::Error::IOError);
            return {false, Buffer::Error::IOError};
        }
        
        state.size = fileSize;
        state.isValid = true;
        state.lastUpdate = std::chrono::system_clock::now();
        currentPath = path;
        updateLastError(Buffer::Error::None);
        
        return {true, Buffer::Error::None};
    } catch (const std::exception& e) {
        updateLastError(Buffer::Error::SystemError);
        return {false, Buffer::Error::SystemError};
    }
}

std::pair<bool, Buffer::Error> Buffer::write(const std::string& path, const std::string& data) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    
    if (!validatePath(path)) {
        updateLastError(Buffer::Error::InvalidPath);
        return {false, Buffer::Error::InvalidPath};
    }
    
    try {
        std::ios_base::openmode mode = std::ios::binary;
        if (config.truncateOnWrite) {
            mode |= std::ios::trunc;
        }
        
        std::ofstream file(path, mode);
        if (!file) {
            updateLastError(Buffer::Error::IOError);
            return {false, Buffer::Error::IOError};
        }
        
        file.write(data.data(), data.size());
        
        if (!file) {
            updateLastError(Buffer::Error::IOError);
            return {false, Buffer::Error::IOError};
        }
        
        // Update buffer if this is the current file
        if (path == currentPath) {
            state.data.assign(data.begin(), data.end());
            state.size = data.size();
            state.isValid = true;
            state.lastUpdate = std::chrono::system_clock::now();
        }
        
        updateLastError(Buffer::Error::None);
        return {true, Buffer::Error::None};
    } catch (const std::exception& e) {
        updateLastError(Buffer::Error::SystemError);
        return {false, Buffer::Error::SystemError};
    }
}

std::pair<bool, Buffer::Error> Buffer::append(const std::string& path, const std::string& data) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    
    if (!validatePath(path)) {
        updateLastError(Buffer::Error::InvalidPath);
        return {false, Buffer::Error::InvalidPath};
    }
    
    try {
        std::ofstream file(path, std::ios::binary | std::ios::app);
        if (!file) {
            updateLastError(Buffer::Error::IOError);
            return {false, Buffer::Error::IOError};
        }
        
        file.write(data.data(), data.size());
        
        if (!file) {
            updateLastError(Buffer::Error::IOError);
            return {false, Buffer::Error::IOError};
        }
        
        // Update buffer if this is the current file
        if (path == currentPath) {
            state.data.insert(state.data.end(), data.begin(), data.end());
            state.size += data.size();
            state.lastUpdate = std::chrono::system_clock::now();
        }
        
        updateLastError(Buffer::Error::None);
        return {true, Buffer::Error::None};
    } catch (const std::exception& e) {
        updateLastError(Buffer::Error::SystemError);
        return {false, Buffer::Error::SystemError};
    }
}

std::pair<bool, Buffer::Error> Buffer::create(const std::string& path) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    
    try {
        if (std::filesystem::exists(path)) {
            updateLastError(Buffer::Error::InvalidOperation);
            return {false, Buffer::Error::InvalidOperation};
        }
        
        std::ofstream file(path);
        if (!file) {
            updateLastError(Buffer::Error::IOError);
            return {false, Buffer::Error::IOError};
        }
        
        std::filesystem::permissions(path, config.permissions);
        updateLastError(Buffer::Error::None);
        return {true, Buffer::Error::None};
    } catch (const std::exception& e) {
        updateLastError(Buffer::Error::SystemError);
        return {false, Buffer::Error::SystemError};
    }
}

std::pair<bool, Buffer::Error> Buffer::remove(const std::string& path) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    
    try {
        if (!std::filesystem::exists(path)) {
            updateLastError(Buffer::Error::FileNotFound);
            return {false, Buffer::Error::FileNotFound};
        }
        
        std::filesystem::remove(path);
        
        if (path == currentPath) {
            clear();
        }
        
        updateLastError(Buffer::Error::None);
        return {true, Buffer::Error::None};
    } catch (const std::exception& e) {
        updateLastError(Buffer::Error::SystemError);
        return {false, Buffer::Error::SystemError};
    }
}

void Buffer::refresh() {
    std::unique_lock<std::shared_mutex> lock(mutex);
    
    if (currentPath.empty()) {
        updateLastError(Buffer::Error::InvalidOperation);
        return;
    }
    
    read(currentPath);
}

void Buffer::clear() {
    std::unique_lock<std::shared_mutex> lock(mutex);
    
    state.data.clear();
    state.size = 0;
    state.isValid = false;
    currentPath.clear();
    updateLastError(Buffer::Error::None);
}

bool Buffer::isValid() const {
    std::shared_lock<std::shared_mutex> lock(mutex);
    return state.isValid;
}

Buffer::Error Buffer::getLastError() const {
    std::shared_lock<std::shared_mutex> lock(mutex);
    return state.lastError;
}

std::error_code Buffer::getSystemError() const {
    std::shared_lock<std::shared_mutex> lock(mutex);
    return state.systemError;
}

std::string Buffer::getData() const {
    std::shared_lock<std::shared_mutex> lock(mutex);
    return std::string(state.data.begin(), state.data.end());
}

std::vector<std::string> Buffer::getLines() const {
    std::shared_lock<std::shared_mutex> lock(mutex);
    std::vector<std::string> lines;
    std::string data(state.data.begin(), state.data.end());
    std::istringstream stream(data);
    std::string line;
    
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    
    return lines;
}

std::optional<std::string> Buffer::getLine(size_t lineNumber) const {
    std::shared_lock<std::shared_mutex> lock(mutex);
    std::istringstream stream(std::string(state.data.begin(), state.data.end()));
    std::string line;
    
    for (size_t i = 0; i <= lineNumber; ++i) {
        if (!std::getline(stream, line)) {
            return std::nullopt;
        }
    }
    
    return line;
}

std::vector<char> Buffer::getRawData() const {
    std::shared_lock<std::shared_mutex> lock(mutex);
    return state.data;
}

void Buffer::setMaxBufferSize(size_t size) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    config.maxBufferSize = size;
}

void Buffer::setRefreshInterval(std::chrono::milliseconds interval) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    config.refreshInterval = interval;
}

void Buffer::setAutoRefresh(bool enable) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    config.autoRefresh = enable;
}

void Buffer::setMode(Mode mode) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    config.mode = mode;
}

void Buffer::setPolicy(Policy policy) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    config.policy = policy;
}

void Buffer::setCreateIfNotExists(bool enable) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    config.createIfNotExists = enable;
}

void Buffer::setTruncateOnWrite(bool enable) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    config.truncateOnWrite = enable;
}

void Buffer::setPermissions(std::filesystem::perms perms) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    config.permissions = perms;
}

bool Buffer::exists(const std::string& path) const {
    return std::filesystem::exists(path);
}

size_t Buffer::size() const {
    std::shared_lock<std::shared_mutex> lock(mutex);
    return state.size;
}

std::string Buffer::getCurrentPath() const {
    std::shared_lock<std::shared_mutex> lock(mutex);
    return currentPath;
}

Buffer::Mode Buffer::getMode() const {
    std::shared_lock<std::shared_mutex> lock(mutex);
    return config.mode;
}

Buffer::Policy Buffer::getPolicy() const {
    std::shared_lock<std::shared_mutex> lock(mutex);
    return config.policy;
}

std::filesystem::perms Buffer::getPermissions() const {
    std::shared_lock<std::shared_mutex> lock(mutex);
    return config.permissions;
}

std::pair<bool, Buffer::Error> Buffer::tryRead(const std::string& path, std::chrono::milliseconds timeout) {
    auto start = std::chrono::steady_clock::now();
    while (true) {
        auto result = read(path);
        if (result.first || result.second != Buffer::Error::PermissionDenied) {
            return result;
        }
        
        if (std::chrono::steady_clock::now() - start > timeout) {
            return {false, Buffer::Error::Timeout};
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

std::pair<bool, Buffer::Error> Buffer::tryWrite(const std::string& path, const std::string& data, std::chrono::milliseconds timeout) {
    auto start = std::chrono::steady_clock::now();
    while (true) {
        auto result = write(path, data);
        if (result.first || result.second != Buffer::Error::PermissionDenied) {
            return result;
        }
        
        if (std::chrono::steady_clock::now() - start > timeout) {
            return {false, Buffer::Error::Timeout};
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

std::pair<bool, Buffer::Error> Buffer::copy(const std::string& source, const std::string& destination) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    
    try {
        std::filesystem::copy_file(source, destination);
        updateLastError(Buffer::Error::None);
        return {true, Buffer::Error::None};
    } catch (const std::filesystem::filesystem_error& e) {
        updateLastError(Buffer::Error::SystemError, e.code());
        return {false, Buffer::Error::SystemError};
    }
}

std::pair<bool, Buffer::Error> Buffer::move(const std::string& source, const std::string& destination) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    
    try {
        std::filesystem::rename(source, destination);
        if (source == currentPath) {
            currentPath = destination;
        }
        updateLastError(Buffer::Error::None);
        return {true, Buffer::Error::None};
    } catch (const std::filesystem::filesystem_error& e) {
        updateLastError(Buffer::Error::SystemError, e.code());
        return {false, Buffer::Error::SystemError};
    }
}

std::pair<bool, Buffer::Error> Buffer::rename(const std::string& newPath) {
    if (currentPath.empty()) {
        updateLastError(Buffer::Error::InvalidOperation);
        return {false, Buffer::Error::InvalidOperation};
    }
    
    return move(currentPath, newPath);
}

bool Buffer::isOpen() const {
    std::shared_lock<std::shared_mutex> lock(mutex);
    return !currentPath.empty();
}

bool Buffer::isReadable() const {
    std::shared_lock<std::shared_mutex> lock(mutex);
    return config.mode == Mode::Read || config.mode == Mode::ReadWrite;
}

bool Buffer::isWritable() const {
    std::shared_lock<std::shared_mutex> lock(mutex);
    return config.mode == Mode::Write || config.mode == Mode::ReadWrite || config.mode == Mode::Append;
}

bool Buffer::isBinary() const {
    std::shared_lock<std::shared_mutex> lock(mutex);
    return config.mode == Mode::Binary;
}

std::chrono::system_clock::time_point Buffer::getLastUpdateTime() const {
    std::shared_lock<std::shared_mutex> lock(mutex);
    return state.lastUpdate;
}

} // namespace kuserspace 