// Malghumuy - Library: kuserspace
#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <future>
#include <functional>
#include <shared_mutex>
#include <optional>

namespace kuserspace {

class Processor {
public:
    // CPU Architecture types
    enum class Architecture {
        X86_64,
        ARM64,
        ARM,
        PPC64,
        S390X,
        RISC_V,
        Unknown
    };

    // CPU Vendor types
    enum class Vendor {
        Intel,
        AMD,
        ARM,
        IBM,
        Unknown
    };

    // CPU Cache types
    enum class CacheType {
        L1I,    // L1 Instruction
        L1D,    // L1 Data
        L2,     // L2 Unified
        L3,     // L3 Unified
        L4      // L4 Unified (if available)
    };

    // CPU Frequency scaling governors
    enum class Governor {
        Performance,
        Powersave,
        Userspace,
        Ondemand,
        Conservative,
        Schedutil,
        Unknown
    };

    // CPU Thermal state
    enum class ThermalState {
        Normal,
        Warning,
        Critical,
        Emergency,
        Unknown
    };

    // Cache information structure
    struct CacheInfo {
        size_t size;           // Size in bytes
        size_t lineSize;       // Cache line size
        size_t associativity;  // Cache associativity
        size_t sets;           // Number of sets
        bool shared;           // Whether cache is shared between cores
        std::vector<int> sharedCores;  // Cores sharing this cache
    };

    // Core information structure
    struct CoreInfo {
        int id;                     // Core ID
        bool online;                // Whether core is online
        int physicalId;            // Physical package ID
        int coreId;                // Core ID within package
        int threadId;              // Thread ID within core
        std::string modelName;     // CPU model name
        uint64_t maxFreq;          // Maximum frequency in Hz
        uint64_t minFreq;          // Minimum frequency in Hz
        uint64_t currentFreq;      // Current frequency in Hz
        Governor currentGovernor;  // Current frequency governor
        float temperature;         // Temperature in Celsius
        ThermalState thermalState; // Current thermal state
        float utilization;         // CPU utilization percentage
        std::map<CacheType, CacheInfo> caches;  // Cache information
    };

    // Package (CPU socket) information
    struct PackageInfo {
        int id;                     // Package ID
        Vendor vendor;              // CPU vendor
        std::string model;          // CPU model
        Architecture architecture;  // CPU architecture
        size_t cores;              // Number of cores
        size_t threads;            // Number of threads
        std::vector<int> coreIds;  // IDs of cores in this package
        float temperature;         // Package temperature
        ThermalState thermalState; // Package thermal state
    };

    // System-wide CPU statistics
    struct Stats {
        uint64_t userTime;      // Time spent in user mode
        uint64_t niceTime;      // Time spent in user mode with low priority
        uint64_t systemTime;    // Time spent in system mode
        uint64_t idleTime;      // Time spent in idle task
        uint64_t iowaitTime;    // Time spent waiting for I/O
        uint64_t irqTime;       // Time spent servicing interrupts
        uint64_t softirqTime;   // Time spent servicing softirqs
        uint64_t stealTime;     // Time stolen by other operating systems
        uint64_t guestTime;     // Time spent running a virtual CPU
        uint64_t guestNiceTime; // Time spent running a niced guest
        float totalUtilization; // Total CPU utilization percentage
        std::vector<float> perCoreUtilization; // Per-core utilization
    };

    // Get singleton instance
    static Processor& getInstance();

    // Delete copy constructor and assignment operator
    Processor(const Processor&) = delete;
    Processor& operator=(const Processor&) = delete;

    // Basic CPU Information
    std::string getModelName() const;
    Vendor getVendor() const;
    Architecture getArchitecture() const;
    size_t getNumCores() const;
    size_t getNumThreads() const;
    size_t getNumPackages() const;

    // Core Information
    std::vector<CoreInfo> getAllCores() const;
    CoreInfo getCoreInfo(int coreId) const;
    bool isCoreOnline(int coreId) const;
    bool setCoreOnline(int coreId, bool online);
    float getCoreTemperature(int coreId) const;
    float getCoreUtilization(int coreId) const;
    uint64_t getCoreFrequency(int coreId) const;
    Governor getCoreGovernor(int coreId) const;
    bool setCoreGovernor(int coreId, Governor governor);

    // Package Information
    std::vector<PackageInfo> getAllPackages() const;
    PackageInfo getPackageInfo(int packageId) const;
    float getPackageTemperature(int packageId) const;

    // Cache Information
    std::map<CacheType, CacheInfo> getCacheInfo(int coreId) const;
    CacheInfo getCacheInfo(int coreId, CacheType type) const;

    // System-wide Statistics
    Stats getStats() const;
    std::future<Stats> getStatsAsync() const;

    // Continuous Monitoring
    using StatsCallback = std::function<void(const Stats&)>;
    void startContinuousMonitoring(StatsCallback callback, 
                                 std::chrono::milliseconds interval = std::chrono::seconds(1));
    void stopMonitoring();

    // Thermal Management
    ThermalState getThermalState() const;
    std::vector<float> getTemperatures() const;
    bool setThermalLimit(float temperature);

    // Frequency Management
    std::vector<uint64_t> getAvailableFrequencies() const;
    bool setFrequency(int coreId, uint64_t frequency);
    bool setFrequencyRange(int coreId, uint64_t minFreq, uint64_t maxFreq);

    // Power Management
    float getPowerConsumption() const;
    float getPowerLimit() const;
    bool setPowerLimit(float watts);

private:
    Processor();
    ~Processor();

    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace kuserspace 
