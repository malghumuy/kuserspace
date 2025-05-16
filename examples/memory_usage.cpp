#include "../include/Memory.h"
#include <iostream>

using namespace kuserspace;

int main() {
    try {
        // Get the Memory singleton instance
        Memory& memory = Memory::getInstance();
        
        // Example 1: Get basic memory stats
        auto stats = memory.getStats();
        std::cout << "Total Memory: " << stats.total << " bytes" << std::endl;
        std::cout << "Free Memory: " << stats.free << " bytes" << std::endl;
        
        // Example 2: Get memory zones
        auto zones = memory.getZoneStats();
        for (const auto& [zoneName, zoneStats] : zones) {
            std::cout << "Zone " << zoneName << " has " 
                      << zoneStats.nrFreePages << " free pages" << std::endl;
        }
        
        // Example 3: Monitor memory usage for 5 seconds
        memory.startContinuousMonitoring([](const Memory::Stats& stats) {
            std::cout << "Memory Usage: " 
                      << (stats.total - stats.free) << " / " 
                      << stats.total << " bytes" << std::endl;
        });
        
        std::this_thread::sleep_for(std::chrono::seconds(5));
        memory.stopMonitoring();
        
        // Example 4: Get NUMA information
        auto numaNodes = memory.getNumaStats();
        for (const auto& [nodeId, nodeStats] : numaNodes) {
            std::cout << "NUMA Node " << nodeId 
                      << " has " << nodeStats.total << " bytes total" << std::endl;
        }
        
        // Example 5: Get huge pages info
        auto hugePages = memory.getHugePagesInfo();
        std::cout << "Huge Pages: " << hugePages.total 
                  << " total, " << hugePages.free << " free" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 