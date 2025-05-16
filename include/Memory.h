// Malghumuy - Library: kuserspace
#pragma once

#include "KSpace.h"
#include <shared_mutex>
#include <memory>
#include <mutex>
#include <future>
#include <functional>
#include <atomic>
#include <condition_variable>
#include <vector>
#include <string>
#include <map>

namespace kuserspace {

class Memory {
private:
    // Singleton instance
    static Memory* instance;
    
    // Internal state
    struct State {
        // Basic memory stats
        size_t total;
        size_t free;
        size_t cached;
        size_t buffers;
        size_t swapTotal;
        size_t swapFree;

        // Detailed memory stats
        size_t active;
        size_t inactive;
        size_t activeAnon;
        size_t inactiveAnon;
        size_t activeFile;
        size_t inactiveFile;
        size_t unevictable;
        size_t mlocked;
        size_t highTotal;
        size_t highFree;
        size_t lowTotal;
        size_t lowFree;
        size_t hugePagesTotal;
        size_t hugePagesFree;
        size_t hugePagesRsvd;
        size_t hugePagesSurp;
        size_t hugePageSize;
        size_t directMap4k;
        size_t directMap2M;
        size_t directMap1G;
        
        // Memory zones
        struct ZoneInfo {
            size_t free;
            size_t min;
            size_t low;
            size_t high;
            size_t spanned;
            size_t present;
            size_t managed;
            size_t protection;
            size_t nrFreePages;
            size_t nrInactive;
            size_t nrActive;
            size_t nrUnevictable;
            size_t nrWriteback;
            size_t nrSlabReclaimable;
            size_t nrSlabUnreclaimable;
            size_t nrKernelStack;
            size_t nrPageTable;
            size_t nrBounce;
            size_t nrFreeCMA;
            size_t nrLowmemReserve;
        };
        std::map<std::string, ZoneInfo> zones;

        // NUMA information
        struct NumaNode {
            size_t total;
            size_t free;
            size_t used;
            std::vector<size_t> distances;
        };
        std::map<int, NumaNode> numaNodes;
    };
    
    // Private members
    std::atomic<bool> isUpdating;
    std::shared_mutex mutex;
    State currentState;
    std::future<void> updateFuture;
    std::condition_variable updateCV;
    std::mutex updateMutex;
    
    // Private constructor for singleton
    Memory();
    
    // Prevent copying
    Memory(const Memory&) = delete;
    Memory& operator=(const Memory&) = delete;

    // Internal methods
    void updateStats();
    void readProcMeminfo();
    void readProcSwaps();
    void readMemoryZones();
    void readNumaInfo();
    void readHugePages();

public:
    // Memory statistics structure
    struct Stats {
        // Basic memory stats
        size_t total;
        size_t free;
        size_t cached;
        size_t buffers;
        size_t swapTotal;
        size_t swapFree;

        // Detailed memory stats
        size_t active;
        size_t inactive;
        size_t activeAnon;
        size_t inactiveAnon;
        size_t activeFile;
        size_t inactiveFile;
        size_t unevictable;
        size_t mlocked;
        size_t highTotal;
        size_t highFree;
        size_t lowTotal;
        size_t lowFree;
        size_t hugePagesTotal;
        size_t hugePagesFree;
        size_t hugePagesRsvd;
        size_t hugePagesSurp;
        size_t hugePageSize;
        size_t directMap4k;
        size_t directMap2M;
        size_t directMap1G;
    };

    // Memory zone information
    struct ZoneStats {
        size_t free;
        size_t min;
        size_t low;
        size_t high;
        size_t spanned;
        size_t present;
        size_t managed;
        size_t protection;
        size_t nrFreePages;
        size_t nrInactive;
        size_t nrActive;
        size_t nrUnevictable;
        size_t nrWriteback;
        size_t nrSlabReclaimable;
        size_t nrSlabUnreclaimable;
        size_t nrKernelStack;
        size_t nrPageTable;
        size_t nrBounce;
        size_t nrFreeCMA;
        size_t nrLowmemReserve;
    };

    // NUMA node information
    struct NumaStats {
        size_t total;
        size_t free;
        size_t used;
        std::vector<size_t> distances;
    };

    // Singleton instance getter
    static inline Memory& getInstance() {
        if (!instance) {
            instance = new Memory();
        }
        return *instance;
    }
    
    // Destructor
    ~Memory();

    // Basic stats methods
    Stats getStats();
    std::future<Stats> getStatsAsync();
    
    // Zone information methods
    std::map<std::string, ZoneStats> getZoneStats();
    ZoneStats getZoneStats(const std::string& zoneName);
    
    // NUMA information methods
    std::map<int, NumaStats> getNumaStats();
    NumaStats getNumaStats(int nodeId);
    
    // Huge pages information
    struct HugePagesInfo {
        size_t total;
        size_t free;
        size_t reserved;
        size_t surplus;
        size_t pageSize;
    };
    HugePagesInfo getHugePagesInfo();

    // Monitoring methods
    void startContinuousMonitoring(std::function<void(const Stats&)> callback);
    void stopMonitoring();
    bool isMonitoring() const { return isUpdating; }
    void reset() { stopMonitoring(); }
};

} // namespace kuserspace
