#include "include/Memory.h"
#include "include/Processor.h"
#include "include/Parser.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <cstddef>  // for size_t

using namespace kuserspace;

// Helper function to format memory size
std::string formatSize(std::size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024 && unit < 4) {
        size /= 1024;
        unit++;
    }
    
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << size << " " << units[unit];
    return ss.str();
}

// Helper function to format frequency
std::string formatFrequency(uint64_t freq) {
    if (freq >= 1000000000) {
        return std::to_string(freq / 1000000000.0) + " GHz";
    } else if (freq >= 1000000) {
        return std::to_string(freq / 1000000.0) + " MHz";
    } else if (freq >= 1000) {
        return std::to_string(freq / 1000.0) + " KHz";
    }
    return std::to_string(freq) + " Hz";
}

// Memory monitoring callback
void memoryCallback(const Memory::Stats& stats) {
    std::cout << "\rMemory Usage: " << std::fixed << std::setprecision(1)
              << (100.0 * (stats.total - stats.free) / stats.total) << "%" << std::flush;
}

// CPU monitoring callback
void cpuCallback(const Processor::Stats& stats) {
    std::cout << "\rCPU Usage: " << std::fixed << std::setprecision(1)
              << stats.totalUtilization << "%" << std::flush;
}

void printMemoryInfo() {
    std::cout << "\n=== Memory Information ===" << std::endl;
    
    Memory& memory = Memory::getInstance();
    auto stats = memory.getStats();
    
    std::cout << "Total Memory: " << formatSize(stats.total) << std::endl;
    std::cout << "Free Memory: " << formatSize(stats.free) << std::endl;
    std::cout << "Cached Memory: " << formatSize(stats.cached) << std::endl;
    std::cout << "Buffer Memory: " << formatSize(stats.buffers) << std::endl;
    std::cout << "Swap Total: " << formatSize(stats.swapTotal) << std::endl;
    std::cout << "Swap Free: " << formatSize(stats.swapFree) << std::endl;
    
    // Print memory zones using Parser
    auto& parser = Parser::getInstance();
    auto zoneResults = parser.parseFile("/proc/zoneinfo", {
        Parser::Patterns::MEM_ZONE_FREE,
        Parser::Patterns::MEM_ZONE_MIN,
        Parser::Patterns::MEM_ZONE_LOW,
        Parser::Patterns::MEM_ZONE_HIGH
    });
    
    if (!zoneResults.empty()) {
        std::cout << "\nMemory Zones:" << std::endl;
        for (const auto& result : zoneResults) {
            if (result.success) {
                std::cout << "  " << result.value << std::endl;
            }
        }
    }
    
    // Print NUMA information using Parser
    auto numaResults = parser.parseFile("/proc/meminfo", {
        Parser::Patterns::MEM_NUMA_TOTAL,
        Parser::Patterns::MEM_NUMA_FREE,
        Parser::Patterns::MEM_NUMA_USED
    });
    
    if (!numaResults.empty()) {
        std::cout << "\nNUMA Information:" << std::endl;
        for (const auto& result : numaResults) {
            if (result.success) {
                std::cout << "  " << result.value << std::endl;
            }
        }
    }
    
    // Print huge pages information using Parser
    auto hugePageResults = parser.parseFile("/proc/meminfo", {
        Parser::Patterns::MEM_HUGE_PAGES_TOTAL,
        Parser::Patterns::MEM_HUGE_PAGES_FREE,
        Parser::Patterns::MEM_HUGE_PAGES_RSVD,
        Parser::Patterns::MEM_HUGE_PAGES_SURP,
        Parser::Patterns::MEM_HUGE_PAGE_SIZE
    });
    
    if (!hugePageResults.empty()) {
        std::cout << "\nHuge Pages:" << std::endl;
        for (const auto& result : hugePageResults) {
            if (result.success) {
                std::cout << "  " << result.value << std::endl;
            }
        }
    }
}

