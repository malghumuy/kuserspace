// Malghumuy - Library: kuserspace
#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <chrono>
#include <shared_mutex>
#include <future>

namespace kuserspace {

class Processor {
public:
    // Types and enums
    enum class Vendor {
        Intel,
        AMD,
        ARM,
        IBM,
        Unknown
    };

    enum class Architecture {
        x86,
        x86_64,
        ARM,
        ARM64,
        Unknown
    };

    enum class CacheType {
        L1D,
        L1I,
        L2,
        L3
    };

    enum class Governor {
        Performance,
        Powersave,
        Userspace,
        Ondemand,
        Conservative,
        Schedutil,
        Unknown
    };

    enum class ThermalState {
        Normal,
        Warning,
        Critical,
        Unknown
    };

    // Information structures
    struct CacheInfo {
        size_t size;
        size_t lineSize;
        size_t associativity;
        size_t sets;
        bool shared;
        std::vector<int> sharedCores;
    };

    struct CoreInfo {
        int id;
        int physicalId;
        std::string modelName;
        bool online;
        uint64_t currentFreq;
        uint64_t minFreq;
        uint64_t maxFreq;
        Governor currentGovernor;
        std::map<CacheType, CacheInfo> caches;
        float temperature;
        float utilization;
    };

    struct PackageInfo {
        int id;
        Vendor vendor;
        Architecture architecture;
        std::string model;
        int cores;
        int threads;
        std::vector<int> coreIds;
        ThermalState thermalState;
    };

    struct Stats {
        uint64_t userTime;
        uint64_t niceTime;
        uint64_t systemTime;
        uint64_t idleTime;
        uint64_t iowaitTime;
        uint64_t irqTime;
        uint64_t softirqTime;
        uint64_t stealTime;
        uint64_t guestTime;
        uint64_t guestNiceTime;
        float totalUtilization;
        std::vector<float> perCoreUtilization;
    };

    // Constructor/Destructor
    Processor();
    ~Processor();

    static Processor& getInstance();

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
    void startContinuousMonitoring(StatsCallback callback, std::chrono::milliseconds interval);
    void stopContinuousMonitoring();

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

    // Available settings queries
    std::vector<Governor> getAvailableGovernors(int coreId) const;
    std::pair<uint64_t, uint64_t> getFrequencyRange(int coreId) const;
    bool isFrequencyScalingEnabled(int coreId) const;
    bool isThermalMonitoringAvailable() const;
    bool isPowerMonitoringAvailable() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace kuserspace 
