// Malghumuy - Library: kuserspace
#include "../include/Memory.h"
#include "../include/Buffer.h"
#include <thread>
#include <chrono>
#include <sstream>
#include <regex>
#include <filesystem>
#include <fstream>

namespace kuserspace {

// Initialize static member
Memory* Memory::instance = nullptr;

Memory::Memory() : isUpdating(false) {
    updateStats();
}

Memory::~Memory() {
    stopMonitoring();
    delete instance;
    instance = nullptr;
}

void Memory::updateStats() {
    std::unique_lock<std::shared_mutex> lock(mutex);
    readProcMeminfo();
    readProcSwaps();
    readMemoryZones();
    readNumaInfo();
    readHugePages();
}

void Memory::readProcMeminfo() {
    Buffer buffer;
    auto result = buffer.read("/proc/meminfo");
    
    if (!result.first) {
        return;
    }
    
    std::string content = buffer.getData();
    std::istringstream stream(content);
    std::string line;
    
    // Regular expressions for parsing
    std::regex memTotalRegex(R"(MemTotal:\s+(\d+))");
    std::regex memFreeRegex(R"(MemFree:\s+(\d+))");
    std::regex cachedRegex(R"(Cached:\s+(\d+))");
    std::regex buffersRegex(R"(Buffers:\s+(\d+))");
    std::regex activeRegex(R"(Active:\s+(\d+))");
    std::regex inactiveRegex(R"(Inactive:\s+(\d+))");
    std::regex activeAnonRegex(R"(Active\(anon\):\s+(\d+))");
    std::regex inactiveAnonRegex(R"(Inactive\(anon\):\s+(\d+))");
    std::regex activeFileRegex(R"(Active\(file\):\s+(\d+))");
    std::regex inactiveFileRegex(R"(Inactive\(file\):\s+(\d+))");
    std::regex unevictableRegex(R"(Unevictable:\s+(\d+))");
    std::regex mlockedRegex(R"(Mlocked:\s+(\d+))");
    std::regex highTotalRegex(R"(HighTotal:\s+(\d+))");
    std::regex highFreeRegex(R"(HighFree:\s+(\d+))");
    std::regex lowTotalRegex(R"(LowTotal:\s+(\d+))");
    std::regex lowFreeRegex(R"(LowFree:\s+(\d+))");
    std::regex hugePagesTotalRegex(R"(HugePages_Total:\s+(\d+))");
    std::regex hugePagesFreeRegex(R"(HugePages_Free:\s+(\d+))");
    std::regex hugePagesRsvdRegex(R"(HugePages_Rsvd:\s+(\d+))");
    std::regex hugePagesSurpRegex(R"(HugePages_Surp:\s+(\d+))");
    std::regex hugePageSizeRegex(R"(Hugepagesize:\s+(\d+))");
    std::regex directMap4kRegex(R"(DirectMap4k:\s+(\d+))");
    std::regex directMap2MRegex(R"(DirectMap2M:\s+(\d+))");
    std::regex directMap1GRegex(R"(DirectMap1G:\s+(\d+))");
    
    std::smatch matches;
    
    while (std::getline(stream, line)) {
        if (std::regex_search(line, matches, memTotalRegex)) {
            currentState.total = std::stoull(matches[1]) * 1024;
        }
        else if (std::regex_search(line, matches, memFreeRegex)) {
            currentState.free = std::stoull(matches[1]) * 1024;
        }
        else if (std::regex_search(line, matches, cachedRegex)) {
            currentState.cached = std::stoull(matches[1]) * 1024;
        }
        else if (std::regex_search(line, matches, buffersRegex)) {
            currentState.buffers = std::stoull(matches[1]) * 1024;
        }
        else if (std::regex_search(line, matches, activeRegex)) {
            currentState.active = std::stoull(matches[1]) * 1024;
        }
        else if (std::regex_search(line, matches, inactiveRegex)) {
            currentState.inactive = std::stoull(matches[1]) * 1024;
        }
        else if (std::regex_search(line, matches, activeAnonRegex)) {
            currentState.activeAnon = std::stoull(matches[1]) * 1024;
        }
        else if (std::regex_search(line, matches, inactiveAnonRegex)) {
            currentState.inactiveAnon = std::stoull(matches[1]) * 1024;
        }
        else if (std::regex_search(line, matches, activeFileRegex)) {
            currentState.activeFile = std::stoull(matches[1]) * 1024;
        }
        else if (std::regex_search(line, matches, inactiveFileRegex)) {
            currentState.inactiveFile = std::stoull(matches[1]) * 1024;
        }
        else if (std::regex_search(line, matches, unevictableRegex)) {
            currentState.unevictable = std::stoull(matches[1]) * 1024;
        }
        else if (std::regex_search(line, matches, mlockedRegex)) {
            currentState.mlocked = std::stoull(matches[1]) * 1024;
        }
        else if (std::regex_search(line, matches, highTotalRegex)) {
            currentState.highTotal = std::stoull(matches[1]) * 1024;
        }
        else if (std::regex_search(line, matches, highFreeRegex)) {
            currentState.highFree = std::stoull(matches[1]) * 1024;
        }
        else if (std::regex_search(line, matches, lowTotalRegex)) {
            currentState.lowTotal = std::stoull(matches[1]) * 1024;
        }
        else if (std::regex_search(line, matches, lowFreeRegex)) {
            currentState.lowFree = std::stoull(matches[1]) * 1024;
        }
        else if (std::regex_search(line, matches, hugePagesTotalRegex)) {
            currentState.hugePagesTotal = std::stoull(matches[1]);
        }
        else if (std::regex_search(line, matches, hugePagesFreeRegex)) {
            currentState.hugePagesFree = std::stoull(matches[1]);
        }
        else if (std::regex_search(line, matches, hugePagesRsvdRegex)) {
            currentState.hugePagesRsvd = std::stoull(matches[1]);
        }
        else if (std::regex_search(line, matches, hugePagesSurpRegex)) {
            currentState.hugePagesSurp = std::stoull(matches[1]);
        }
        else if (std::regex_search(line, matches, hugePageSizeRegex)) {
            currentState.hugePageSize = std::stoull(matches[1]) * 1024;
        }
        else if (std::regex_search(line, matches, directMap4kRegex)) {
            currentState.directMap4k = std::stoull(matches[1]) * 1024;
        }
        else if (std::regex_search(line, matches, directMap2MRegex)) {
            currentState.directMap2M = std::stoull(matches[1]) * 1024;
        }
        else if (std::regex_search(line, matches, directMap1GRegex)) {
            currentState.directMap1G = std::stoull(matches[1]) * 1024;
        }
    }
}

void Memory::readProcSwaps() {
    Buffer buffer;
    auto result = buffer.read("/proc/swaps");
    
    if (!result.first) {
        return;
    }
    
    std::string content = buffer.getData();
    std::istringstream stream(content);
    std::string line;
    
    // Skip header line
    std::getline(stream, line);
    
    currentState.swapTotal = 0;
    currentState.swapFree = 0;
    
    while (std::getline(stream, line)) {
        std::istringstream lineStream(line);
        std::string filename, type;
        size_t size, used;
        
        lineStream >> filename >> type >> size >> used;
        
        currentState.swapTotal += size * 1024; // Convert KB to bytes
        currentState.swapFree += (size - used) * 1024;
    }
}

void Memory::readMemoryZones() {
    const std::string zonePath = "/proc/zoneinfo";
    Buffer buffer;
    auto result = buffer.read(zonePath);
    
    if (!result.first) {
        return;
    }
    
    std::string content = buffer.getData();
    std::istringstream stream(content);
    std::string line;
    std::string currentZone;
    
    while (std::getline(stream, line)) {
        if (line.find("Node") != std::string::npos && line.find("zone") != std::string::npos) {
            currentZone = line.substr(line.find("zone") + 5);
            currentZone = currentZone.substr(0, currentZone.find_first_of(" 	"));
            currentState.zones[currentZone] = State::ZoneInfo();
        }
        else if (!currentZone.empty()) {
            std::istringstream lineStream(line);
            std::string key;
            size_t value;
            
            lineStream >> key >> value;
            
            if (key == "free") currentState.zones[currentZone].free = value;
            else if (key == "min") currentState.zones[currentZone].min = value;
            else if (key == "low") currentState.zones[currentZone].low = value;
            else if (key == "high") currentState.zones[currentZone].high = value;
            else if (key == "spanned") currentState.zones[currentZone].spanned = value;
            else if (key == "present") currentState.zones[currentZone].present = value;
            else if (key == "managed") currentState.zones[currentZone].managed = value;
            else if (key == "protection") currentState.zones[currentZone].protection = value;
            else if (key == "nr_free_pages") currentState.zones[currentZone].nrFreePages = value;
            else if (key == "nr_inactive") currentState.zones[currentZone].nrInactive = value;
            else if (key == "nr_active") currentState.zones[currentZone].nrActive = value;
            else if (key == "nr_unevictable") currentState.zones[currentZone].nrUnevictable = value;
            else if (key == "nr_writeback") currentState.zones[currentZone].nrWriteback = value;
            else if (key == "nr_slab_reclaimable") currentState.zones[currentZone].nrSlabReclaimable = value;
            else if (key == "nr_slab_unreclaimable") currentState.zones[currentZone].nrSlabUnreclaimable = value;
            else if (key == "nr_kernel_stack") currentState.zones[currentZone].nrKernelStack = value;
            else if (key == "nr_page_table") currentState.zones[currentZone].nrPageTable = value;
            else if (key == "nr_bounce") currentState.zones[currentZone].nrBounce = value;
            else if (key == "nr_free_cma") currentState.zones[currentZone].nrFreeCMA = value;
            else if (key == "nr_lowmem_reserve") currentState.zones[currentZone].nrLowmemReserve = value;
        }
    }
}

void Memory::readNumaInfo() {
    const std::string numaPath = "/sys/devices/system/node/";
    for (const auto& entry : std::filesystem::directory_iterator(numaPath)) {
        if (entry.path().filename().string().find("node") == 0) {
            int nodeId = std::stoi(entry.path().filename().string().substr(4));
            State::NumaNode node;
            
            // Read node memory info
            Buffer buffer;
            auto result = buffer.read(entry.path().string() + "/meminfo");
            if (result.first) {
                std::string content = buffer.getData();
                std::istringstream stream(content);
                std::string line;
                
                while (std::getline(stream, line)) {
                    if (line.find("MemTotal") != std::string::npos) {
                        std::regex totalRegex(R"(MemTotal:\s+(\d+))");
                        std::smatch matches;
                        if (std::regex_search(line, matches, totalRegex)) {
                            node.total = std::stoull(matches[1]) * 1024;
                        }
                    }
                    else if (line.find("MemFree") != std::string::npos) {
                        std::regex freeRegex(R"(MemFree:\s+(\d+))");
                        std::smatch matches;
                        if (std::regex_search(line, matches, freeRegex)) {
                            node.free = std::stoull(matches[1]) * 1024;
                        }
                    }
                }
            }
            
            // Read node distances
            result = buffer.read(entry.path().string() + "/distance");
            if (result.first) {
                std::string content = buffer.getData();
                std::istringstream stream(content);
                std::string line;
                std::getline(stream, line);
                std::istringstream distanceStream(line);
                size_t distance;
                while (distanceStream >> distance) {
                    node.distances.push_back(distance);
                }
            }
            
            node.used = node.total - node.free;
            currentState.numaNodes[nodeId] = node;
        }
    }
}

void Memory::readHugePages() {
    const std::string hugePagesPath = "/sys/kernel/mm/hugepages/";
    for (const auto& entry : std::filesystem::directory_iterator(hugePagesPath)) {
        if (entry.path().filename().string().find("hugepages-") == 0) {
            std::string sizeStr = entry.path().filename().string().substr(10);
            size_t pageSize = std::stoull(sizeStr) * 1024; // Convert KB to bytes
            
            Buffer buffer;
            auto result = buffer.read(entry.path().string() + "/nr_hugepages");
            if (result.first) {
                currentState.hugePagesTotal = std::stoull(buffer.getData());
            }
            
            result = buffer.read(entry.path().string() + "/free_hugepages");
            if (result.first) {
                currentState.hugePagesFree = std::stoull(buffer.getData());
            }
            
            result = buffer.read(entry.path().string() + "/resv_hugepages");
            if (result.first) {
                currentState.hugePagesRsvd = std::stoull(buffer.getData());
            }
            
            result = buffer.read(entry.path().string() + "/surplus_hugepages");
            if (result.first) {
                currentState.hugePagesSurp = std::stoull(buffer.getData());
            }
            
            currentState.hugePageSize = pageSize;
            break; // We only need the first huge page size
        }
    }
}

Memory::Stats Memory::getStats() {
    std::shared_lock<std::shared_mutex> lock(mutex);
    return {
        currentState.total,
        currentState.free,
        currentState.cached,
        currentState.buffers,
        currentState.swapTotal,
        currentState.swapFree,
        currentState.active,
        currentState.inactive,
        currentState.activeAnon,
        currentState.inactiveAnon,
        currentState.activeFile,
        currentState.inactiveFile,
        currentState.unevictable,
        currentState.mlocked,
        currentState.highTotal,
        currentState.highFree,
        currentState.lowTotal,
        currentState.lowFree,
        currentState.hugePagesTotal,
        currentState.hugePagesFree,
        currentState.hugePagesRsvd,
        currentState.hugePagesSurp,
        currentState.hugePageSize,
        currentState.directMap4k,
        currentState.directMap2M,
        currentState.directMap1G
    };
}

std::map<std::string, Memory::ZoneStats> Memory::getZoneStats() {
    std::shared_lock<std::shared_mutex> lock(mutex);
    std::map<std::string, ZoneStats> result;
    
    for (const auto& [zoneName, zoneInfo] : currentState.zones) {
        result[zoneName] = {
            zoneInfo.free,
            zoneInfo.min,
            zoneInfo.low,
            zoneInfo.high,
            zoneInfo.spanned,
            zoneInfo.present,
            zoneInfo.managed,
            zoneInfo.protection,
            zoneInfo.nrFreePages,
            zoneInfo.nrInactive,
            zoneInfo.nrActive,
            zoneInfo.nrUnevictable,
            zoneInfo.nrWriteback,
            zoneInfo.nrSlabReclaimable,
            zoneInfo.nrSlabUnreclaimable,
            zoneInfo.nrKernelStack,
            zoneInfo.nrPageTable,
            zoneInfo.nrBounce,
            zoneInfo.nrFreeCMA,
            zoneInfo.nrLowmemReserve
        };
    }
    
    return result;
}

Memory::ZoneStats Memory::getZoneStats(const std::string& zoneName) {
    std::shared_lock<std::shared_mutex> lock(mutex);
    const auto& zoneInfo = currentState.zones.at(zoneName);
    
    return {
        zoneInfo.free,
        zoneInfo.min,
        zoneInfo.low,
        zoneInfo.high,
        zoneInfo.spanned,
        zoneInfo.present,
        zoneInfo.managed,
        zoneInfo.protection,
        zoneInfo.nrFreePages,
        zoneInfo.nrInactive,
        zoneInfo.nrActive,
        zoneInfo.nrUnevictable,
        zoneInfo.nrWriteback,
        zoneInfo.nrSlabReclaimable,
        zoneInfo.nrSlabUnreclaimable,
        zoneInfo.nrKernelStack,
        zoneInfo.nrPageTable,
        zoneInfo.nrBounce,
        zoneInfo.nrFreeCMA,
        zoneInfo.nrLowmemReserve
    };
}

std::map<int, Memory::NumaStats> Memory::getNumaStats() {
    std::shared_lock<std::shared_mutex> lock(mutex);
    std::map<int, NumaStats> result;
    
    for (const auto& [nodeId, nodeInfo] : currentState.numaNodes) {
        result[nodeId] = {
            nodeInfo.total,
            nodeInfo.free,
            nodeInfo.used,
            nodeInfo.distances
        };
    }
    
    return result;
}

Memory::NumaStats Memory::getNumaStats(int nodeId) {
    std::shared_lock<std::shared_mutex> lock(mutex);
    const auto& nodeInfo = currentState.numaNodes.at(nodeId);
    
    return {
        nodeInfo.total,
        nodeInfo.free,
        nodeInfo.used,
        nodeInfo.distances
    };
}

Memory::HugePagesInfo Memory::getHugePagesInfo() {
    std::shared_lock<std::shared_mutex> lock(mutex);
    return {
        currentState.hugePagesTotal,
        currentState.hugePagesFree,
        currentState.hugePagesRsvd,
        currentState.hugePagesSurp,
        currentState.hugePageSize
    };
}

std::future<Memory::Stats> Memory::getStatsAsync() {
    return std::async(std::launch::async, [this]() {
        updateStats();
        return getStats();
    });
}

void Memory::startContinuousMonitoring(std::function<void(const Stats&)> callback) {
    if (isUpdating) {
        return;
    }
    
    isUpdating = true;
    
    updateFuture = std::async(std::launch::async, [this, callback]() {
        while (isUpdating) {
            updateStats();
            callback(getStats());
            
            std::unique_lock<std::mutex> lock(updateMutex);
            updateCV.wait_for(lock, std::chrono::seconds(1), [this]() {
                return !isUpdating;
            });
        }
    });
}

void Memory::stopMonitoring() {
    if (!isUpdating) {
        return;
    }
    
    isUpdating = false;
    updateCV.notify_all();
    
    if (updateFuture.valid()) {
        updateFuture.wait();
    }
}

} // namespace kuserspace