void printProcessorInfo() {
    std::cout << "\n=== Processor Information ===" << std::endl;
    
    Processor& processor = Processor::getInstance();
    auto& parser = Parser::getInstance();
    
    // Print basic CPU information using Parser
    auto cpuInfo = parser.parseToMap("/proc/cpuinfo", 
        Parser::Patterns::CPU_PROCESSOR,
        Parser::Patterns::CPU_MODEL_NAME);
    
    std::cout << "Model: " << processor.getModelName() << std::endl;
    std::cout << "Cores: " << processor.getNumCores() << std::endl;
    std::cout << "Threads: " << processor.getNumThreads() << std::endl;
    std::cout << "Packages: " << processor.getNumPackages() << std::endl;
    
    // Print package information
    std::cout << "\nPackage Information:" << std::endl;
    for (const auto& package : processor.getAllPackages()) {
        std::cout << "Package " << package.id << ":" << std::endl;
        std::cout << "  Vendor: ";
        switch (package.vendor) {
            case Processor::Vendor::Intel: std::cout << "Intel"; break;
            case Processor::Vendor::AMD: std::cout << "AMD"; break;
            case Processor::Vendor::ARM: std::cout << "ARM"; break;
            case Processor::Vendor::IBM: std::cout << "IBM"; break;
            default: std::cout << "Unknown"; break;
        }
        std::cout << std::endl;
        std::cout << "  Model: " << package.model << std::endl;
        std::cout << "  Cores: " << package.cores << std::endl;
        std::cout << "  Threads: " << package.threads << std::endl;
        std::cout << "  Temperature: " << std::fixed << std::setprecision(1) 
                  << processor.getPackageTemperature(package.id) << "°C" << std::endl;
    }
    
    // Print core information
    std::cout << "\nCore Information:" << std::endl;
    for (const auto& core : processor.getAllCores()) {
        std::cout << "Core " << core.id << ":" << std::endl;
        std::cout << "  Status: " << (core.online ? "Online" : "Offline") << std::endl;
        std::cout << "  Model: " << core.modelName << std::endl;
        std::cout << "  Frequency: " << formatFrequency(core.currentFreq) << std::endl;
        std::cout << "  Temperature: " << std::fixed << std::setprecision(1) 
                  << core.temperature << "°C" << std::endl;
        std::cout << "  Utilization: " << std::fixed << std::setprecision(1) 
                  << core.utilization << "%" << std::endl;
        
        // Print cache information
        std::cout << "  Cache Information:" << std::endl;
        for (const auto& [type, cache] : core.caches) {
            std::cout << "    " << (type == Processor::CacheType::L1I ? "L1I" :
                                  type == Processor::CacheType::L1D ? "L1D" :
                                  type == Processor::CacheType::L2 ? "L2" :
                                  type == Processor::CacheType::L3 ? "L3" : "L4") << ": ";
            std::cout << formatSize(cache.size);
            if (cache.shared) {
                std::cout << " (Shared with cores: ";
                for (std::size_t i = 0; i < cache.sharedCores.size(); ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << cache.sharedCores[i];
                }
                std::cout << ")";
            }
            std::cout << std::endl;
        }
    }
    
    // Print frequency information
    std::cout << "\nAvailable Frequencies:" << std::endl;
    auto freqs = processor.getAvailableFrequencies();
    for (const auto& freq : freqs) {
        std::cout << formatFrequency(freq) << " ";
    }
    std::cout << std::endl;
    
    // Print thermal information
    std::cout << "\nThermal Information:" << std::endl;
    auto temps = processor.getTemperatures();
    for (std::size_t i = 0; i < temps.size(); ++i) {
        std::cout << "Core " << i << ": " << std::fixed << std::setprecision(1) 
                  << temps[i] << "°C" << std::endl;
    }
    
    // Print power information
    std::cout << "\nPower Information:" << std::endl;
    std::cout << "Current Power: " << std::fixed << std::setprecision(2) 
              << processor.getPowerConsumption() << " W" << std::endl;
    std::cout << "Power Limit: " << std::fixed << std::setprecision(2) 
              << processor.getPowerLimit() << " W" << std::endl;
}

int main() {
    try {
        // Print initial system information
        printMemoryInfo();
        printProcessorInfo();
        
        // Start monitoring both memory and CPU
        std::cout << "\nStarting system monitoring for 5 seconds..." << std::endl;
        
        Memory& memory = Memory::getInstance();
        Processor& processor = Processor::getInstance();
        
        memory.startContinuousMonitoring(memoryCallback);
        processor.startContinuousMonitoring(cpuCallback, std::chrono::milliseconds(1000));
        
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        memory.stopMonitoring();
        processor.stopContinuousMonitoring();
        
        std::cout << "\n\nMonitoring complete!" << std::endl;
        
        // Get final statistics asynchronously
        std::cout << "\nGetting final statistics..." << std::endl;
        
        auto futureMemStats = memory.getStatsAsync();
        auto futureCpuStats = processor.getStatsAsync();
        
        auto memStats = futureMemStats.get();
        auto cpuStats = futureCpuStats.get();
        
        std::cout << "Final Memory Usage: " << std::fixed << std::setprecision(1)
                  << (100.0 * (memStats.total - memStats.free) / memStats.total) << "%" << std::endl;
        std::cout << "Final CPU Usage: " << std::fixed << std::setprecision(1)
                  << cpuStats.totalUtilization << "%" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 