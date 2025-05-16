#include "../include/Processor.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <filesystem>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace kuserspace {

class Processor::Impl {
public:
    Impl() : monitoringActive(false) {
        initialize();
    }

    void initialize() {
        // Read CPU information from /proc/cpuinfo
        readCpuInfo();
        // Read cache information
        readCacheInfo();
        // Initialize thermal monitoring
        initializeThermal();
        // Initialize frequency scaling
        initializeFrequencyScaling();
    }

    void readCpuInfo() {
        std::ifstream cpuinfo("/proc/cpuinfo");
        std::string line;
        int currentCore = -1;
        int currentPackage = -1;

        while (std::getline(cpuinfo, line)) {
            if (line.empty()) {
                currentCore = -1;
                continue;
            }

            std::smatch matches;
            if (std::regex_match(line, matches, std::regex("processor\\s+:\\s+(\\d+)"))) {
                currentCore = std::stoi(matches[1]);
                cores[currentCore] = CoreInfo{};
                cores[currentCore].id = currentCore;
            }
            else if (std::regex_match(line, matches, std::regex("physical id\\s+:\\s+(\\d+)"))) {
                currentPackage = std::stoi(matches[1]);
                if (packages.find(currentPackage) == packages.end()) {
                    packages[currentPackage] = PackageInfo{};
                    packages[currentPackage].id = currentPackage;
                }
                cores[currentCore].physicalId = currentPackage;
                packages[currentPackage].coreIds.push_back(currentCore);
            }
            else if (std::regex_match(line, matches, std::regex("model name\\s+:\\s+(.+)"))) {
                cores[currentCore].modelName = matches[1];
                packages[currentPackage].model = matches[1];
            }
            else if (std::regex_match(line, matches, std::regex("vendor_id\\s+:\\s+(.+)"))) {
                std::string vendor = matches[1];
                if (vendor.find("Intel") != std::string::npos) {
                    packages[currentPackage].vendor = Vendor::Intel;
                }
                else if (vendor.find("AMD") != std::string::npos) {
                    packages[currentPackage].vendor = Vendor::AMD;
                }
                else if (vendor.find("ARM") != std::string::npos) {
                    packages[currentPackage].vendor = Vendor::ARM;
                }
                else if (vendor.find("IBM") != std::string::npos) {
                    packages[currentPackage].vendor = Vendor::IBM;
                }
                else {
                    packages[currentPackage].vendor = Vendor::Unknown;
                }
            }
        }

        // Update package information
        for (auto& [id, package] : packages) {
            package.cores = package.coreIds.size();
            package.threads = package.cores * 2; // Assuming SMT/Hyperthreading
        }
    }

    void readCacheInfo() {
        for (const auto& [coreId, core] : cores) {
            std::string cachePath = "/sys/devices/system/cpu/cpu" + std::to_string(coreId) + "/cache/";
            for (int i = 0; i < 4; ++i) { // L1 to L4
                std::string levelPath = cachePath + "index" + std::to_string(i) + "/";
                if (!std::filesystem::exists(levelPath)) continue;

                CacheInfo cache;
                std::ifstream sizeFile(levelPath + "size");
                std::ifstream lineSizeFile(levelPath + "coherency_line_size");
                std::ifstream waysFile(levelPath + "ways_of_associativity");
                std::ifstream setsFile(levelPath + "number_of_sets");
                std::ifstream sharedFile(levelPath + "shared_cpu_list");

                sizeFile >> cache.size;
                lineSizeFile >> cache.lineSize;
                waysFile >> cache.associativity;
                setsFile >> cache.sets;

                std::string sharedCores;
                std::getline(sharedFile, sharedCores);
                cache.shared = !sharedCores.empty();
                if (cache.shared) {
                    std::istringstream iss(sharedCores);
                    std::string core;
                    while (std::getline(iss, core, ',')) {
                        cache.sharedCores.push_back(std::stoi(core));
                    }
                }

                CacheType type;
                switch (i) {
                    case 0: type = CacheType::L1D; break;
                    case 1: type = CacheType::L1I; break;
                    case 2: type = CacheType::L2; break;
                    case 3: type = CacheType::L3; break;
                    default: continue;
                }

                cores[coreId].caches[type] = cache;
            }
        }
    }

