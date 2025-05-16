// Malghumuy - Library: kuserspace
#include "../include/Buffer.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <thread>
#include <chrono>
#include <errno.h>
#include <system_error>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <mutex>
#include <shared_mutex>

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

Buffer::Buffer() {
    config.maxBufferSize = 1024 * 1024 * 1024;  // 1GB default
    config.refreshInterval = std::chrono::seconds(1);
    config.autoRefresh = false;
    config.mode = Mode::Read;
    config.policy = Policy::Default;
    
    // Initialize performance settings
    config.readAheadSize = 8192;
    config.useMemoryMapping = true;
    
    initializeBuffers();
}

Buffer::Buffer(const std::string& path, Mode mode) : Buffer() {
    config.mode = mode;
    read(path);
}

Buffer::~Buffer() {
    cleanupBuffers();
}

bool Buffer::initializeBuffers() {
    std::unique_lock<std::shared_mutex> lock(mutex);
    state.readBuffer.fill(0);
    state.isMapped = false;
    return true;
}

void Buffer::cleanupBuffers() {
    std::unique_lock<std::shared_mutex> lock(mutex);
    if (state.isMapped) {
        unmapFile();
    }
    state.data.clear();
    state.size = 0;
    state.isValid = false;
}

bool Buffer::mapFile(const std::string& path) {
    if (!config.useMemoryMapping) return false;
    
    int fd = open(path.c_str(), O_RDONLY);
    if (fd == -1) return false;
    
    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        return false;
    }
    
    void* mapped = mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    
    if (mapped == MAP_FAILED) return false;
    
    state.data.assign(static_cast<char*>(mapped), static_cast<char*>(mapped) + st.st_size);
    munmap(mapped, st.st_size);
    state.isMapped = true;
    return true;
}

void Buffer::unmapFile() {
    if (!state.isMapped) return;
    state.data.clear();
    state.isMapped = false;
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
        return std::filesystem::exists(path);
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
        // Try memory mapping first for large files
        if (config.useMemoryMapping && mapFile(path)) {
            state.size = state.data.size();
            state.isValid = true;
            state.lastUpdate = std::chrono::system_clock::now();
            currentPath = path;
            updateLastError(Buffer::Error::None);
            return {true, Buffer::Error::None};
        }
        
        // Fall back to buffered I/O
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
        
        // Read file using buffered I/O
        state.data.resize(fileSize);
        size_t totalRead = 0;
        
        while (totalRead < fileSize) {
            size_t toRead = std::min(config.readAheadSize, fileSize - totalRead);
            file.read(state.data.data() + totalRead, toRead);
            totalRead += file.gcount();
        }
        
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

void Buffer::refresh() {
    std::unique_lock<std::shared_mutex> lock(mutex);
    
    if (currentPath.empty()) {
        updateLastError(Buffer::Error::InvalidPath);
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

void Buffer::setReadAheadSize(size_t size) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    config.readAheadSize = size;
}

void Buffer::setUseMemoryMapping(bool enable) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    config.useMemoryMapping = enable;
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

bool Buffer::isOpen() const {
    std::shared_lock<std::shared_mutex> lock(mutex);
    return !currentPath.empty();
}

bool Buffer::isReadable() const {
    std::shared_lock<std::shared_mutex> lock(mutex);
    return true;  // Always readable in read-only mode
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
