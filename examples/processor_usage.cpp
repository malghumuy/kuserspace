#include "../include/Processor.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>

using namespace kuserspace;

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

// Helper function to print core information
void printCoreInfo(const Processor::CoreInfo& core) {
    std::cout << "Core " << core.id << ":" << std::endl;
    std::cout << "  Status: " << (core.online ? "Online" : "Offline") << std::endl;
    std::cout << "  Model: " << core.modelName << std::endl;
    std::cout << "  Frequency: " << formatFrequency(core.currentFreq) << std::endl;
    std::cout << "  Temperature: " << std::fixed << std::setprecision(1) << core.temperature << "°C" << std::endl;
    std::cout << "  Utilization: " << std::fixed << std::setprecision(1) << core.utilization << "%" << std::endl;
    
    std::cout << "  Cache Information:" << std::endl;
    for (const auto& [type, cache] : core.caches) {
        std::cout << "    " << (type == Processor::CacheType::L1I ? "L1I" :
                              type == Processor::CacheType::L1D ? "L1D" :
                              type == Processor::CacheType::L2 ? "L2" :
                              type == Processor::CacheType::L3 ? "L3" : "L4") << ": ";
        std::cout << cache.size / 1024 << " KB";
        if (cache.shared) {
            std::cout << " (Shared with cores: ";
            for (size_t i = 0; i < cache.sharedCores.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << cache.sharedCores[i];
            }
            std::cout << ")";
        }
        std::cout << std::endl;
    }
}

// Helper function to print package information
void printPackageInfo(const Processor::PackageInfo& package) {
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
    std::cout << "  Temperature: " << std::fixed << std::setprecision(1) << package.temperature << "°C" << std::endl;
}

// Callback function for continuous monitoring
void monitoringCallback(const Processor::Stats& stats) {
    std::cout << "\rCPU Utilization: " << std::fixed << std::setprecision(1) 
              << stats.totalUtilization << "%" << std::flush;
}

int main() {
    try {
        // Get Processor instance
        Processor& processor = Processor::getInstance();
        
        // Print basic CPU information
        std::cout << "CPU Information:" << std::endl;
        std::cout << "Model: " << processor.getModelName() << std::endl;
        std::cout << "Cores: " << processor.getNumCores() << std::endl;
        std::cout << "Threads: " << processor.getNumThreads() << std::endl;
        std::cout << "Packages: " << processor.getNumPackages() << std::endl;
        std::cout << std::endl;
        
        // Print package information
        std::cout << "Package Information:" << std::endl;
        for (const auto& package : processor.getAllPackages()) {
            printPackageInfo(package);
            std::cout << std::endl;
        }
        
        // Print core information
        std::cout << "Core Information:" << std::endl;
        for (const auto& core : processor.getAllCores()) {
            printCoreInfo(core);
            std::cout << std::endl;
        }
        
        // Example of frequency management
        std::cout << "Available Frequencies:" << std::endl;
        auto freqs = processor.getAvailableFrequencies();
        for (const auto& freq : freqs) {
            std::cout << formatFrequency(freq) << " ";
        }
        std::cout << std::endl << std::endl;
        
        // Example of thermal management
        std::cout << "Thermal Information:" << std::endl;
        auto temps = processor.getTemperatures();
        for (size_t i = 0; i < temps.size(); ++i) {
            std::cout << "Core " << i << ": " << std::fixed << std::setprecision(1) 
                      << temps[i] << "°C" << std::endl;
        }
        std::cout << std::endl;
        
        // Example of power management
        std::cout << "Power Information:" << std::endl;
        std::cout << "Current Power: " << std::fixed << std::setprecision(2) 
                  << processor.getPowerConsumption() << " W" << std::endl;
        std::cout << "Power Limit: " << std::fixed << std::setprecision(2) 
                  << processor.getPowerLimit() << " W" << std::endl;
        std::cout << std::endl;
        
        // Example of continuous monitoring
        std::cout << "Starting CPU monitoring for 5 seconds..." << std::endl;
        processor.startContinuousMonitoring(monitoringCallback, std::chrono::seconds(1));
        std::this_thread::sleep_for(std::chrono::seconds(5));
        processor.stopContinuousMonitoring();
        std::cout << std::endl;
        
        // Example of asynchronous statistics
        std::cout << "Getting CPU statistics asynchronously..." << std::endl;
        auto futureStats = processor.getStatsAsync();
        auto stats = futureStats.get();
        std::cout << "CPU Statistics:" << std::endl;
        std::cout << "User Time: " << stats.userTime << std::endl;
        std::cout << "System Time: " << stats.systemTime << std::endl;
        std::cout << "Idle Time: " << stats.idleTime << std::endl;
        std::cout << "Total Utilization: " << std::fixed << std::setprecision(1) 
                  << stats.totalUtilization << "%" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 