    void initializeThermal() {
        // Initialize thermal monitoring
        for (const auto& [coreId, core] : cores) {
            std::string thermalPath = "/sys/class/thermal/thermal_zone" + std::to_string(coreId) + "/";
            if (std::filesystem::exists(thermalPath)) {
                thermalPaths[coreId] = thermalPath;
            }
        }
    }

    void initializeFrequencyScaling() {
        // Initialize frequency scaling
        for (const auto& [coreId, core] : cores) {
            std::string freqPath = "/sys/devices/system/cpu/cpu" + std::to_string(coreId) + "/cpufreq/";
            if (std::filesystem::exists(freqPath)) {
                freqPaths[coreId] = freqPath;
                updateCoreFrequency(coreId);
            }
        }
    }

    void updateCoreFrequency(int coreId) {
        if (freqPaths.find(coreId) == freqPaths.end()) return;

        std::ifstream scalingCurFreq(freqPaths[coreId] + "scaling_cur_freq");
        std::ifstream scalingMinFreq(freqPaths[coreId] + "scaling_min_freq");
        std::ifstream scalingMaxFreq(freqPaths[coreId] + "scaling_max_freq");

        scalingCurFreq >> cores[coreId].currentFreq;
        scalingMinFreq >> cores[coreId].minFreq;
        scalingMaxFreq >> cores[coreId].maxFreq;
    }

    Stats getStats() const {
        Stats stats;
        std::ifstream statFile("/proc/stat");
        std::string line;
        std::getline(statFile, line); // Skip first line (total)

        std::istringstream iss(line);
        iss >> stats.userTime >> stats.niceTime >> stats.systemTime >> stats.idleTime
            >> stats.iowaitTime >> stats.irqTime >> stats.softirqTime >> stats.stealTime
            >> stats.guestTime >> stats.guestNiceTime;

        // Calculate utilization
        uint64_t totalTime = stats.userTime + stats.niceTime + stats.systemTime + stats.idleTime
                           + stats.iowaitTime + stats.irqTime + stats.softirqTime + stats.stealTime;
        uint64_t idleTime = stats.idleTime + stats.iowaitTime;
        stats.totalUtilization = 100.0f * (1.0f - static_cast<float>(idleTime) / totalTime);

        // Get per-core utilization
        stats.perCoreUtilization.clear();
        while (std::getline(statFile, line)) {
            if (line.find("cpu") == 0) continue;
            std::istringstream coreIss(line);
            std::string cpu;
            uint64_t user, nice, system, idle, iowait, irq, softirq, steal, guest, guestNice;
            coreIss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal >> guest >> guestNice;

            uint64_t coreTotal = user + nice + system + idle + iowait + irq + softirq + steal;
            uint64_t coreIdle = idle + iowait;
            float coreUtil = 100.0f * (1.0f - static_cast<float>(coreIdle) / coreTotal);
            stats.perCoreUtilization.push_back(coreUtil);
        }

        return stats;
    }

    void startMonitoring(StatsCallback callback, std::chrono::milliseconds interval) {
        monitoringActive = true;
        monitoringThread = std::thread([this, callback, interval]() {
            while (monitoringActive) {
                callback(getStats());
                std::this_thread::sleep_for(interval);
            }
        });
    }

    void stopMonitoring() {
        monitoringActive = false;
        if (monitoringThread.joinable()) {
            monitoringThread.join();
        }
    }

    bool setCoreOnline(int coreId, bool online) {
        std::string path = "/sys/devices/system/cpu/cpu" + std::to_string(coreId) + "/online";
        std::ofstream onlineFile(path);
        if (!onlineFile) return false;
        onlineFile << (online ? "1" : "0");
        cores[coreId].online = online;
        return true;
    }

