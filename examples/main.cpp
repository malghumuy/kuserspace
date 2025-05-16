// Malghumuy - Library: kuserspace
#include "../include/List.h"
#include "../include/Processor.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>

using namespace kuserspace;

void print_cpu_info(const Processor& proc) {
    std::cout << "CPU Information:\n";
    std::cout << "Model: " << proc.getModelName() << "\n";
    std::cout << "Vendor: " << (proc.getVendor() == Processor::Vendor::Intel ? "Intel" : "AMD") << "\n";
    std::cout << "Cores: " << proc.getNumCores() << "\n";
    std::cout << "Threads: " << proc.getNumThreads() << "\n";
    std::cout << "Packages: " << proc.getNumPackages() << "\n\n";
}

void monitor_cpu(Processor& proc) {
    std::cout << "Starting CPU monitoring...\n";
    
    proc.startContinuousMonitoring([](const Processor::Stats& stats) {
        std::cout << "\rCPU Utilization: " << std::fixed << std::setprecision(1) 
                  << stats.totalUtilization << "%" << std::flush;
    }, std::chrono::seconds(1));
    
    std::this_thread::sleep_for(std::chrono::seconds(5));
    proc.stopContinuousMonitoring();
    std::cout << "\nMonitoring stopped.\n";
}

void list_example() {
    std::cout << "\nList Example:\n";
    
    // Create a thread-safe list
    List<int> numbers;
    
    // Add elements from multiple threads
    std::thread t1([&]() {
        for (int i = 0; i < 5; ++i) {
            numbers.try_push_back(i);
        }
    });
    
    std::thread t2([&]() {
        for (int i = 5; i < 10; ++i) {
            numbers.try_push_back(i);
        }
    });
    
    t1.join();
    t2.join();
    
    // Print the list
    std::cout << "List contents: ";
    for (const auto& num : numbers) {
        std::cout << num << " ";
    }
    std::cout << "\n";
    
    // Demonstrate other operations
    numbers.sort();
    std::cout << "Sorted list: ";
    for (const auto& num : numbers) {
        std::cout << num << " ";
    }
    std::cout << "\n";
}

void processor_example() {
    std::cout << "\nProcessor Example:\n";
    
    // Get processor instance
    auto& proc = Processor::getInstance();
    
    // Print basic information
    print_cpu_info(proc);
    
    // Monitor CPU for 5 seconds
    monitor_cpu(proc);
    
    // Print detailed core information
    for (size_t i = 0; i < proc.getNumCores(); ++i) {
        std::cout << "Core " << i << ":\n";
        std::cout << "  Temperature: " << proc.getCoreTemperature(i) << "°C\n";
        std::cout << "  Frequency: " << proc.getCoreFrequency(i) / 1000 << " MHz\n";
        std::cout << "  Utilization: " << proc.getCoreUtilization(i) << "%\n";
    }
    
    // Print package information
    for (size_t i = 0; i < proc.getNumPackages(); ++i) {
        std::cout << "Package " << i << ":\n";
        std::cout << "  Temperature: " << proc.getPackageTemperature(i) << "°C\n";
    }
}

int main() {
    try {
        // Demonstrate List usage
        list_example();
        
        // Demonstrate Processor usage
        processor_example();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 