    bool setCoreGovernor(int coreId, Governor governor) {
        if (freqPaths.find(coreId) == freqPaths.end()) return false;

        std::string governorStr;
        switch (governor) {
            case Governor::Performance: governorStr = "performance"; break;
            case Governor::Powersave: governorStr = "powersave"; break;
            case Governor::Userspace: governorStr = "userspace"; break;
            case Governor::Ondemand: governorStr = "ondemand"; break;
            case Governor::Conservative: governorStr = "conservative"; break;
            case Governor::Schedutil: governorStr = "schedutil"; break;
            default: return false;
        }

        std::ofstream governorFile(freqPaths[coreId] + "scaling_governor");
        if (!governorFile) return false;
        governorFile << governorStr;
        cores[coreId].currentGovernor = governor;
        return true;
    }

    bool setFrequency(int coreId, uint64_t frequency) {
        if (freqPaths.find(coreId) == freqPaths.end()) return false;
        if (cores[coreId].currentGovernor != Governor::Userspace) return false;

        std::ofstream freqFile(freqPaths[coreId] + "scaling_setspeed");
        if (!freqFile) return false;
        freqFile << frequency;
        cores[coreId].currentFreq = frequency;
        return true;
    }

    bool setThermalLimit(float temperature) {
        for (const auto& [coreId, path] : thermalPaths) {
            std::ofstream tripFile(path + "trip_point_0_temp");
            if (!tripFile) continue;
            tripFile << static_cast<int>(temperature * 1000); // Convert to millicelsius
        }
        return true;
    }

    bool setPowerLimit(float watts) {
        for (const auto& [packageId, _] : packages) {
            std::string path = "/sys/class/powercap/intel-rapl:" + std::to_string(packageId) + ":0/constraint_0_power_limit_uw";
            if (!std::filesystem::exists(path)) continue;
            std::ofstream powerFile(path);
            if (!powerFile) continue;
            powerFile << static_cast<int>(watts * 1000000); // Convert to microwatts
        }
        return true;
    }

    std::map<int, CoreInfo> cores;
    std::map<int, PackageInfo> packages;
    std::map<int, std::string> thermalPaths;
    std::map<int, std::string> freqPaths;
    std::atomic<bool> monitoringActive;
    std::thread monitoringThread;
};

// Singleton instance
Processor& Processor::getInstance() {
    static Processor instance;
    return instance;
}

Processor::Processor() : pImpl(std::make_unique<Impl>()) {}
Processor::~Processor() = default;

// Basic CPU Information
std::string Processor::getModelName() const {
    return pImpl->cores[0].modelName;
}

Processor::Vendor Processor::getVendor() const {
    return pImpl->packages[0].vendor;
}

Processor::Architecture Processor::getArchitecture() const {
    return pImpl->packages[0].architecture;
}

size_t Processor::getNumCores() const {
    return pImpl->cores.size();
}

size_t Processor::getNumThreads() const {
    size_t threads = 0;
    for (const auto& [id, package] : pImpl->packages) {
        threads += package.threads;
    }
    return threads;
}

size_t Processor::getNumPackages() const {
    return pImpl->packages.size();
}

// Core Information
std::vector<Processor::CoreInfo> Processor::getAllCores() const {
    std::vector<CoreInfo> result;
    for (const auto& [id, core] : pImpl->cores) {
        result.push_back(core);
    }
    return result;
}

Processor::CoreInfo Processor::getCoreInfo(int coreId) const {
    return pImpl->cores[coreId];
}

bool Processor::isCoreOnline(int coreId) const {
    return pImpl->cores[coreId].online;
}

bool Processor::setCoreOnline(int coreId, bool online) {
    return pImpl->setCoreOnline(coreId, online);
}

float Processor::getCoreTemperature(int coreId) const {
    if (pImpl->thermalPaths.find(coreId) == pImpl->thermalPaths.end()) return 0.0f;
    std::ifstream tempFile(pImpl->thermalPaths.at(coreId) + "temp");
    float temp;
    tempFile >> temp;
    return temp / 1000.0f; // Convert from millicelsius to celsius
}

float Processor::getCoreUtilization(int coreId) const {
    return pImpl->cores[coreId].utilization;
}

uint64_t Processor::getCoreFrequency(int coreId) const {
    return pImpl->cores[coreId].currentFreq;
}

Processor::Governor Processor::getCoreGovernor(int coreId) const {
    return pImpl->cores[coreId].currentGovernor;
}

bool Processor::setCoreGovernor(int coreId, Governor governor) {
    return pImpl->setCoreGovernor(coreId, governor);
}

// Package Information
std::vector<Processor::PackageInfo> Processor::getAllPackages() const {
    std::vector<PackageInfo> result;
    for (const auto& [id, package] : pImpl->packages) {
        result.push_back(package);
    }
    return result;
}

Processor::PackageInfo Processor::getPackageInfo(int packageId) const {
    return pImpl->packages[packageId];
}

float Processor::getPackageTemperature(int packageId) const {
    return pImpl->packages[packageId].temperature;
}

// Cache Information
std::map<Processor::CacheType, Processor::CacheInfo> Processor::getCacheInfo(int coreId) const {
    return pImpl->cores[coreId].caches;
}

Processor::CacheInfo Processor::getCacheInfo(int coreId, CacheType type) const {
    return pImpl->cores[coreId].caches[type];
}

// System-wide Statistics
Processor::Stats Processor::getStats() const {
    return pImpl->getStats();
}

std::future<Processor::Stats> Processor::getStatsAsync() const {
    return std::async(std::launch::async, [this]() { return getStats(); });
}

// Continuous Monitoring
void Processor::startContinuousMonitoring(StatsCallback callback, std::chrono::milliseconds interval) {
    pImpl->startMonitoring(callback, interval);
}

void Processor::stopMonitoring() {
    pImpl->stopMonitoring();
}

// Thermal Management
Processor::ThermalState Processor::getThermalState() const {
    return pImpl->packages[0].thermalState;
}

std::vector<float> Processor::getTemperatures() const {
    std::vector<float> temps;
    for (const auto& [coreId, core] : pImpl->cores) {
        temps.push_back(core.temperature);
    }
    return temps;
}

bool Processor::setThermalLimit(float temperature) {
    return pImpl->setThermalLimit(temperature);
}

// Frequency Management
std::vector<uint64_t> Processor::getAvailableFrequencies() const {
    std::vector<uint64_t> freqs;
    if (pImpl->freqPaths.empty()) return freqs;

    std::ifstream freqFile(pImpl->freqPaths.begin()->second + "scaling_available_frequencies");
    uint64_t freq;
    while (freqFile >> freq) {
        freqs.push_back(freq);
    }
    return freqs;
}

bool Processor::setFrequency(int coreId, uint64_t frequency) {
    return pImpl->setFrequency(coreId, frequency);
}

bool Processor::setFrequencyRange(int coreId, uint64_t minFreq, uint64_t maxFreq) {
    if (pImpl->freqPaths.find(coreId) == pImpl->freqPaths.end()) return false;

    std::ofstream minFreqFile(pImpl->freqPaths[coreId] + "scaling_min_freq");
    std::ofstream maxFreqFile(pImpl->freqPaths[coreId] + "scaling_max_freq");
    if (!minFreqFile || !maxFreqFile) return false;

    minFreqFile << minFreq;
    maxFreqFile << maxFreq;
    return true;
}

// Power Management
float Processor::getPowerConsumption() const {
    float totalPower = 0.0f;
    for (const auto& [packageId, _] : pImpl->packages) {
        std::string path = "/sys/class/powercap/intel-rapl:" + std::to_string(packageId) + ":0/energy_uj";
        if (!std::filesystem::exists(path)) continue;
        std::ifstream powerFile(path);
        uint64_t energy;
        powerFile >> energy;
        totalPower += energy / 1000000.0f; // Convert from microjoules to joules
    }
    return totalPower;
}

float Processor::getPowerLimit() const {
    float limit = 0.0f;
    for (const auto& [packageId, _] : pImpl->packages) {
        std::string path = "/sys/class/powercap/intel-rapl:" + std::to_string(packageId) + ":0/constraint_0_power_limit_uw";
        if (!std::filesystem::exists(path)) continue;
        std::ifstream limitFile(path);
        uint64_t powerLimit;
        limitFile >> powerLimit;
        limit += powerLimit / 1000000.0f; // Convert from microwatts to watts
    }
    return limit;
}

bool Processor::setPowerLimit(float watts) {
    return pImpl->setPowerLimit(watts);
}

} // namespace kuserspace 